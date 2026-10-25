#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "call.h"
#include "call_engine.h"
#include "gui.h"
static int tick,mode,stage,starts,joins,mutes,inits;static bool touch_mode;
void network_pause(bool pause){if(pause){assert(stage==0);stage=1;}else{assert(stage==3);stage=0;}}
void call_engine_init(void){assert(stage==1);inits++;}
Result call_engine_start(const CallRequest *s){assert(stage==1);assert(call_validate_request(s));starts++;return mode==2?99:0;}
void call_engine_status(CallState *s){memset(s,0,sizeof(*s));s->active=true;}
void call_engine_mute(void){mutes++;}
void call_engine_stop(void){assert(stage==1);stage=2;}
void call_engine_join(void){assert(stage==2);stage=3;joins++;}
void padInitializeDefault(PadState *p){(void)p;tick=0;}
void padUpdate(PadState *p){(void)p;tick++;}
u64 padGetButtonsDown(PadState *p){(void)p;if(touch_mode)return 0;return tick==1?HidNpadButton_Y:(mode==1?HidNpadButton_Plus:HidNpadButton_B);}
bool appletMainLoop(void){assert(tick<5);return true;}
void consoleUpdate(void *p){(void)p;}
void svcSleepThread(int64_t n){(void)n;}
int ui_test_printf(const char *f,...){(void)f;return 0;}
void gui_call(const CallState *s,uint32_t result,bool ending){(void)s;(void)result;assert(ending?stage==2:stage==1);}
GuiAction gui_touch(const GuiModel *m,int *selected){(void)m;(void)selected;return touch_mode?(tick==1?GUI_MUTE:GUI_HANGUP):GUI_NONE;}
int main(void){
 static TalkClient c;strcpy(c.server,"https://example.com");strcpy(c.username,"test");strcpy(c.app_password,"secret");
 for(mode=0;mode<3;mode++){assert(call_run(&c,"abcd1234","Test")== (mode==1));assert(stage==0);}
 assert(inits==1 && starts==3 && joins==3 && mutes==3);
 touch_mode=true;mode=0;assert(!call_run(&c,"abcd1234","Test"));assert(starts==4 && joins==4 && stage==0);
 return 0;
}
