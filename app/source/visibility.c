/* Optional local IPC only: never starts a worker or affects messaging on failure. */
#include <switch.h>
#include <string.h>
#include <stdio.h>
#include "quick_reply.h"
#include "probe.h"
#include "visibility.h"
static Service session;
static AppletHookCookie hook;
static bool started;
static u64 last;
static char desired[128],sent[128];
static void disconnect(void){if(serviceIsActive(&session))serviceClose(&session);memset(&session,0,sizeof(session));sent[0]=0;}
static bool connect_worker(void){
 Result rc=smInitialize();if(R_FAILED(rc))return false;
 bool exists=false;SmServiceName name=smEncodeName(PROBE_PORT);
 rc=tipcDispatchInOut(smGetServiceSessionTipc(),65100,name,exists);
 Handle handle=INVALID_HANDLE;
 if(R_SUCCEEDED(rc) && exists)rc=smGetServiceOriginal(&handle,name);
 smExit();if(R_FAILED(rc)||!exists)return false;
 session=(Service){.session=handle,.own_handle=1};ProbeStatus status={0};
 rc=serviceDispatchOut(&session,PROBE_STATUS,status);
 if(R_FAILED(rc)||status.version!=PROBE_VERSION){disconnect();return false;}return true;
}
void visibility_tick(void){
 if(!started)return;
 const char *token=appletGetFocusState()==AppletFocusState_InFocus?desired:"";
 u64 now=svcGetSystemTick();if(last && now-last<19200000ULL && (!serviceIsActive(&session) || !strcmp(token,sent)))return;
 last=now;if(!serviceIsActive(&session) && !connect_worker())return;
 QRRequest request={.op=QR_VISIBLE,.arg=0};QRReply reply={0};
 snprintf(request.bytes,sizeof(request.bytes),"%s",token);
 Result rc=serviceDispatchInOut(&session,QR_RPC,request,reply);
 if(R_FAILED(rc)||reply.error){disconnect();return;}
 snprintf(sent,sizeof(sent),"%s",token);
}
void visibility_set(const char *token){snprintf(desired,sizeof(desired),"%s",token?token:"");visibility_tick();}
static void focus_changed(AppletHookType event,void *unused){(void)unused;if(event==AppletHookType_OnFocusState){last=0;visibility_tick();}}
void visibility_init(void){started=true;appletHook(&hook,focus_changed,NULL);appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleepNotify);}
void visibility_exit(void){if(!started)return;visibility_set(NULL);appletUnhook(&hook);started=false;disconnect();}
