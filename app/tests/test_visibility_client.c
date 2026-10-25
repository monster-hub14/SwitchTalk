#include <switch.h>
#include "quick_reply.h"
#include "probe.h"
#include "visibility.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static u64 now=1;static int focus=1,checks,sends;static bool exists;static unsigned version=PROBE_VERSION;static char token[128];static void(*callback)(AppletHookType,void*);
bool serviceIsActive(Service*s){return s->session!=0;}void serviceClose(Service*s){s->session=0;}
Result smInitialize(void){return 0;}void smExit(void){}SmServiceName smEncodeName(const char*s){(void)s;return 0;}int smGetServiceSessionTipc(void){return 0;}
Result smGetServiceOriginal(Handle*h,SmServiceName n){(void)n;*h=1;return 0;}u64 svcGetSystemTick(void){return now;}int appletGetFocusState(void){return focus;}
void appletHook(AppletHookCookie*c,void(*f)(AppletHookType,void*),void*p){(void)c;(void)p;callback=f;}void appletUnhook(AppletHookCookie*c){(void)c;callback=0;}void appletSetFocusHandlingMode(int m){assert(m==AppletFocusHandlingMode_SuspendHomeSleepNotify);}
Result mock_has(bool*b){checks++;*b=exists;return 0;}Result mock_status(void*p){ProbeStatus*s=p;s->version=version;return 0;}
Result mock_visible(void*a,void*b){QRRequest*q=a;QRReply*r=b;assert(q->op==QR_VISIBLE && q->arg==0);sends++;strcpy(token,q->bytes);r->error=0;return 0;}
int main(void){
 visibility_set("Minecraft");assert(!checks);visibility_init();visibility_tick();assert(checks==1&&!sends);
 for(int i=0;i<30;i++)visibility_tick();assert(checks==1);
 now+=19200000;exists=true;visibility_tick();assert(sends==1&&!strcmp(token,"Minecraft"));
 visibility_set("Other");assert(sends==2&&!strcmp(token,"Other"));
 focus=3;callback(0,0);assert(sends==3&&!token[0]);
 focus=1;callback(0,0);assert(sends==4&&!strcmp(token,"Other"));
 visibility_set(0);assert(sends==5&&!token[0]);visibility_exit();assert(!callback);
 now+=19200000;version=9;visibility_init();visibility_set("Minecraft");assert(sends==5);visibility_exit();
 puts("Visibility client: missing/old worker, retry throttle, room change, focus loss/resume and exit: PASS");return 0;
}
