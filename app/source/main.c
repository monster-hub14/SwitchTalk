/* SwitchTalk: Nextcloud Talk messaging and calls. */
#include <switch.h>
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "talk.h"
#include "network.h"
#include "audio_test.h"
#include "call.h"
#include "gui.h"
#include "server_url.h"
#include "saved_login.h"
#include "avatars.h"
#include "visibility.h"
#include "updater.h"

#define ROOMS_PER_PAGE GUI_ROOMS_PER_PAGE
#define MESSAGES_PER_PAGE 1

typedef enum { VIEW_LOGIN, VIEW_ROOMS, VIEW_CHAT } View;
static TalkClient client;
static bool command_busy;
static char opening_token[128],draft[1200];
static View view=VIEW_LOGIN;
static int room_sel, room_page, message_scroll, current_room;
static char statusline[240]="Enter your Nextcloud Talk connection details";

static void status(const char *s){snprintf(statusline,sizeof(statusline),"%s",s);}
static void render(void) {
    visibility_set(view==VIEW_CHAT && current_room>=0 && current_room<client.room_count?client.rooms[current_room].token:NULL);
    char update_line[240]={0};bool update_notice=view!=VIEW_CHAT && st_update_notice(update_line,sizeof(update_line));
    GuiModel model={.client=&client,.view=view,.room_sel=room_sel,.room_page=room_page,
        .message_scroll=message_scroll,.current_room=current_room,.busy=command_busy,.status=update_notice?update_line:statusline};
    gui_app(&model);
}
static bool microphone_test(void){visibility_set(NULL);return audio_test_run();}
static bool updates_panel(void){
 PadState pad;padInitializeDefault(&pad);
 while(appletMainLoop()){
  padUpdate(&pad);u64 down=padGetButtonsDown(&pad);GuiAction action=gui_touch(NULL,NULL);
  if(action==GUI_UPDATE_CHECK)down|=HidNpadButton_A;
  if(action==GUI_UPDATE_AUTO)down|=HidNpadButton_X;
  if(action==GUI_UPDATE_RESTORE)down|=HidNpadButton_Y;
  if(action==GUI_BACK)down|=HidNpadButton_B;
  if(down&HidNpadButton_Plus)return true;
  st_update_input(down);
  if(down&HidNpadButton_B)return false;
  if(st_update_tick(network_idle_for_update()))return true;
  GuiUpdateModel model;st_update_model(&model);gui_updates(&model);svcSleepThread(33000000);
 }return true;
}
static bool keyboard(char *out,size_t cap,bool password) {
    SwkbdConfig cfg;
    Result rc=swkbdCreate(&cfg,0);
    if(R_FAILED(rc)){status("Cannot open Switch software keyboard");return false;}
    if(password)swkbdConfigMakePresetPassword(&cfg);
    else swkbdConfigMakePresetDefault(&cfg);
    if(out[0])swkbdConfigSetInitialText(&cfg,out);
    char candidate[2048]={0};
    rc=swkbdShow(&cfg,candidate,sizeof(candidate));
    swkbdClose(&cfg);
    if(R_FAILED(rc)||!candidate[0])return false;
    if(strlen(candidate)>=cap){status("Text too long; nothing changed");return false;}
    memcpy(out,candidate,strlen(candidate)+1);
    memset(candidate,0,sizeof(candidate));
    return true;
}
static void enter_login(void) {
    if(!server_has_host(client.server)&&!keyboard(client.server,sizeof(client.server),false))return;
    if(!server_normalize(client.server,sizeof(client.server))){status("Enter a server hostname or HTTPS URL");return;}
    if(!client.username[0]&&!keyboard(client.username,sizeof(client.username),false))return;
    if(!client.app_password[0]&&!keyboard(client.app_password,sizeof(client.app_password),true))return;
    if(network_submit(NET_LOGIN,&client,NULL,NULL)){
        command_busy=true;client.error[0]=0;status("Connecting...");
    }
}
static void refresh(void) {
    if(command_busy)return;
    NetAction action=view==VIEW_ROOMS?NET_ROOMS:NET_HISTORY;
    const char *token=view==VIEW_CHAT?client.rooms[current_room].token:NULL;
    if(network_submit(action,&client,token,NULL)){
        command_busy=true;client.error[0]=0;status("Refreshing...");
    }
}
static void open_selected_room(void) {
    if(command_busy)return;
    if(room_sel==-1)return;
    if(!client.room_count)return;
    network_watch(NULL);
    snprintf(opening_token,sizeof(opening_token),"%s",client.rooms[room_sel].token);
    if(network_submit(NET_HISTORY,&client,opening_token,NULL)){
        command_busy=true;client.error[0]=0;status("Loading messages...");
    }
}
static void compose_message(void) {
    if(command_busy){status("Please wait for the current request");return;}
    if(!keyboard(draft,sizeof(draft),false))return;
    if(network_submit(NET_SEND,&client,client.rooms[current_room].token,draft)){
        command_busy=true;client.error[0]=0;status("Sending...");
    }

}
static bool read_message(int selected){
 if(selected<0 || selected>=client.message_count)return false;
 int offset=0;bool exit_app=false;PadState pad;padInitializeDefault(&pad);
 gui_message(&client.messages[selected],offset);
 while(appletMainLoop()){
  visibility_tick();padUpdate(&pad);u64 down=padGetButtonsDown(&pad);GuiAction a=gui_touch(NULL,NULL);
  if(a==GUI_BACK)down|=HidNpadButton_B;
  if(a==GUI_SCROLL_UP)down|=HidNpadButton_Up;
  if(a==GUI_SCROLL_DOWN)down|=HidNpadButton_Down;
  if(down&HidNpadButton_Plus){exit_app=true;break;}
  if(down&HidNpadButton_B)break;
  if(down&HidNpadButton_Up && offset>0)--offset;
  if(down&HidNpadButton_Down && offset+13<gui_message_lines(&client.messages[selected]))++offset;
  if(down)gui_message(&client.messages[selected],offset);
  svcSleepThread(33000000);
 }
 return exit_app;
}
static bool receive_update(void){
    NetAction action;bool ok;char notice[240],selected_token[128]={0};
    if(view==VIEW_ROOMS && client.room_count && room_sel>=0 && room_sel<client.room_count)
        snprintf(selected_token,sizeof(selected_token),"%s",client.rooms[room_sel].token);
    long anchor=0;
    if(view==VIEW_CHAT && message_scroll>0 && client.message_count>message_scroll)
        anchor=client.messages[client.message_count-1-message_scroll].id;
    if(!network_take(&client,&action,&ok,notice,sizeof(notice)))return false;
    if(action!=NET_POLL && action!=NET_AUTO_ROOMS)command_busy=false;
    if(ok){
        if(action==NET_LOGIN){view=VIEW_ROOMS;room_sel=-1;room_page=0;
            if(saved_login_save(&client))status("Connected. Sign-in saved on this Switch.");
            else status(saved_login_error());
        }
        else if(action==NET_HISTORY){
            if(view==VIEW_ROOMS){
                current_room=-1;
                for(int i=0;i<client.room_count;i++)if(!strcmp(client.rooms[i].token,opening_token)){current_room=i;break;}
                if(current_room<0){status("Conversation changed; refresh and select it again");return true;}
            }
            view=VIEW_CHAT;message_scroll=0;
            network_watch(client.rooms[current_room].token);
            status("Live messages connected");
        }else {if(action==NET_SEND)memset(draft,0,sizeof(draft));status(notice);}
    }else if(action==NET_LOGIN){talk_clear_password(&client);status("Could not sign in. Check the connection or enter your password again.");
    }else if(action==NET_SEND){status("Send not confirmed. Draft retained; refresh before retrying.");
    }else if(action==NET_POLL || action==NET_AUTO_ROOMS){
        /* Show retry state alongside the retained message history. */
        status(notice);
    }
    if(view==VIEW_ROOMS){
        for(int i=0;i<client.room_count;i++)if(selected_token[0] && !strcmp(client.rooms[i].token,selected_token)){room_sel=i;break;}
        if(room_sel>=client.room_count)room_sel=client.room_count?client.room_count-1:0;
        room_page=room_sel<0?0:room_sel/ROOMS_PER_PAGE;
    }
    if(anchor && action==NET_POLL){
        for(int i=0;i<client.message_count;i++)if(client.messages[i].id==anchor){message_scroll=client.message_count-1-i;break;}
    }
    int maxscroll=client.message_count-MESSAGES_PER_PAGE;
    if(maxscroll<0)maxscroll=0;
    if(message_scroll>maxscroll)message_scroll=maxscroll;
    return true;
}

