/* Render the actual native GUI into guarded host memory; verify touch routing. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui.h"
#include "avatars.h"
#include "fixtures/avatar_fixtures.h"
bool avatar_download(const TalkClient *t,const AvatarKey *key,unsigned char **data,size_t *size,AvatarCancel cancel,void *ctx){
 (void)t;(void)key;(void)cancel;(void)ctx;*size=sizeof(avatar_png);*data=malloc(*size);assert(*data);memcpy(*data,avatar_png,*size);return true;
}
static uint32_t memory[1280*720+2];
static uint32_t login_pixels[1280*720];
static TalkClient client;
static void snapshot(const char *name){
 char file[2048];snprintf(file,sizeof(file),"%s/%s.ppm",GUI_PREVIEW_DIR,name);
 FILE *f=fopen(file,"wb");assert(f);fprintf(f,"P6\n1280 720\n255\n");
 for(int i=1;i<=1280*720;i++){uint32_t c=memory[i];unsigned char rgb[3]={(unsigned char)c,(unsigned char)(c>>8),(unsigned char)(c>>16)};fwrite(rgb,1,3,f);}fclose(f);
 assert(memory[0]==0x12345678 && memory[1280*720+1]==0x87654321);
}
int main(void){
 memory[0]=0x12345678;memory[1280*720+1]=0x87654321;
 avatars_init();gui_test_buffer(memory+1,1280);
 strcpy(client.server,"https://cloud.example.com");strcpy(client.username,"Jordan");strcpy(client.app_password,"SECRET-NOT-RENDERED");
 GuiModel m={.client=&client,.view=0,.room_sel=0,.current_room=0,.status="Ready to sign in"};
 gui_app(&m);assert(gui_hit_test(500,520,NULL)==GUI_CONNECT);assert(gui_hit_test(500,390,NULL)==GUI_EDIT_USER);snapshot("preview-sign-in");memcpy(login_pixels,memory+1,sizeof(login_pixels));strcpy(client.app_password,"DIFFERENT-SECRET");gui_app(&m);assert(!memcmp(login_pixels,memory+1,sizeof(login_pixels)));
 m.busy=true;gui_app(&m);assert(gui_hit_test(500,520,NULL)==GUI_NONE);m.busy=false;
 const char *rooms[]={"Family","Weekend plans","Switch gaming","Project ideas","Friends","Notes to self","Photography","Travel plans","A ninth conversation"};
 client.room_count=9;for(int i=0;i<9;i++){snprintf(client.rooms[i].title,sizeof(client.rooms[i].title),"%s",rooms[i]);client.rooms[i].unread=i==2;}
 m.view=1;m.status="Connected. Select a conversation.";gui_app(&m);int chosen=-1;assert(gui_hit_test(130,146,&chosen)==GUI_SELECT_ROOM && chosen==0);assert(gui_hit_test(260,580,NULL)==GUI_PAGE_DOWN);snapshot("preview-conversations");
 m.room_page=1;gui_app(&m);assert(gui_hit_test(130,146,&chosen)==GUI_SELECT_ROOM && chosen==8);assert(gui_hit_test(130,200,NULL)==GUI_NONE);m.room_page=0;
 client.message_count=4;
 strcpy(client.messages[0].sender,"Jordan");strcpy(client.messages[0].body,"Hey everyone! Who is around for a call later?");
 strcpy(client.messages[1].sender,"Alex");strcpy(client.messages[1].body,"I am! Just finishing a game. How does 8 pm sound?");
 strcpy(client.messages[2].sender,"Sam");strcpy(client.messages[2].body,"Sounds good. I can bring my headset too.");
 strcpy(client.messages[3].sender,"Jordan");strcpy(client.messages[3].body,"Perfect. I will start the call here when we are ready.");
 client.rooms[0].custom_avatar=true;strcpy(client.rooms[0].token,"family");strcpy(client.rooms[0].avatar_version,"1");
 for(int i=0;i<4;i++)snprintf(client.messages[i].sender_id,sizeof(client.messages[i].sender_id),"user%d",i);
 m.view=2;m.status="Live messages connected";gui_app(&m);while(avatars_pending())avatars_step(&client,NULL,NULL);gui_app(&m);assert(gui_hit_test(1150,103,NULL)==GUI_CALL);assert(gui_hit_test(650,600,NULL)==GUI_COMPOSE);snapshot("preview-chat");
 m.busy=true;gui_app(&m);assert(gui_hit_test(1150,103,NULL)==GUI_NONE);assert(gui_hit_test(650,600,NULL)==GUI_NONE);m.busy=false;
 memset(client.messages[3].body,'W',1000);client.messages[3].body[1000]=0;strcpy(client.messages[2].body,"Line one\nLine two\nUTF-8: caf\xc3\xa9 \xf0\x9f\x8e\xae");gui_app(&m);snapshot("preview-long-message");gui_message(&client.messages[3],0);assert(gui_message_lines(&client.messages[3])>13);assert(gui_hit_test(1000,595,NULL)==GUI_SCROLL_DOWN);snapshot("preview-full-message");
 m.message_scroll=3;gui_app(&m);assert(gui_hit_test(1200,180,NULL)==GUI_NONE);assert(gui_hit_test(1200,230,NULL)==GUI_SCROLL_DOWN);
 client.message_count=0;gui_app(&m);snapshot("preview-empty-chat");
 strcpy(client.error,"HTTPS certificate rejected before login. Check server hostname and Switch date.");gui_app(&m);snapshot("preview-error");client.error[0]=0;
 gui_call_room(&client.rooms[0]);CallState state={.active=1,.linked=2};strcpy(state.title,"Family");strcpy(state.status,"Connected. Microphone audio is being sent.");gui_call(&state,0,false);assert(gui_hit_test(500,550,NULL)==GUI_MUTE);assert(gui_hit_test(750,550,NULL)==GUI_HANGUP);snapshot("preview-call");
 gui_call(&state,0,true);assert(gui_hit_test(500,550,NULL)==GUI_NONE);
 MicState mic={.device_count=1,.capturing=true,.rate=48000,.channels=1,.level=62,.peak=78,.blocks=1254};strcpy(mic.active,"Headset microphone (3.5 mm)");strcpy(mic.status,"Microphone input is active. Speak to see the meter move.");gui_mic(&mic);assert(gui_hit_test(950,130,NULL)==GUI_BACK);assert(gui_hit_test(500,540,NULL)==GUI_MIC_TOGGLE);snapshot("preview-microphone");
 assert(gui_hit_test(-1,0,NULL)==GUI_NONE);assert(gui_hit_test(1280,720,NULL)==GUI_NONE);
 avatars_shutdown();gui_exit();return 0;
}
