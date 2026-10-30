#include "alert_worker.h"
#include "alert_policy.h"
#include "alert_preview.h"
#include "notify_file.h"
#include "talk.h"
#include "saved_login.h"
#include "qr_service.h"
#include "qr_http.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
atomic_bool alert_stop,alert_test;
static Thread thread;static bool started;static Mutex lock;
static ProbeStatus info;static TalkClient client;static AlertPolicy policy;static AlertRoom rooms[ALERT_ROOMS];
u32 __nx_fs_num_sessions=1;
u32 __nx_time_service_type=TimeServiceType_System;
static void erase(void *p,size_t n){volatile unsigned char *b=p;while(n--)*b++=0;}
static void status(unsigned mode,const char *text){mutexLock(&lock);info.mode=mode;snprintf(info.detail,sizeof(info.detail),"%.79s",text);mutexUnlock(&lock);}
static bool room_polling;
static bool cancel(void *p){(void)p;return atomic_load(&alert_stop) || (room_polling && qr_has_job());}
static bool emit(const AlertRoom *r,void *p){(void)p;if(qr_room_visible(r->token,svcGetSystemTick()))return true;char text[512];alert_preview(r,text,sizeof(text));bool ok=notify_write(r->token,r->id,text);if(ok){mutexLock(&lock);++info.alerts;++info.real_alerts;mutexUnlock(&lock);}return ok;}
static void wait_seconds(unsigned n){for(unsigned i=0;i<n*10 && !atomic_load(&alert_stop) && !atomic_load(&alert_test) && !qr_has_job();i++)svcSleepThread(100000000);}
static void outbox_status(void){unsigned pending,real,test,errors;notify_diagnostics(&pending,&real,&test,&errors);mutexLock(&lock);info.pending=pending;info.last_real=real;info.last_test=test;info.write_errors=errors;mutexUnlock(&lock);}
static bool stage(Result rc,const char *name){if(R_SUCCEEDED(rc))return true;char detail[112];snprintf(detail,sizeof(detail),"%s failed: 0x%08x. Stop and restart to retry.",name,rc);qr_reset(detail);status(4,detail);return false;}
static void run(void *arg){
 (void)arg;bool fs=false,sd=false,timer=false,nifm=false,sockets=false,curl=false;
 Result rc=setsysInitialize();if(!stage(rc,"Firmware service"))goto idle;
 SetSysFirmwareVersion fw;rc=setsysGetFirmwareVersion(&fw);setsysExit();if(!stage(rc,"Firmware version"))goto idle;hosversionSet(MAKEHOSVERSION(fw.major,fw.minor,fw.micro));
 if(!stage(timeInitialize(),"System time"))goto idle;timer=true;extern void __libnx_init_time(void);__libnx_init_time();
 if(!stage(fsInitialize(),"SD service"))goto idle;fs=true;
 if(!stage(fsdevMountSdmc(),"SD mount"))goto idle;sd=true;notify_clear();notify_begin(svcGetSystemTick());
 if(!stage(nifmInitialize(NifmServiceType_System),"Network manager"))goto idle;nifm=true;
 SocketInitConfig cfg=*socketGetDefaultInitConfig();cfg.bsd_service_type=BsdServiceType_Auto;cfg.num_bsd_sessions=1;
 cfg.tcp_tx_buf_size=cfg.tcp_rx_buf_size=0x4000;cfg.tcp_tx_buf_max_size=cfg.tcp_rx_buf_max_size=0;cfg.udp_tx_buf_size=0x2400;cfg.udp_rx_buf_size=0xa500;cfg.sb_efficiency=1;
 if(!stage(socketInitialize(&cfg),"Sockets"))goto idle;sockets=true;
 if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK){status(4,"HTTP setup failed. Stop and restart.");goto idle;}curl=true;
 talk_set_cancel(cancel,NULL);status(1,"Starting: loading saved account and recording initial message IDs");
 unsigned failures=0;bool have_account=false;
 while(!atomic_load(&alert_stop)){
  if(atomic_exchange(&alert_test,false)){bool ok=notify_write("test",(long)(svcGetSystemTick()/19200000),"Background notification test - SwitchTalk worker is running");if(ok){mutexLock(&lock);++info.alerts;++info.test_alerts;mutexUnlock(&lock);}status(ok?2:3,ok?"Test alert queued; check Ultrahand display":"Could not queue test alert (SD error or 8 pending alerts)");}
  if(access("sdmc:/switch/SwitchTalk/account.bin",F_OK) && access("sdmc:/switch/SwitchTalk/account.bin.tmp",F_OK)){
   erase(&client,sizeof(client));memset(&policy,0,sizeof(policy));notify_clear();have_account=false;qr_reset("Sign in using SwitchTalk first");status(3,"No saved login. Sign in using SwitchTalk 1.1.0 first.");wait_seconds(8);continue;
  }
  /* Reopen the encrypted record so sign-out or account changes take effect. */
  static TalkClient loaded;erase(&loaded,sizeof(loaded));
  if(!saved_login_load(&loaded)){erase(&client,sizeof(client));memset(&policy,0,sizeof(policy));have_account=false;qr_reset(saved_login_error());status(3,saved_login_error());wait_seconds(16);continue;}
  if(!have_account || strcmp(client.server,loaded.server) || strcmp(client.username,loaded.username) || strcmp(client.app_password,loaded.app_password)){
   notify_clear();qr_reset("Loading conversations...");erase(&client,sizeof(client));memset(&policy,0,sizeof(policy));snprintf(client.server,sizeof(client.server),"%s",loaded.server);snprintf(client.username,sizeof(client.username),"%s",loaded.username);snprintf(client.app_password,sizeof(client.app_password),"%s",loaded.app_password);have_account=true;
  }erase(&loaded,sizeof(loaded));
  QRJob command;
  if(qr_take_job(&command)){
   if(command.kind==QR_HISTORY_JOB){static QRMessage history[QR_MESSAGES];int n=0;bool ok=qr_http_history(&client,command.token,history,&n);qr_complete_history(command.id,history,n,ok?NULL:client.error);erase(history,sizeof(history));}
   #if QR_ENABLE_SEND
   else if(command.kind==QR_SEND_JOB){QRMessage sent;bool ok=qr_http_send(&client,command.token,command.text,&sent);qr_complete_send(command.id,&sent,ok?NULL:client.error);erase(&sent,sizeof(sent));}
   #endif
   erase(&command,sizeof(command));
  }
  int count=0;room_polling=true;bool fetched=alert_fetch(&client,rooms,&count);room_polling=false;
  if(fetched){
   qr_publish_rooms(rooms,count);
   alert_scan(&policy,rooms,count,client.username,client.account_id,emit,NULL);failures=0;
   mutexLock(&lock);++info.polls;info.max_id=policy.stats.max_id;info.changes+=policy.stats.changes;info.own+=policy.stats.own;info.settings+=policy.stats.settings;info.system+=policy.stats.system;info.blocked+=policy.stats.blocked;info.baselines+=policy.stats.baselines;mutexUnlock(&lock);
   char detail[112];if(policy.stats.last[0])snprintf(detail,sizeof(detail),"%s",policy.stats.last);else snprintf(detail,sizeof(detail),"Monitoring: %u rooms; %u have last-message IDs. Checks every 8s.",policy.stats.rooms,policy.stats.ids);status(2,detail);
  }else if(!qr_has_job()){if(failures<4)++failures;mutexLock(&lock);++info.failures;mutexUnlock(&lock);status(3,client.error);}
  outbox_status();
  wait_seconds(failures?(8u<<failures):8);
 }
 idle:
 while(!atomic_load(&alert_stop)){
  if(atomic_exchange(&alert_test,false) && sd){bool ok=notify_write("test",(long)(svcGetSystemTick()/19200000),"SwitchTalk test alert (network setup failed; see launcher)");if(ok){mutexLock(&lock);++info.alerts;++info.test_alerts;mutexUnlock(&lock);}else status(4,"Test alert write failed");}
  if(sd)outbox_status();svcSleepThread(100000000);
 }
 erase(&client,sizeof(client));erase(&policy,sizeof(policy));if(sd)notify_clear();
 if(curl)curl_global_cleanup();if(sockets)socketExit();if(nifm)nifmExit();if(sd)fsdevUnmountAll();if(fs)fsExit();if(timer)timeExit();
}
Result alert_start(void){mutexInit(&lock);qr_init(svcGetSystemTick());atomic_init(&alert_stop,false);atomic_init(&alert_test,false);status(1,"Starting background notification services");Result rc=threadCreate(&thread,run,NULL,NULL,64*1024,0x30,3);if(R_FAILED(rc)){stage(rc,"Network thread");return rc;}rc=threadStart(&thread);if(R_FAILED(rc)){threadClose(&thread);stage(rc,"Network thread start");return rc;}started=true;return 0;}
void alert_join(void){atomic_store(&alert_stop,true);if(started){threadWaitForExit(&thread);threadClose(&thread);started=false;}}
void alert_status(ProbeStatus *out){mutexLock(&lock);out->polls=info.polls;out->alerts=info.alerts;out->failures=info.failures;out->mode=info.mode;out->real_alerts=info.real_alerts;out->test_alerts=info.test_alerts;out->max_id=info.max_id;out->pending=info.pending;out->last_real=info.last_real;out->last_test=info.last_test;out->write_errors=info.write_errors;out->changes=info.changes;out->own=info.own;out->settings=info.settings;out->system=info.system;out->blocked=info.blocked;out->baselines=info.baselines;memcpy(out->detail,info.detail,sizeof(info.detail));mutexUnlock(&lock);}