int main(int argc,char **argv) {
    (void)argc;(void)argv;
    if(!gui_init()){
        consoleInit(NULL);printf("Unable to initialize graphics. Press + to exit.\n");
        PadState p;padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&p);
        while(appletMainLoop()){padUpdate(&p);if(padGetButtonsDown(&p)&HidNpadButton_Plus)break;consoleUpdate(NULL);svcSleepThread(33000000);}
        consoleExit(NULL);return 1;
    }
    gui_notice("Starting SwitchTalk","Checking installed components...");
    if(!st_update_prepare(argc>0 && argv && argv[0]?argv[0]:"")){
        gui_notice("Update recovery",st_update_error());
        PadState recovery;padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&recovery);
        while(appletMainLoop()){padUpdate(&recovery);if(padGetButtonsDown(&recovery)&HidNpadButton_Plus)break;svcSleepThread(33000000);}
        st_update_shutdown();gui_exit();return 1;
    }
    hidInitializeTouchScreen();
    padConfigureInput(1,HidNpadStyleSet_NpadStandard);
    PadState pad;padInitializeDefault(&pad);
    talk_init(&client);snprintf(client.server,sizeof(client.server),"https://");
    bool reconnect=saved_login_load(&client);
    if(!reconnect && saved_login_error()[0])status(saved_login_error());
    Result sock=socketInitializeDefault();
    if(R_FAILED(sock)) {
        char error[120];snprintf(error,sizeof(error),"Network initialization failed: 0x%x",sock);gui_notice("Connection error",error);
        while(appletMainLoop()) {
            padUpdate(&pad);
            if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;
            svcSleepThread(100000000);
        }
        st_update_shutdown();gui_exit();return 1;
    }
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK){
        socketExit();st_update_shutdown();gui_exit();return 1;
    }
    Result net=network_start();
    if(R_FAILED(net)){
        gui_notice("Connection error","Unable to start network worker. Press + to exit.");
        while(appletMainLoop()){padUpdate(&pad);if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;svcSleepThread(33000000);}
        curl_global_cleanup();socketExit();st_update_shutdown();gui_exit();return 1;
    }
    if(reconnect && network_submit(NET_LOGIN,&client,NULL,NULL)){command_busy=true;status("Signing in with saved account...");}
    visibility_init();
    unsigned image_revision=avatars_revision();
    render();st_update_boot_ok();
    char last_update_line[240]={0};
    while(appletMainLoop()) {
        visibility_tick();
        bool updated=receive_update();unsigned current_revision=avatars_revision();
        if(updated || image_revision!=current_revision){image_revision=current_revision;render();}
        padUpdate(&pad);
        u64 down=padGetButtonsDown(&pad);
        GuiModel touch_model={.client=&client,.view=view,.room_sel=room_sel,.room_page=room_page,.message_scroll=message_scroll,.current_room=current_room,.busy=command_busy,.status=statusline};
        int selected=-1;GuiAction action=gui_touch(&touch_model,&selected);
        if(action==GUI_UPDATES || (down&HidNpadButton_ZL)){
            if(view==VIEW_CHAT || command_busy)status("Return to Conversations and wait, then press ZL for Updates.");
            else if(updates_panel())break;
            render();continue;
        }
        bool update_idle=view!=VIEW_CHAT && !command_busy && !client.error[0] && network_idle_for_update();
        GuiUpdateModel update_state;st_update_model(&update_state);
        if(update_idle && update_state.phase==3 && update_state.countdown>=0 && (down&HidNpadButton_B)){st_update_input(HidNpadButton_B);down&=~HidNpadButton_B;}
        if(st_update_tick(update_idle && !down && action==GUI_NONE))break;
        char update_line[240]={0};st_update_notice(update_line,sizeof(update_line));
        if(view!=VIEW_CHAT && strcmp(last_update_line,update_line)){snprintf(last_update_line,sizeof(last_update_line),"%s",update_line);render();}
        if(action==GUI_MESSAGE){if(read_message(selected))break;render();continue;}
        if(action==GUI_SELECT_ROOM && !command_busy){room_sel=selected;view=VIEW_ROOMS;room_page=room_sel/ROOMS_PER_PAGE;open_selected_room();render();continue;}
        if(action>=GUI_EDIT_SERVER && action<=GUI_EDIT_PASSWORD && !command_busy){
            if(action==GUI_EDIT_SERVER)keyboard(client.server,sizeof(client.server),false);
            else if(action==GUI_EDIT_USER)keyboard(client.username,sizeof(client.username),false);
            else keyboard(client.app_password,sizeof(client.app_password),true);
            render();continue;
        }
        if(action==GUI_PAGE_UP && view==VIEW_ROOMS && room_page>0){--room_page;room_sel=room_page*ROOMS_PER_PAGE;render();continue;}
        if(action==GUI_PAGE_DOWN && view==VIEW_ROOMS && (room_page+1)*ROOMS_PER_PAGE<client.room_count){++room_page;room_sel=room_page*ROOMS_PER_PAGE;render();continue;}
        if(action==GUI_CONNECT)down|=HidNpadButton_A;
        if(action==GUI_EDIT)down|=HidNpadButton_X;
        if(action==GUI_MIC)down|=HidNpadButton_Minus;
        if(action==GUI_REFRESH)down|=HidNpadButton_X;
        if(action==GUI_BACK)down|=HidNpadButton_B;
        if(action==GUI_COMPOSE)down|=HidNpadButton_Y;
        if(action==GUI_CALL)down|=HidNpadButton_L;
        if(action==GUI_SCROLL_UP)down|=HidNpadButton_Up;
        if(action==GUI_SCROLL_DOWN)down|=HidNpadButton_Down;
        if(!down){svcSleepThread(33000000);continue;}
        if(down&HidNpadButton_Plus)break;
        if(down&HidNpadButton_Minus){
            if(command_busy)status("Wait for the current request, then open Microphone test");
            else if(microphone_test())break;
            render();continue;
        }
        if(view==VIEW_LOGIN) {
            if(down&HidNpadButton_X && !command_busy){talk_clear_password(&client);bool forgot=saved_login_forget();snprintf(client.server,sizeof(client.server),"https://");client.username[0]=0;client.error[0]=0;status(forgot?"Connection cleared. Press A to enter details.":saved_login_error());}
            else if(down&HidNpadButton_Y && !command_busy){if(microphone_test())break;}
            else if(down&HidNpadButton_A && !command_busy)enter_login();
        } else if(view==VIEW_ROOMS) {
            if(down&HidNpadButton_B){network_signout();memset(draft,0,sizeof(draft));command_busy=false;talk_clear_password(&client);client.error[0]=0;view=VIEW_LOGIN;bool forgot=saved_login_forget();avatars_clear();status(forgot?"Signed out. Saved sign-in removed.":saved_login_error());}
            else if(down&HidNpadButton_X)refresh();
            if(down&HidNpadButton_Up && room_sel>=0)--room_sel;
            if(down&HidNpadButton_Down && room_sel<client.room_count-1)++room_sel;
            room_page=room_sel<0?0:room_sel/ROOMS_PER_PAGE;
            if(view==VIEW_ROOMS && down&HidNpadButton_A){
                if(room_sel==-1 && !command_busy){if(microphone_test())break;}
                else open_selected_room();
            }
        } else {
            if(down&HidNpadButton_B){network_watch(NULL);memset(draft,0,sizeof(draft));command_busy=false;view=VIEW_ROOMS;status("Select a conversation");}
            else if(down&HidNpadButton_X)refresh();
            else if(down&HidNpadButton_L && !command_busy){
                visibility_set(NULL);
                gui_call_room(&client.rooms[current_room]);
                if(call_run(&client,client.rooms[current_room].token,client.rooms[current_room].title))break;
                status("Call ended.");
            }
            else if(down&HidNpadButton_A){if(read_message(client.message_count-1-message_scroll))break;}
            else if(down&HidNpadButton_Y)compose_message();
            else if(down&HidNpadButton_Up && message_scroll<client.message_count-MESSAGES_PER_PAGE)++message_scroll;
            else if(down&HidNpadButton_Down && message_scroll>0)--message_scroll;
        }
        render();
    }
    visibility_exit();
    status("Closing network connections...");render();
    st_update_shutdown();
    network_stop();
    talk_clear_password(&client);
    curl_global_cleanup();socketExit();gui_exit();
    return 0;
}
