#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include "quick_reply.h"
#include "probe.h"
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <map>
#include "reply_editor.h"

namespace {
constexpr unsigned PAGE=6;
struct Client {
 Service service{};std::string error;
 void close(){if(serviceIsActive(&service))serviceClose(&service);service={};}
 bool connect(bool start){
  close();Result rc=smInitialize();if(R_FAILED(rc)){error="SM access failed";return false;}
  bool exists=false;SmServiceName name=smEncodeName(PROBE_PORT);rc=tipcDispatchInOut(smGetServiceSessionTipc(),65100,name,exists);
  if(R_SUCCEEDED(rc) && !exists && start){rc=pmshellInitialize();if(R_SUCCEEDED(rc)){NcmProgramLocation loc={};loc.program_id=PROBE_ID;loc.storageID=NcmStorageId_None;u64 pid=0;rc=pmshellLaunchProgram(0,&loc,&pid);pmshellExit();}
   if(R_SUCCEEDED(rc))for(int i=0;i<40;i++){rc=tipcDispatchInOut(smGetServiceSessionTipc(),65100,name,exists);if(R_FAILED(rc)||exists)break;svcSleepThread(50000000);}}
  Handle h=INVALID_HANDLE;if(R_SUCCEEDED(rc) && exists)rc=smGetServiceOriginal(&h,name);else if(R_SUCCEEDED(rc))rc=KERNELRESULT(NotFound);smExit();
  if(R_FAILED(rc)){char s[120];snprintf(s,sizeof(s),"Worker unavailable: 0x%08x. A starts/reconnects.",rc);error=s;return false;}
  service={};service.session=h;service.own_handle=1;ProbeStatus status{};rc=serviceDispatchOut(&service,PROBE_STATUS,status);
  if(R_FAILED(rc)||status.version!=PROBE_VERSION){close();error="Worker version mismatch. Replace both files and reboot.";return false;}error.clear();return true;
 }
 bool call(QRRequest q,QRReply &reply){if(!serviceIsActive(&service)){error="Worker disconnected. A reconnects.";return false;}
  Result rc=serviceDispatchInOut(&service,QR_RPC,q,reply);if(R_FAILED(rc)){close();char s[80];snprintf(s,sizeof(s),"Worker connection lost: 0x%08x",rc);error=s;return false;}
  if(reply.error){error=reply.error==QR_BUSY?"Worker busy; try again shortly":reply.error==QR_NOT_READY?"Worker is not ready yet":"Worker rejected this request";return false;}return true;
 }
 bool read(unsigned op,unsigned arg,void *data,size_t size){
  for(int retry=0;retry<3;retry++){unsigned revision=0;bool changed=false;for(size_t offset=0;offset<size;offset+=QR_DATA){QRRequest q{};q.op=op;q.arg=arg;q.offset=offset;QRReply reply{};if(!call(q,reply))return false;if(reply.total!=size){error="Unexpected worker data size";return false;}if(offset && revision!=reply.revision){changed=true;break;}revision=reply.revision;memcpy((char*)data+offset,reply.bytes,std::min(size-offset,size_t(QR_DATA)));}if(!changed)return true;}
  error="Conversation changed; press Y to refresh";return false;
 }
 void viewing(const std::string &token,bool shown=true){if(!serviceIsActive(&service))return;QRRequest q{};q.op=QR_VISIBLE;q.arg=1;q.size=shown?1:0;snprintf(q.bytes,sizeof(q.bytes),"%s",token.c_str());QRReply reply{};call(q,reply);}
bool send(const std::string &token,const std::string &text,unsigned &job,bool &ambiguous){ambiguous=false;
  QRRequest q{};q.op=QR_DRAFT_BEGIN;snprintf(q.bytes,sizeof(q.bytes),"%s",token.c_str());QRReply r{};if(!call(q,r))return false;unsigned draft=r.total;
  for(size_t off=0;off<text.size();off+=sizeof(q.bytes)){q={};q.op=QR_DRAFT_WRITE;q.arg=draft;q.offset=off;q.size=std::min(text.size()-off,sizeof(q.bytes));memcpy(q.bytes,text.data()+off,q.size);if(!call(q,r))return false;}
  q={};q.op=QR_SEND;q.arg=draft;ambiguous=true;if(!call(q,r)){error="Send not confirmed. Review chat before retrying.";return false;}job=r.total;return true;
 }
 bool open(const std::string &token){QRRequest q{};q.op=QR_OPEN;snprintf(q.bytes,sizeof(q.bytes),"%s",token.c_str());QRReply reply{};return call(q,reply);}
} client;
struct Line {std::string text;bool heading;};
struct Model {
bool composing=false,sending=false,uncertain=false,reviewRequested=false;unsigned sendJob=0;uint64_t sendInstance=0;
 ReplyEditor editor{};std::map<std::string,std::string> drafts;std::string submittedToken,submittedText;
 std::string &draft(){return drafts[token];}
 void compose(){if(!chat)return;if(drafts.size()>=16 && !drafts.count(token)){notice="Draft limit reached; finish an existing draft first";return;}composing=true;editor={};notice=sending?"Sending...":"Choose keys with D-pad + A, or tap. Select Send when ready.";}
 void edit(char ch){char text[QR_TEXT+1];snprintf(text,sizeof(text),"%s",draft().c_str());if(reply_append(text,sizeof(text),ch))draft()=text;else notice="Reply limit is 512 bytes";}
 void eraseChar(){char text[QR_TEXT+1];snprintf(text,sizeof(text),"%s",draft().c_str());reply_delete(text);draft()=text;}
 void submit(){if(sending)return;if(uncertain){notice="Close keyboard (B), refresh (Y), and check chat before retrying.";return;}
  if(draft().find_first_not_of(" \t\r\n")==std::string::npos){notice="Type a message first";return;}
  submittedToken=token;submittedText=draft();sendInstance=meta.instance;
  bool ambiguous=false;if(!client.send(token,submittedText,sendJob,ambiguous)){notice=client.error;uncertain=ambiguous;return;}
  sending=true;notice="Sending...";
 }
 void editorKey(){if(sending)return;if(editor.row<4){edit(reply_character(&editor));return;}
  switch(editor.col){case 0:edit(' ');break;case 1:eraseChar();break;case 2:editor.upper=!editor.upper;break;case 3:editor.symbols=!editor.symbols;break;case 4:submit();break;}}
 bool visible=false,chat=false,wantRooms=true;unsigned page=0,selected=0,total=0,loadedJob=0;int scroll=0,viewWidth=350,viewHeight=430;uint64_t lastMeta=0,lastHistory=0;
 QRMeta meta{};std::vector<QRRoom> rooms;std::vector<Line> lines;std::string token,title,notice="A starts or reconnects to the worker";
 void loadRooms(){if(!client.read(QR_META,0,&meta,sizeof(meta))){notice=client.error;return;}total=meta.rooms;if(page*PAGE>=total)page=0;rooms.clear();for(unsigned i=page*PAGE;i<std::min(total,(page+1)*PAGE);i++){QRRoom room{};if(!client.read(QR_ROOM,i,&room,sizeof(room))){notice=client.error;break;}rooms.push_back(room);}if(selected>=rooms.size())selected=0;notice=meta.notice;wantRooms=false;}
 void loadMessages(){bool bottom=scroll>=std::max(0,int(lines.size())*24+16-viewHeight)-30;lines.clear();for(unsigned i=0;i<std::min(meta.messages,unsigned(QR_MESSAGES));i++){QRMessage msg{};if(!client.read(QR_MESSAGE,i,&msg,sizeof(msg))){notice=client.error;return;}lines.push_back({msg.sender[0]?msg.sender:"Someone",true});auto body=tsl::wrapText(msg.body,float(viewWidth-12),"auto",false,"",0,18);for(auto &s:body)lines.push_back({s,false});lines.push_back({"",false});}if(bottom)scroll=std::max(0,int(lines.size())*24+16-viewHeight);loadedJob=meta.job_id;notice=meta.notice;}
 void openRoom(unsigned index){if(index>=rooms.size())return;if(!client.open(rooms[index].token)){notice=client.error;return;}token=rooms[index].token;title=rooms[index].title;composing=false;uncertain=false;chat=true;client.viewing(token);lines.clear();scroll=0;loadedJob=0;lastHistory=svcGetSystemTick();notice="Loading recent messages...";}
 void refresh(){if(sending){notice="Sending...";return;}if(chat){reviewRequested=uncertain;if(client.open(token)){lastHistory=svcGetSystemTick();notice="Refreshing...";}else notice=client.error;}else loadRooms();}
 void update(){if(!visible)return;uint64_t now=svcGetSystemTick();if(now-lastMeta<19200000/3)return;lastMeta=now;client.viewing(chat?token:"");if(!serviceIsActive(&client.service)){if(sending){sending=false;uncertain=true;}notice="Worker disconnected. B to rooms, then A reconnects.";return;}
  if(!client.read(QR_META,0,&meta,sizeof(meta))){notice=client.error;if(sending){sending=false;uncertain=true;notice="Connection lost. Draft kept; refresh before retrying.";}return;}
  if(sending){
   if(meta.instance!=sendInstance || meta.job_id!=sendJob){sending=false;uncertain=true;notice="Worker changed. Draft kept; check chat before retrying.";}
   else if(meta.state==QR_SENT){sending=false;if(drafts[submittedToken]==submittedText)drafts.erase(submittedToken);composing=false;notice="Sent";loadMessages();lastHistory=now;}
   else if(meta.state==QR_FAILED || meta.state==QR_OFFLINE){sending=false;uncertain=true;notice="Send not confirmed. Draft kept; B then Y to check chat.";}
  }
  if(meta.state==QR_OFFLINE){drafts.clear();composing=false;sending=false;uncertain=false;rooms.clear();lines.clear();notice=meta.notice;return;}
  if(chat){if(strcmp(meta.token,token.c_str()))return;if(meta.state==QR_READY && meta.job_id!=loadedJob){loadMessages();if(reviewRequested){uncertain=false;reviewRequested=false;}}else if(meta.state==QR_FAILED && !uncertain)notice=meta.notice;
   if(!composing && !sending && !uncertain && meta.state!=QR_PENDING && now-lastHistory>19200000ULL*5)refresh();
  }else if(wantRooms || rooms.empty() || now-lastHistory>19200000ULL*8){loadRooms();lastHistory=now;}
 }
 void key(u64 keys){if(keys&HidNpadButton_Plus){tsl::Overlay::get()->hide();return;}
  if(composing){if(keys&HidNpadButton_B){if(!sending && draft().empty())drafts.erase(token);composing=false;return;}if(sending)return;
   if(keys&HidNpadButton_Up)reply_move(&editor,0,-1);if(keys&HidNpadButton_Down)reply_move(&editor,0,1);
   if(keys&HidNpadButton_Left)reply_move(&editor,-1,0);if(keys&HidNpadButton_Right)reply_move(&editor,1,0);
   if(keys&HidNpadButton_X)eraseChar();if(keys&HidNpadButton_Y)edit(' ');
   if(keys&HidNpadButton_L)editor.upper=!editor.upper;if(keys&HidNpadButton_R)editor.symbols=!editor.symbols;
   if(keys&HidNpadButton_A)editorKey();return;
  }
  if(chat && (keys&(HidNpadButton_A|HidNpadButton_X))){compose();return;}if(keys&HidNpadButton_B){if(chat){client.viewing("");chat=false;wantRooms=true;notice="Recent conversations";}else tsl::goBack();return;}
  if(keys&HidNpadButton_Y){refresh();return;}if(!chat){if(keys&HidNpadButton_A){if(!serviceIsActive(&client.service)){if(client.connect(true)){wantRooms=true;loadRooms();}else notice=client.error;}else if(rooms.empty())loadRooms();else openRoom(selected);return;}
   if(keys&HidNpadButton_Down){if(!rooms.empty())selected=(selected+1)%rooms.size();}if(keys&HidNpadButton_Up){if(!rooms.empty())selected=(selected+rooms.size()-1)%rooms.size();}
   if((keys&HidNpadButton_Right) && (page+1)*PAGE<total){page++;selected=0;loadRooms();}if((keys&HidNpadButton_Left)&&page){page--;selected=0;loadRooms();}
  }else{if(keys&HidNpadButton_Down)scroll=std::min(std::max(0,int(lines.size())*24+16-viewHeight),scroll+48);if(keys&HidNpadButton_Up)scroll=std::max(0,scroll-48);}
 }
} model;
class Canvas: public tsl::elm::Element {
public:
 tsl::elm::Element *requestFocus(tsl::elm::Element*,tsl::FocusDirection) override{return this;}
 // OverlayFrame assigns the inset content bounds before invalidate() calls
 // layout with its own full-frame bounds. Preserve the assigned rectangle.
 void layout(u16,u16,u16,u16) override{model.viewWidth=getWidth();model.viewHeight=std::max(0,int(getHeight())-120);}
 void draw(tsl::gfx::Renderer *r) override{
  int x=getX(),y=getY(),w=getWidth(),h=getHeight();auto white=tsl::Color{15,15,15,15},gray=tsl::Color{10,10,10,15},blue=tsl::Color{4,12,15,15};
  if(model.composing){
   r->drawString(model.title,false,x,y+22,21,white,w);
   auto preview=tsl::wrapText(model.draft().empty()?"Type your reply...":model.draft(),float(w-12),"auto",false,"",0,18);
   r->drawRect(x,y+34,w,102,tsl::Color{1,2,3,15});size_t first=preview.size()>4?preview.size()-4:0;
   for(size_t i=first;i<preview.size();i++)r->drawString(preview[i],false,x+6,y+56+int(i-first)*23,18,white,w-12);
   char count[72];snprintf(count,sizeof(count),"%zu / 512   %s   %s",model.draft().size(),model.editor.upper?"ABC":"abc",model.editor.symbols?"Symbols":"Letters");r->drawString(count,false,x,y+162,16,gray,w);
   for(unsigned row=0;row<5;row++){unsigned cols=reply_columns(row);int cell=w/cols;for(unsigned col=0;col<cols;col++){
    int py=y+(row==4?370:190+int(row)*42);bool active=row==model.editor.row && col==model.editor.col;
    r->drawRect(x+col*cell+1,py,cell-3,38,active?tsl::Color{1,5,7,15}:tsl::Color{2,2,2,15});
    std::string label;if(row==4){static const char *actions[]={"Space","Del","Shift","Sym","Send"};label=actions[col];}else{ReplyEditor e=model.editor;e.row=row;e.col=col;label=std::string(1,reply_character(&e));}
    r->drawString(label,false,x+col*cell+5,py+25,row==4?16:20,active?blue:white,cell-7);
   }}
   r->drawString("A key   X delete   Y space",false,x,y+440,16,gray,w);
   r->drawString("L shift   R symbols   B back   + hide",false,x,y+463,15,gray,w);
   auto status=tsl::wrapText(model.notice,float(w),"auto",false,"",0,16);for(size_t i=0;i<std::min(status.size(),size_t(2));i++)r->drawString(status[i],false,x,y+h-38+int(i)*19,16,gray,w);
   return;
  }
  r->drawString(model.chat?"< Back       A / X: Reply":"QUICK REPLY",false,x,y+20,17,blue,w);
  r->drawString(model.chat?model.title:"Recent conversations",false,x,y+49,22,white,w);
  if(model.chat){r->enableScissoring(x,y+61,w,h-120);for(size_t i=0;i<model.lines.size();i++){int py=y+84+int(i)*24-model.scroll;if(py<y+60||py>y+h-58)continue;r->drawString(model.lines[i].text,false,x+4,py,model.lines[i].heading?19:18,model.lines[i].heading?blue:white,w-8);}r->disableScissoring();
  }else{for(size_t i=0;i<model.rooms.size();i++){int py=y+65+int(i)*64;if(i==model.selected)r->drawRect(x,py,w,60,tsl::Color{1,4,6,15});r->drawString(model.rooms[i].title,false,x+8,py+24,20,white,w-16);r->drawString(model.rooms[i].preview,false,x+8,py+47,16,gray,w-16);}
   if(model.rooms.empty()){r->drawRect(x+4,y+85,w-8,50,tsl::Color{1,4,6,15});r->drawString("A / tap: start or reconnect",false,x+10,y+115,19,white,w-20);}
   char pages[72];snprintf(pages,sizeof(pages),"Page %u / %u   Left/Right changes page",model.page+1,std::max(1u,(model.total+PAGE-1)/PAGE));r->drawString(pages,false,x,y+h-65,15,gray,w);
  }
  auto status=tsl::wrapText(model.notice,float(w),"auto",false,"",0,16);int sy=y+h-38;for(size_t i=0;i<std::min(status.size(),size_t(2));i++)r->drawString(status[i],false,x,sy+int(i)*19,16,gray,w);
 }
 bool handleInput(u64 down,u64,const HidTouchState&,HidAnalogStickState,HidAnalogStickState) override{if(down){model.key(down);return true;}return false;}
 bool onTouch(tsl::elm::TouchEvent event,s32 x,s32 y,s32 prevX,s32 prevY,s32 initialX,s32 initialY) override{
  (void)prevX;(void)initialX;
  if(model.composing){if(event==tsl::elm::TouchEvent::Release && abs(y-initialY)<=18 && !model.sending){int rx=x-getX(),ry=y-getY();if(rx>=0 && rx<getWidth()){
   int row=ry>=370&&ry<408?4:ry>=190&&ry<358?(ry-190)/42:-1;
   if(row>=0){model.editor.row=row;model.editor.col=std::min(unsigned(rx/(getWidth()/reply_columns(row))),reply_columns(row)-1);model.editorKey();}
  }}return true;}
  if(model.chat && event==tsl::elm::TouchEvent::Scroll){model.scroll=std::clamp(model.scroll+prevY-y,0,std::max(0,int(model.lines.size())*24+16-model.viewHeight));return true;}
  if(event!=tsl::elm::TouchEvent::Release || abs(y-initialY)>18)return true;
  int rx=x-getX(),ry=y-getY();if(rx<0||rx>getWidth())return true;if(ry<58){if(model.chat)model.key(rx>getWidth()/2?HidNpadButton_X:HidNpadButton_B);return true;}
  if(!model.chat){if(model.rooms.empty() && ry>=85 && ry<=135){model.key(HidNpadButton_A);return true;}int row=(ry-65)/64;if(ry>=65 && row>=0 && row<int(model.rooms.size()))model.openRoom(row);else if(ry>getHeight()-85)model.key(rx<getWidth()/2?HidNpadButton_Left:HidNpadButton_Right);}return true;
 }
};
class QuickGui: public tsl::Gui {
public:tsl::elm::Element *createUI() override{auto *f=new tsl::elm::OverlayFrame("SwitchTalk","Monster Review / 1.1.0");f->setContent(new Canvas);return f;}
 // libultrahand routes B directly to Gui before the focused Canvas.
 // Consume only B here; other keys are already handled by Canvas.
 bool handleInput(u64 down,u64,const HidTouchState&,HidAnalogStickState,HidAnalogStickState) override{
  if(down&HidNpadButton_B){model.key(HidNpadButton_B);return true;}return false;
 }
 void update() override{model.update();}
};
class QuickOverlay: public tsl::Overlay {
public:void initServices() override{client.connect(false);}void exitServices() override{client.viewing("",false);client.close();}
 void onShow() override{model.visible=true;model.wantRooms=true;model.lastMeta=0;}
 void onHide() override{client.viewing("",false);model.visible=false;}
 std::unique_ptr<tsl::Gui> loadInitialGui() override{return initially<QuickGui>();}
};
}
int main(int argc,char **argv){return tsl::loop<QuickOverlay>(argc,argv);}
