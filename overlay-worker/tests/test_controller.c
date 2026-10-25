#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#define main probe_main
#include "../source/controller.c"
#undef main
static bool running;static unsigned starts,stops,test_requests,frame,count;static u64 keys[8];static Result launch_result,init_result,status_result,connect_result;static ProbeStatus fake={.version=PROBE_VERSION,.instance=100,.heartbeats=10,.pid=99};
SmServiceName smEncodeName(const char *s){SmServiceName n={{0}};memcpy(n.name,s,strlen(s));return n;}
Service *smGetServiceSessionTipc(void){static Service s;return &s;}
Result mock_has_service(SmServiceName n,bool *present){assert(!strcmp(n.name,PROBE_PORT));*present=running;return 0;}
Result smGetServiceOriginal(Handle *h,SmServiceName n){assert(!strcmp(n.name,PROBE_PORT));assert(running);if(connect_result)return connect_result;*h=7;return 0;}

void serviceCreate(Service *s,Handle h){(void)s;(void)h;}void serviceClose(Service *s){(void)s;}
Result mock_dispatch(Service *s,unsigned cmd,MockDispatch args){(void)s;if(cmd==PROBE_TEST){test_requests++;return 0;}if(cmd==PROBE_STATUS){if(status_result)return status_result;assert(args.buffers[0].size==sizeof(fake));memcpy((void*)args.buffers[0].ptr,&fake,sizeof(fake));return 0;}assert(cmd==PROBE_STOP);stops++;running=false;return 0;}
void *consoleInit(void *p){return p;}void consoleExit(void *p){(void)p;}void consoleUpdate(void *p){(void)p;}
void padConfigureInput(unsigned a,unsigned b){(void)a;(void)b;}void padInitializeDefault(PadState *p){(void)p;frame=0;}void padUpdate(PadState *p){(void)p;frame++;}u64 padGetButtonsDown(PadState *p){(void)p;return keys[frame-1];}
bool appletMainLoop(void){return frame<count;}void svcSleepThread(int64_t ns){(void)ns;}
Result pmshellInitialize(void){return init_result;}void pmshellExit(void){}
Result pmshellLaunchProgram(uint32_t flags,const NcmProgramLocation *location,uint64_t *pid){assert(!flags && location->program_id==PROBE_ID);starts++;if(launch_result)return launch_result;running=true;*pid=fake.pid;return 0;}
int probe_test_printf(const char *f,...){(void)f;return 0;}
static void fresh(u64 first,u64 second){memset(&current,0,sizeof(current));memset(&baseline,0,sizeof(baseline));available=have_baseline=false;keys[0]=first;keys[1]=second;count=2;probe_main(0,NULL);}
int main(void){
 remove(BASELINE);fresh(HidNpadButton_X,HidNpadButton_Plus);assert(!starts && !stops && !available);
 fresh(HidNpadButton_A,HidNpadButton_Plus);assert(starts==1 && !stops && running && have_baseline && baseline.instance==100);
 fake.heartbeats=500;fresh(HidNpadButton_X,HidNpadButton_Plus);assert(starts==1 && !stops && current.heartbeats-baseline.heartbeats==490 && current.instance==baseline.instance);
 fake.instance=200;fresh(HidNpadButton_X,HidNpadButton_Plus);assert(current.instance!=baseline.instance && starts==1);
 fresh(HidNpadButton_Y,HidNpadButton_Plus);assert(stops==1 && !running);
 launch_result=0x10801;fresh(HidNpadButton_A,HidNpadButton_Plus);assert(starts==2 && !running && strstr(note,"00010801"));
 assert(strstr(note,"process launch"));
 launch_result=0;init_result=0x1015;fresh(HidNpadButton_A,HidNpadButton_Plus);assert(starts==2 && !running && strstr(note,"pm:shell access") && strstr(note,"00001015"));
 init_result=0;launch_result=0x1015;fresh(HidNpadButton_A,HidNpadButton_Plus);assert(starts==3 && !running && strstr(note,"process launch") && strstr(note,"00001015"));
 launch_result=0;status_result=0x10801;fresh(HidNpadButton_A,HidNpadButton_Plus);assert(running && strstr(note,"inline status reply") && strstr(note,"00010801"));
 status_result=0;running=false;connect_result=0x10801;fresh(HidNpadButton_A,HidNpadButton_Plus);assert(running && strstr(note,"SM service connection") && strstr(note,"00010801"));
 connect_result=0;
 running=true;connect_result=0;fresh(HidNpadButton_R,HidNpadButton_Plus);assert(test_requests==1 && running);
 unsigned before=starts;connect_result=0x10801;fresh(HidNpadButton_A,HidNpadButton_Plus);assert(starts==before && strstr(note,"SM service connection"));connect_result=0;
 unsigned prior_starts=starts,prior_stops=stops;fresh(HidNpadButton_L,HidNpadButton_Plus);assert(starts==prior_starts && stops==prior_stops && running);
 remove(BASELINE);return 0;
}
