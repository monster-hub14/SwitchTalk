/* Native, dependency-free 1280x720 UI. Only this file owns the framebuffer. */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#include "gui_font.h"
#include "avatars.h"
#ifndef GUI_HEADLESS
#include <switch.h>
static Framebuffer framebuffer;
#endif
static uint32_t *pixels;
static unsigned stride;
static bool ready,touch_held;
static char own_user[128];static AvatarKey call_avatar;
typedef struct {int x,y,w,h,value;GuiAction action;} Hit;
static Hit hits[128];
static unsigned hit_count;
#ifdef RGB
#undef RGB
#endif
#define RGB(r,g,b) ((uint32_t)(r)|((uint32_t)(g)<<8)|((uint32_t)(b)<<16)|0xff000000u)
#define BLUE RGB(0,130,201)
#define INK RGB(27,43,55)
#define MUTED RGB(98,116,128)
#define BG RGB(245,248,250)
#define WHITE RGB(255,255,255)
#define LINE RGB(224,232,237)
#define PALE RGB(226,242,251)
#define GREEN RGB(26,139,97)
#define RED RGB(199,58,69)
static int min_i(int a,int b){return a<b?a:b;}
static int max_i(int a,int b){return a>b?a:b;}
static void rect(int x,int y,int w,int h,uint32_t color){
 int x0=max_i(0,x),y0=max_i(0,y),x1=min_i(GUI_WIDTH,x+w),y1=min_i(GUI_HEIGHT,y+h);
 for(int j=y0;j<y1;j++)for(int i=x0;i<x1;i++)pixels[j*stride+i]=color;
}
static void circle(int x,int y,int radius,uint32_t color){
 for(int j=-radius;j<=radius;j++)for(int i=-radius;i<=radius;i++)if(i*i+j*j<=radius*radius)rect(x+i,y+j,1,1,color);
}
static void round_rect(int x,int y,int w,int h,int r,uint32_t color){
 if(w<2*r || h<2*r){rect(x,y,w,h,color);return;}
 rect(x+r,y,w-2*r,h,color);rect(x,y+r,w,h-2*r,color);
 circle(x+r,y+r,r,color);circle(x+w-r-1,y+r,r,color);
 circle(x+r,y+h-r-1,r,color);circle(x+w-r-1,y+h-r-1,r,color);
}
static void line(int x0,int y0,int x1,int y1,int thick,uint32_t color){
 int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
 for(;;){rect(x0-thick/2,y0-thick/2,thick,thick,color);if(x0==x1 && y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
}
static unsigned decode(const char **p){
 const unsigned char *s=(const unsigned char*)*p;unsigned cp=s[0];int n=1;
 if(cp>=0xc2 && cp<=0xdf){cp&=31;n=2;}
 else if(cp>=0xe0 && cp<=0xef){cp&=15;n=3;}
 else if(cp>=0xf0 && cp<=0xf4){cp&=7;n=4;}
 else if(cp>=128){(*p)++;return '?';}
 for(int j=1;j<n;j++){if(!s[j] || (s[j]&0xc0)!=0x80){(*p)++;return '?';}cp=(cp<<6)|(s[j]&63);}
 *p+=n;
 if(cp==0x2019 || cp==0x2018)return '\'';
 if(cp==0x201c || cp==0x201d)return '"';
 if(cp==0x2013 || cp==0x2014)return '-';
 if(cp==0x2026)return '.';
 return cp;
}
static const unsigned char *font_data(int size){return size==2?gui_font_2:(size==1?gui_font_1:gui_font_0);}
static const unsigned char *advances(int size){return size==2?gui_advance_2:(size==1?gui_advance_1:gui_advance_0);}
static unsigned printable(unsigned cp){return cp>=32 && cp<256?cp:'?';}
static int text_width(const char *s,int size){int w=0;while(*s){unsigned c=printable(decode(&s));w+=advances(size)[c-32];}return w;}
static void glyph(int x,int y,unsigned c,int size,uint32_t color){
 c=printable(c);const unsigned char *src=font_data(size)+(c-32)*1600;
 for(int j=0;j<40;j++)for(int i=0;i<40;i++){
  int px=x+i,py=y+j;unsigned a=src[j*40+i];
  if(!a || px<0 || py<0 || px>=GUI_WIDTH || py>=GUI_HEIGHT)continue;
  uint32_t old=pixels[py*stride+px],out=0xff000000u;
  for(int k=0;k<24;k+=8){unsigned v=(((color>>k)&255)*a+((old>>k)&255)*(255-a)+127)/255;out|=v<<k;}
  pixels[py*stride+px]=out;
 }
}
static void text(int x,int y,const char *s,int size,uint32_t color,int maxwidth){
 int origin=x;while(*s){unsigned cp=printable(decode(&s));int a=advances(size)[cp-32];
  if(x-origin+a>maxwidth){if(maxwidth>=16){glyph(origin+maxwidth-14,y,'.',size,color);glyph(origin+maxwidth-8,y,'.',size,color);}break;}
  glyph(x,y,cp,size,color);x+=a;
 }
}
static void center(int x,int y,int w,const char *s,int size,uint32_t color){text(x+max_i(0,(w-text_width(s,size))/2),y,s,size,color,w);}
static void add_hit(int x,int y,int w,int h,GuiAction action,int value){if(hit_count<128)hits[hit_count++]=(Hit){x,y,w,h,value,action};}
static void button(int x,int y,int w,int h,const char *label,GuiAction action,bool enabled,uint32_t fill,uint32_t fg){
 round_rect(x,y,w,h,12,enabled?fill:LINE);center(x+6,y+(h-(22+4))/2,w-12,label,1,enabled?fg:MUTED);
 if(enabled)add_hit(x,y,w,h,action,0);
}
static void avatar(int x,int y,int r,const char *name,uint32_t color){
 circle(x,y,r,color);char initial[2]={'?',0};
 if(name && name[0]){unsigned c=decode(&name);initial[0]=(char)printable(c);if(initial[0]>='a' && initial[0]<='z')initial[0]-=32;}
 center(x-r,y-18,r*2,initial,2,WHITE);
}
static void photo_avatar(int x,int y,int radius,const char *name,uint32_t color,AvatarKey key){
 static uint32_t image[AVATAR_PIXELS];
 if(!avatars_get(&key,image)){avatar(x,y,radius,name,color);return;}
 circle(x,y,radius,color);
 for(int j=-radius;j<=radius;j++)for(int i=-radius;i<=radius;i++){
  if(i*i+j*j>radius*radius)continue;int px=x+i,py=y+j;
  if(px<0 || py<0 || px>=GUI_WIDTH || py>=GUI_HEIGHT)continue;
  int sx=(i+radius)*AVATAR_SIDE/(radius*2+1),sy=(j+radius)*AVATAR_SIDE/(radius*2+1);
  uint32_t rgba=image[sy*AVATAR_SIDE+sx],old=pixels[py*stride+px];unsigned a=rgba>>24;uint32_t result=0xff000000u;
  for(int c=0;c<24;c+=8)result|=(((((rgba>>c)&255)*a+((old>>c)&255)*(255-a)+127)/255)<<c);
  pixels[py*stride+px]=result;
 }
}
void gui_call_room(const TalkRoom *room){call_avatar=avatars_room_key(room);}
static void bubble_icon(int x,int y,uint32_t color){
 round_rect(x,y,32,23,8,color);line(x+8,y+21,x+4,y+30,4,color);
 uint32_t dot=color==WHITE?BLUE:WHITE;circle(x+9,y+11,2,dot);circle(x+16,y+11,2,dot);circle(x+23,y+11,2,dot);
}
static void begin(uint32_t clear_color){
#ifndef GUI_HEADLESS
 u32 bytes;pixels=framebufferBegin(&framebuffer,&bytes);stride=bytes/4;
#endif
 hit_count=0;rect(0,0,GUI_WIDTH,GUI_HEIGHT,clear_color);
}
static void end(void){
#ifndef GUI_HEADLESS
 framebufferEnd(&framebuffer);
#endif
}
bool gui_init(void){
#ifndef GUI_HEADLESS
 Result rc=framebufferCreate(&framebuffer,nwindowGetDefault(),GUI_WIDTH,GUI_HEIGHT,PIXEL_FORMAT_RGBA_8888,2);
 if(R_FAILED(rc))return false;
 rc=framebufferMakeLinear(&framebuffer);if(R_FAILED(rc)){framebufferClose(&framebuffer);return false;}
#endif
 ready=true;touch_held=false;return true;
}
void gui_exit(void){
#ifndef GUI_HEADLESS
 if(ready)framebufferClose(&framebuffer);
#endif
 ready=false;pixels=NULL;
}
#ifdef GUI_HEADLESS
void gui_test_buffer(uint32_t *buffer,unsigned stride_pixels){pixels=buffer;stride=stride_pixels;ready=true;}
#endif
static void header(void){
 rect(0,0,1280,64,BLUE);bubble_icon(24,18,WHITE);text(72,14,"SwitchTalk",2,WHITE,250);
 text(263,24,"By Monster Review",0,RGB(211,237,251),400);
 if(own_user[0])photo_avatar(1005,32,18,own_user,RGB(63,164,218),avatars_user_key(own_user));
 text(1038,24,"Version 1.0.1",0,WHITE,222);
}
static void footer(const char *hint){rect(0,682,1280,38,WHITE);rect(0,682,1280,1,LINE);text(22,690,hint,0,MUTED,1236);}
static void status_bar(const char *status,const char *error,bool busy){
 rect(320,640,960,42,WHITE);rect(320,640,960,1,LINE);
 circle(340,660,4,error[0]?RED:(busy?BLUE:GREEN));text(352,648,error[0]?error:status,0,error[0]?RED:MUTED,902);
}
static void sidebar(const GuiModel *m){
 const TalkClient *c=m->client;rect(0,64,320,618,WHITE);rect(319,64,1,618,LINE);
 text(24,82,"Conversations",1,INK,236);char count[12];snprintf(count,sizeof(count),"%d",c->room_count);text(276,86,count,0,MUTED,40);
 int start=m->room_page*GUI_ROOMS_PER_PAGE;
 if(!c->room_count)text(24,150,"No conversations yet",0,MUTED,270);
 for(int i=start;i<c->room_count && i<start+GUI_ROOMS_PER_PAGE;i++){
  int y=126+(i-start)*54;const TalkRoom *r=&c->rooms[i];
  bool selected=m->view==2?i==m->current_room:i==m->room_sel;
  if(selected){round_rect(10,y,300,50,10,PALE);rect(10,y+10,3,30,BLUE);}
  photo_avatar(40,y+25,17,r->title,selected?BLUE:RGB(115,143,162),avatars_room_key(r));
  text(66,y+7,r->title,0,INK,210);
  text(66,y+28,i==m->current_room && m->view==2?"Open conversation":"Tap to open",0,MUTED,204);
  if(r->unread)circle(294,y+24,4,BLUE);
  if(!m->busy)add_hit(10,y,300,50,GUI_SELECT_ROOM,i);
 }
 if(m->view==1){
  button(16,565,82,34,"<",GUI_PAGE_UP,start>0,PALE,BLUE);
  char p[32];snprintf(p,sizeof(p),"%d / %d",m->room_page+1,max_i(1,(c->room_count+7)/8));center(100,570,120,p,0,MUTED);
  button(222,565,82,34,">",GUI_PAGE_DOWN,start+8<c->room_count,PALE,BLUE);
 }else button(16,565,288,34,"B  Conversations",GUI_BACK,true,PALE,BLUE);
 button(16,611,288,46,"-  Microphone test",GUI_MIC,!m->busy,m->room_sel==-1 && m->view==1?PALE:BG,BLUE);
}
/* Word wrapping shares measurement and painting paths; explicit newlines and long words work. */
static int wrapped_range(const char *s,int x,int y,int width,int maxlines,int skip,bool paint,uint32_t color){
 int lines=0;const char *p=s;
 if(!*p)p="[Attachment or system event]";
 while(*p && lines<maxlines+skip){
  const char *start=p,*last_space=NULL,*end=p;int w=0;
  while(*p && *p!='\n'){
   const char *before=p;unsigned cp=printable(decode(&p));int advance=advances(1)[cp-32];
   if(w+advance>width){p=before;break;}w+=advance;end=p;if(cp==' ')last_space=before;
  }
  if(*p && *p!='\n' && last_space && last_space>start){end=last_space;p=last_space+1;}
  if(end==start && *p && *p!='\n'){decode(&p);end=p;}
  if(paint && lines>=skip){int pos=x;const char *q=start;while(q<end){unsigned cp=printable(decode(&q));glyph(pos,y+(lines-skip)*28,cp,1,color);pos+=advances(1)[cp-32];}}
  ++lines;if(*p=='\n')++p;
 }
 if(*p && paint)text(x+width-30,y+(lines-skip-1)*28,"...",1,color,30);
 return max_i(1,lines-skip);
}
static int wrapped(const char *s,int x,int y,int width,int maxlines,bool paint,uint32_t color){return wrapped_range(s,x,y,width,maxlines,0,paint,color);}
static void login(const GuiModel *m){
 rect(0,64,1280,618,BG);
 round_rect(364,90,552,550,18,WHITE);
 circle(640,146,30,BLUE);bubble_icon(624,133,WHITE);
 center(392,190,496,"Welcome to SwitchTalk",2,INK);
 center(392,233,496,"Sign in to your Nextcloud Talk server",0,MUTED);
 const char *labels[]={"Server address","Username","Password"};
 const char *values[]={m->client->server[0]?m->client->server:"https://your.nextcloud.server",m->client->username[0]?m->client->username:"Your username",m->client->app_password[0]?"Password entered":"Your account password"};
 for(int i=0;i<3;i++){
  int y=278+i*72;text(398,y,labels[i],0,MUTED,480);round_rect(396,y+27,488,38,8,BG);
  text(409,y+32,values[i],0,INK,462);if(!m->busy)add_hit(396,y+27,488,38,(GuiAction)(GUI_EDIT_SERVER+i),0);
 }
 button(396,505,488,48,m->busy?"Connecting...":"A  Sign in",GUI_CONNECT,!m->busy,BLUE,WHITE);
 text(399,566,"Sign-in is encrypted and saved on this Switch.",0,MUTED,484);
 text(399,590,"Sign out to remove your saved sign-in.",0,MUTED,484);
 const char *notice=m->client->error[0]?m->client->error:m->status;
 center(120,648,1040,notice,0,m->client->error[0]?RED:MUTED);
 footer("A Sign in     X Clear connection     Y / - Microphone test     + Exit    |    Tap a field to edit");
}
static void chat(const GuiModel *m){
 const TalkClient *c=m->client;const TalkRoom *r=&c->rooms[m->current_room];
 rect(320,64,960,80,WHITE);photo_avatar(364,103,22,r->title,BLUE,avatars_room_key(r));text(400,78,r->title,2,INK,530);
 text(401,117,"Conversation",0,MUTED,400);button(950,82,118,44,"X Refresh",GUI_REFRESH,!m->busy,PALE,BLUE);button(1082,82,168,44,"L  Call",GUI_CALL,!m->busy,BLUE,WHITE);rect(320,143,960,1,LINE);
 int bottom=558;int last=c->message_count-1-m->message_scroll;
 for(int i=last;i>=0;i--){
  const TalkMessage *msg=&c->messages[i];int n=wrapped(msg->body,0,0,718,10,false,INK),h=n*28+49;
  int top=bottom-h;
  if(top<160)break;
  photo_avatar(354,top+24,16,msg->sender,RGB(115,143,162),avatars_user_key(msg->sender_id));
  round_rect(384,top,752,h,12,WHITE);
  add_hit(384,top,752,h,GUI_MESSAGE,i);
  text(400,top+5,msg->sender,0,BLUE,710);wrapped(msg->body,400,top+32,718,10,true,INK);
  bottom=top-12;
 }
 if(!c->message_count){center(384,290,752,"No messages yet",2,INK);center(384,334,752,"Send a message to start the conversation.",1,MUTED);}
 if(m->message_scroll)text(396,148,"Earlier messages",0,MUTED,470);
 button(1160,166,88,40,"Up",GUI_SCROLL_UP,last>0,PALE,BLUE);
 button(1160,216,88,40,"Down",GUI_SCROLL_DOWN,m->message_scroll>0,PALE,BLUE);
 round_rect(344,576,904,48,14,WHITE);text(363,588,m->busy?"Please wait...":"Write a message...",1,MUTED,620);
 if(!m->busy)add_hit(344,576,698,48,GUI_COMPOSE,0);
 button(1054,580,188,40,"Y  Write",GUI_COMPOSE,!m->busy,BLUE,WHITE);
 status_bar(m->status,c->error,m->busy);
 footer("Y Write     L Call     A Read message     Up / Down Scroll     B Conversations     - Microphone     + Exit");
}
void gui_app(const GuiModel *m){
 if(!ready)return;
 snprintf(own_user,sizeof(own_user),"%s",m->view>0?(m->client->account_id[0]?m->client->account_id:m->client->username):"");
 begin(BG);header();
 if(m->view==0)login(m);
 else {
  sidebar(m);
  if(m->view==2 && m->current_room>=0 && m->current_room<m->client->room_count)chat(m);
  else {
   circle(800,278,54,PALE);bubble_icon(784,264,BLUE);
   center(356,365,888,"Your conversations, together",2,INK);
   center(356,414,888,"Select a conversation to read messages or start a call.",1,MUTED);
   button(672,484,256,46,"X  Refresh",GUI_REFRESH,!m->busy,BLUE,WHITE);
   button(672,542,256,44,"B  Sign out",GUI_BACK,true,PALE,BLUE);
   status_bar(m->status,m->client->error,m->busy);
   footer("Up / Down Choose     A Open     X Refresh     B Sign out     - Microphone test     + Exit");
  }
 }
 end();
}
int gui_message_lines(const TalkMessage *message){return wrapped(message->body,0,0,880,1000,false,INK);}
void gui_message(const TalkMessage *message,int offset){
 if(!ready)return;begin(BG);header();
 round_rect(160,92,960,552,18,WHITE);photo_avatar(206,130,18,message->sender,BLUE,avatars_user_key(message->sender_id));
 text(238,109,message->sender,2,INK,640);button(960,108,132,42,"B Back",GUI_BACK,true,PALE,BLUE);
 wrapped_range(message->body,198,174,880,13,offset,true,INK);
 int total=wrapped(message->body,0,0,880,1000,false,INK);
 button(700,576,182,42,"Up",GUI_SCROLL_UP,offset>0,PALE,BLUE);
 button(900,576,182,42,"Down",GUI_SCROLL_DOWN,offset+13<total,PALE,BLUE);
 text(198,586,"Full message",0,MUTED,460);
 footer("Up / Down Scroll message     B Return     + Exit");end();
}
void gui_notice(const char *title,const char *message){
 if(!ready)return;begin(BG);header();center(120,280,1040,title,2,INK);wrapped(message,240,342,800,7,true,MUTED);footer("+ Exit");end();
}
void gui_call(const CallState *s,uint32_t result,bool ending){
 if(!ready)return;begin(RGB(22,39,54));header();
 center(140,106,1000,s->title[0]?s->title:"Call",2,WHITE);
 center(140,150,1000,ending?"Ending call...":(s->active?"Call in progress":"Call ended"),1,RGB(173,202,219));
 photo_avatar(640,292,72,s->title,BLUE,call_avatar);
 center(160,400,960,s->status,1,WHITE);
 char info[160];if(result)snprintf(info,sizeof(info),"Unable to start call (0x%08x)",result);
 else snprintf(info,sizeof(info),"Microphone %s     Remote audio %u / 5",s->muted?"muted":"on",s->linked);
 center(160,446,960,info,1,result?RGB(255,164,164):RGB(173,202,219));
 button(374,524,250,56,s->muted?"Y  Unmute":"Y  Mute",GUI_MUTE,!ending && s->active,s->muted?RED:RGB(52,78,96),WHITE);
 button(648,524,258,56,"B  Hang up",GUI_HANGUP,!ending,RED,WHITE);
 center(150,622,980,"Keep SwitchTalk open during the call.",0,RGB(173,202,219));
 footer("Y Mute / unmute     B Hang up and return     + Hang up and exit");end();
}
void gui_mic(const MicState *m){
 if(!ready)return;begin(BG);header();
 round_rect(200,92,880,546,18,WHITE);text(236,116,"Microphone test",2,INK,600);button(934,112,110,40,"B Back",GUI_BACK,true,PALE,BLUE);
 text(238,165,"Speak into a 3.5 mm headset mic or USB-C microphone.",1,MUTED,800);
 text(238,214,"Selected input",0,MUTED,800);
 const char *name=m->active[0]?m->active:(m->device_count?m->devices[m->selected]:"No microphone detected");text(238,242,name,1,INK,800);
 char info[160];snprintf(info,sizeof(info),"%s   |   %u Hz / %u channels   |   %u inputs",m->capturing?"Capturing":"Stopped",m->rate,m->channels,m->device_count);text(238,285,info,0,MUTED,800);
 text(238,332,"Live input level",1,INK,600);char percent[24];snprintf(percent,sizeof(percent),"%u%%",m->level);text(943,332,percent,1,BLUE,90);
 round_rect(238,380,800,24,10,LINE);unsigned level=m->level>100?100:m->level;if(level)round_rect(238,380,(int)level*8,24,10,GREEN);
 snprintf(info,sizeof(info),"Peak %u%%    |    Captured buffers %llu",m->peak,(unsigned long long)m->blocks);text(238,420,info,0,MUTED,800);
 text(238,460,m->status,0,m->capturing?MUTED:RED,800);
 button(238,514,170,48,"<  Input",GUI_MIC_PREV,m->device_count>0,PALE,BLUE);
 button(420,514,194,48,m->capturing?"A  Stop":"A  Start",GUI_MIC_TOGGLE,true,BLUE,WHITE);
 button(626,514,218,48,"X  Reconnect",GUI_MIC_RESCAN,true,PALE,BLUE);
 button(856,514,182,48,"Input  >",GUI_MIC_NEXT,m->device_count>0,PALE,BLUE);
 text(238,590,"Input only. No recordings are saved or uploaded.",0,MUTED,800);
 footer("Left / Right Select input     A Start / stop capture     X Reconnect     B Return     + Exit");end();
}
GuiAction gui_hit_test(int x,int y,int *selected){
 if(selected)*selected=-1;
 for(unsigned i=hit_count;i>0;i--){const Hit *h=&hits[i-1];if(x>=h->x && y>=h->y && x<h->x+h->w && y<h->y+h->h){if(selected)*selected=h->value;return h->action;}}
 return GUI_NONE;
}
GuiAction gui_touch(const GuiModel *m,int *selected){
 (void)m;
#ifndef GUI_HEADLESS
 HidTouchScreenState state;memset(&state,0,sizeof(state));hidGetTouchScreenStates(&state,1);
 if(!state.count){touch_held=false;return GUI_NONE;}
 if(touch_held)return GUI_NONE;touch_held=true;
 return gui_hit_test((int)state.touches[0].x,(int)state.touches[0].y,selected);
#else
 (void)selected;return GUI_NONE;
#endif
}
