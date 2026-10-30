#include "update-manager.hpp"
#include "update-bundle.hpp"
#include "updater.h"
#include "probe.h"
#include <cmath>
namespace {
update::Manager *manager;bool mounted=false,restartRequested=false,workerWasRunning=false;std::string error;
double now(){return double(svcGetSystemTick())/19200000.0;}
bool stopWorker(){
 Result rc=smInitialize();if(R_FAILED(rc))return false;SmServiceName name=smEncodeName(PROBE_PORT);bool exists=false;rc=tipcDispatchInOut(smGetServiceSessionTipc(),65100,name,exists);
 if(R_SUCCEEDED(rc)&&exists){Handle h=INVALID_HANDLE;rc=smGetServiceOriginal(&h,name);if(R_SUCCEEDED(rc)){Service service={};service.session=h;service.own_handle=1;ProbeStatus state{};rc=serviceDispatchOut(&service,PROBE_STATUS,state);
  if(R_SUCCEEDED(rc)&&state.version==PROBE_VERSION)rc=serviceDispatch(&service,PROBE_STOP);else rc=MAKERESULT(345,3);serviceClose(&service);}
  if(R_SUCCEEDED(rc)){workerWasRunning=true;for(unsigned i=0;i<600;i++){rc=tipcDispatchInOut(smGetServiceSessionTipc(),65100,name,exists);if(R_FAILED(rc)||!exists)break;svcSleepThread(50000000);}}
 }smExit();return R_SUCCEEDED(rc)&&!exists;
}
void resumeWorker(){if(!workerWasRunning)return;Result rc=pmshellInitialize();if(R_SUCCEEDED(rc)){NcmProgramLocation loc{};loc.program_id=PROBE_ID;loc.storageID=NcmStorageId_None;u64 pid=0;for(unsigned attempt=0;attempt<30;attempt++){rc=pmshellLaunchProgram(0,&loc,&pid);if(R_SUCCEEDED(rc))break;svcSleepThread(100000000);}pmshellExit();}if(R_FAILED(rc))manager->set(update::Phase::Error,"Update installed. Start the worker with A in its controller.");}
}
extern "C" bool st_update_prepare(const char *path){
 manager=new update::Manager;manager->init(path?path:"");mounted=R_SUCCEEDED(romfsInit());if(!mounted){error="The complete update-enabled ZIP is required (missing app bundle)";return false;}
 mkdir("sdmc:/switch",0777);mkdir(update::Root,0777);mkdir("sdmc:/switch/.overlays",0777);mkdir("sdmc:/switch/SwitchTalkAlertsTest",0777);mkdir("sdmc:/atmosphere",0777);mkdir("sdmc:/atmosphere/contents",0777);mkdir("sdmc:/atmosphere/contents/4200000000535755",0777);
 std::array<std::string,3> targets={"sdmc:/switch/.overlays/SwitchTalk.ovl","sdmc:/atmosphere/contents/4200000000535755/exefs.nsp","sdmc:/switch/SwitchTalkAlertsTest/SwitchTalkAlertsTest.nro"};
 if(!update::syncBundle("romfs:/bundle",update::Version,targets,stopWorker,error))return false;
 resumeWorker();return true;
}
extern "C" const char *st_update_error(){return error.c_str();}
extern "C" void st_update_boot_ok(){if(manager&&manager->running&&update::readText(manager->slots.root+"/booting.txt",4)==std::string(1,manager->running)+"\n")update::erase(manager->slots.root+"/booting.txt");}
extern "C" bool st_update_tick(bool idle){if(!manager)return false;if(!manager->started){manager->started=true;if(manager->automatic)manager->start();}manager->tick(idle,now());if(idle&&manager->restart&&!manager->busy&&manager->commit()){restartRequested=true;return true;}return false;}
extern "C" void st_update_input(uint64_t keys){if(manager)manager->input(keys,HidNpadButton_A,HidNpadButton_B,HidNpadButton_X,HidNpadButton_Y);}
extern "C" void st_update_model(GuiUpdateModel *out){memset(out,0,sizeof(*out));if(!manager)return;auto status=manager->snapshot();out->automatic=manager->automatic;out->busy=manager->busy;out->phase=int(status.phase);out->downloaded=status.downloaded;out->total=status.total;out->countdown=-1;
 if(status.phase==update::Phase::Ready&&manager->automatic&&!manager->deferred&&manager->readySince>=0)out->countdown=std::max(0,5-int(now()-manager->readySince));
 snprintf(out->message,sizeof(out->message),"%s",status.message.c_str());snprintf(out->version,sizeof(out->version),"%s",status.version.c_str());}
extern "C" bool st_update_notice(char *out,size_t cap){GuiUpdateModel state;st_update_model(&state);if(state.phase==int(update::Phase::Ready)){if(state.countdown>=0)snprintf(out,cap,"Update %s ready: restart in %ds. B Later; ZL Updates",state.version,state.countdown);else snprintf(out,cap,"Update %s verified. ZL Updates to install",state.version);return true;}if(state.phase==int(update::Phase::Downloading)){snprintf(out,cap,"Downloading update: %llu / %llu KB; ZL Updates",(unsigned long long)(state.downloaded/1024),(unsigned long long)(state.total/1024));return true;}return false;}
extern "C" void st_update_shutdown(){if(manager){manager->stop();delete manager;manager=nullptr;}if(mounted){romfsExit();mounted=false;}if(restartRequested&&envHasNextLoad())envSetNextLoad("sdmc:/switch/SwitchTalk/SwitchTalk.nro","sdmc:/switch/SwitchTalk/SwitchTalk.nro");}
