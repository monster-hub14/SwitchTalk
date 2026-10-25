#include <switch.h>
#include <stdio.h>
#include <string.h>
#include "call.h"
#include "call_engine.h"
#include "network.h"
#include "gui.h"
bool call_run(const TalkClient *client,const char *token,const char *title){
 static bool initialized;
 bool exit_app=false;
 network_pause(true);
 if(!initialized){call_engine_init();initialized=true;}
 CallRequest start={.version=CALL_VERSION};
 snprintf(start.server,sizeof(start.server),"%s",client->server);
 snprintf(start.username,sizeof(start.username),"%s",client->username);
 snprintf(start.password,sizeof(start.password),"%s",client->app_password);
 snprintf(start.room,sizeof(start.room),"%s",token);
 snprintf(start.title,sizeof(start.title),"%s",title?title:"");
 Result rc=call_engine_start(&start);call_erase(&start,sizeof(start));
 PadState pad;padInitializeDefault(&pad);
 while(appletMainLoop()){
  CallState state;call_engine_status(&state);
  padUpdate(&pad);u64 down=padGetButtonsDown(&pad);
  GuiAction action=gui_touch(NULL,NULL);
  if(action==GUI_MUTE)down|=HidNpadButton_Y;
  if(action==GUI_HANGUP)down|=HidNpadButton_B;
  if(down&HidNpadButton_Plus){exit_app=true;break;}
  if(down&HidNpadButton_B)break;
  if(down&HidNpadButton_Y)call_engine_mute();
  gui_call(&state,rc,false);svcSleepThread(100000000);
 }
 call_engine_stop();
 CallState final;call_engine_status(&final);gui_call(&final,rc,true);
 call_engine_join();network_pause(false);return exit_app;
}
