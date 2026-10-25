#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "probe.h"
#ifndef BASELINE
#define BASELINE "sdmc:/switch/SwitchTalkAlertsTest/baseline.bin"
#endif
static ProbeStatus current,baseline;
static char note[256]="A starts the tiny worker; X only checks it.";
static bool available,have_baseline;
static const char *query_stage="port connection";
static Result connect(Service *s){
 query_stage="SM service lookup";SmServiceName name=smEncodeName(PROBE_PORT);bool present=false;
 Result rc=tipcDispatchInOut(smGetServiceSessionTipc(),65100,name,present);
 if(R_FAILED(rc))return rc;
 if(!present)return KERNELRESULT(NotFound);
 query_stage="SM service connection";Handle handle;
 rc=smGetServiceOriginal(&handle,name);
 if(R_SUCCEEDED(rc))*s=(Service){.session=handle,.own_handle=1};
 return rc;
}
static __attribute__((noinline)) Result query(void){
 Service service;Result rc=connect(&service);if(R_FAILED(rc)){available=false;return rc;}
 memset(&current,0,sizeof(current));query_stage="inline status reply";rc=serviceDispatchOut(&service,PROBE_STATUS,current);serviceClose(&service);
 if(R_SUCCEEDED(rc) && current.version!=PROBE_VERSION)rc=MAKERESULT(345,3);available=R_SUCCEEDED(rc);return rc;
}
static void save_baseline(void){
 baseline=current;
#ifndef PROBE_HOST
 mkdir("sdmc:/switch",0777);mkdir("sdmc:/switch/SwitchTalkAlertsTest",0777);
#endif
 FILE *f=fopen(BASELINE,"wb");if(!f){snprintf(note,sizeof(note),"Worker started, but could not save the baseline. Record instance/count manually.");return;}
 bool ok=fwrite(&baseline,1,sizeof(baseline),f)==sizeof(baseline);if(fclose(f))ok=false;have_baseline=ok;
 if(!ok)snprintf(note,sizeof(note),"Worker started, but baseline write failed.");
}
static __attribute__((noinline)) void draw(void){
 printf("\x1b[2J\x1b[H=== SwitchTalk Worker 1.0.0 ===\n\n");
 printf("SwitchTalk 1.0.0 notification and quick-reply worker control.\n");
 printf("Uses your encrypted saved account; no voice or auto-start.\n\n");
 printf("Worker: %s\n",available?"RUNNING":"NOT CONNECTED");
 if(available){
  printf("Checks: %u  Message previews queued: %u  Test popups: %u\n",current.polls,current.real_alerts,current.test_alerts);
  printf("Alerts: All incoming - direct and group (fixed)\n");
  printf("State %u: %s\n\n",current.mode,current.detail);
  printf("Process ID: %llu\nInstance: %016llx\nHeartbeat count: %llu\n",(unsigned long long)current.pid,(unsigned long long)current.instance,(unsigned long long)current.heartbeats);
  if(have_baseline){
   bool same=current.instance==baseline.instance;
   printf("Same instance as last A: %s\n",same?"YES":"NO - worker restarted");
   if(same && current.heartbeats>=baseline.heartbeats){
    unsigned long long delta=current.heartbeats-baseline.heartbeats;
    printf("Heartbeat advance since A: %llu\n",delta);
    printf("Worker wait-loop activity: about %llu seconds\n",delta/4);
   }
  }
 }
 printf("\n%s\n\n",note);
 printf("A Start worker / record new baseline\nX Check status WITHOUT starting anything\nY Stop worker and clear queued SwitchTalk alerts\nR Queue an Ultrahand test alert\n+ or B Close test app; worker stays running\n\n");
 printf("A starts; use X until State 2 / Monitoring and checks >= 1.\n");
 printf("Send a direct message from another account to see sender + preview.\n");
 printf("Return via Album and X for status. Ultrahand API Notifications must be on.\n");
 consoleUpdate(NULL);
}
int main(int argc,char **argv){
 (void)argc;(void)argv;consoleInit(NULL);padConfigureInput(1,HidNpadStyleSet_NpadStandard);PadState pad;padInitializeDefault(&pad);
 FILE *f=fopen(BASELINE,"rb");if(f){have_baseline=fread(&baseline,1,sizeof(baseline),f)==sizeof(baseline) && baseline.version==PROBE_VERSION;fclose(f);}
 query();draw();
 while(appletMainLoop()){
  padUpdate(&pad);u64 down=padGetButtonsDown(&pad);if(down&(HidNpadButton_B|HidNpadButton_Plus))break;
  if(down&HidNpadButton_A){
   const char *stage="status query";Result rc=query();
   if(rc==KERNELRESULT(NotFound)){
    stage="pm:shell access";rc=pmshellInitialize();
    if(R_SUCCEEDED(rc)){stage="process launch";NcmProgramLocation location={.program_id=PROBE_ID,.storageID=NcmStorageId_None};u64 pid=0;rc=pmshellLaunchProgram(0,&location,&pid);pmshellExit();}
    if(R_SUCCEEDED(rc)){stage="worker connection";for(int i=0;i<40;i++){rc=query();if(R_SUCCEEDED(rc) || rc!=KERNELRESULT(NotFound))break;svcSleepThread(50000000);}}
   }
   if(R_FAILED(rc) && (!strcmp(stage,"worker connection") || !strcmp(stage,"status query")))stage=query_stage;
   if(R_SUCCEEDED(rc)){snprintf(note,sizeof(note),"Worker is running. Press X to check initialization; wait for Monitoring.");save_baseline();}
   else snprintf(note,sizeof(note),"Stage [%s] rejected: 0x%08x. Alerts are not enabled.",stage,rc);
  }
  if(down&HidNpadButton_X){Result rc=query();if(R_SUCCEEDED(rc))snprintf(note,sizeof(note),"Status read only. Worker was NOT launched by this check.");else snprintf(note,sizeof(note),"Worker [%s]: 0x%08x. X did NOT start it.",query_stage,rc);}
  if(down&HidNpadButton_R){
   Service service;Result rc=connect(&service);if(R_SUCCEEDED(rc)){rc=serviceDispatch(&service,PROBE_TEST);serviceClose(&service);}
   snprintf(note,sizeof(note),"Test request: 0x%08x. Alert can take several seconds; X updates status.",rc);
  }
  if(down&HidNpadButton_Y){
   Service service;Result rc=connect(&service);if(R_SUCCEEDED(rc)){rc=serviceDispatch(&service,PROBE_STOP);serviceClose(&service);}
   snprintf(note,sizeof(note),"Stop result: 0x%08x. A can start a fresh test.",rc);available=false;
  }
  if(down)draw();svcSleepThread(33000000);
 }
 consoleExit(NULL);return 0;
}
