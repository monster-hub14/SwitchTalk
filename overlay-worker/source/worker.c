/* Message Alert Test 7: manual-start SM service; no applet, HID or audio. */
#include <switch.h>
#include <string.h>
#include "probe.h"
#include "alert_worker.h"
#include "quick_reply.h"
u32 __nx_applet_type=AppletType_None;
void __libnx_initheap(void){
 static u8 heap[1536*1024] __attribute__((aligned(4096)));
 extern void *fake_heap_start,*fake_heap_end;fake_heap_start=heap;fake_heap_end=heap+sizeof(heap);
}
void __appInit(void){}
void __appExit(void){}
static ProbeStatus state;
static bool stopping;
static Result command(u32 cmd,HipcParsedRequest *request){
 if(request->meta.num_send_buffers)return MAKERESULT(345,1);
 if(cmd==PROBE_STATUS){
  if(request->meta.num_recv_buffers)return MAKERESULT(345,1);
  state.ticks=svcGetSystemTick();alert_status(&state);return 0;
 }
 if(cmd==PROBE_STOP && !request->meta.num_recv_buffers){stopping=true;atomic_store(&alert_stop,true);return 0;}
 if(cmd==PROBE_TEST && !request->meta.num_recv_buffers){atomic_store(&alert_test,true);return 0;}
 return MAKERESULT(345,2);
}
static void drop(Handle *handles,int *count,int index){svcCloseHandle(handles[index]);handles[index]=handles[--*count];}
int main(int argc,char **argv){
 (void)argc;(void)argv;state.version=PROBE_VERSION;state.instance=svcGetSystemTick();svcGetProcessId(&state.pid,CUR_PROCESS_HANDLE);
 Handle handles[5];int count=1;Handle reply=INVALID_HANDLE;
 Result rc=smInitialize();if(R_FAILED(rc))return 1;
 rc=smRegisterService(&handles[0],smEncodeName(PROBE_PORT),false,4);if(R_FAILED(rc)){smExit();return 2;}
 (void)alert_start();
 for(;;){
  s32 index=-1;Handle previous=reply;
  rc=svcReplyAndReceive(&index,handles,count,reply,stopping?0:250000000ULL);reply=INVALID_HANDLE;
  /* The STOP response was sent before this exit; no background leftovers. */
  if(stopping)break;
  if(R_FAILED(rc)){
   if(rc==KERNELRESULT(TimedOut)){++state.heartbeats;continue;}
   if(index>0 && index<count)drop(handles,&count,index);
   else if(previous!=INVALID_HANDLE)for(int i=1;i<count;i++)if(handles[i]==previous){drop(handles,&count,i);break;}
   svcSleepThread(10000000);continue;
  }
  if(index==0){Handle session;if(R_SUCCEEDED(svcAcceptSession(&session,handles[0]))){if(count<5)handles[count++]=session;else svcCloseHandle(session);}continue;}
  void *tls=armGetTls();HipcParsedRequest request=hipcParseRequest(tls);
  if(request.meta.type==CmifCommandType_Close){drop(handles,&count,index);continue;}
  if((request.meta.type!=CmifCommandType_Request && request.meta.type!=CmifCommandType_Control) || request.meta.num_send_statics || request.meta.num_recv_statics || request.meta.num_exch_buffers || request.meta.num_send_buffers || request.meta.num_recv_buffers>1 || request.meta.num_copy_handles || request.meta.num_move_handles || request.meta.send_pid){
   for(u32 i=0;i<request.meta.num_copy_handles;i++)svcCloseHandle(request.data.copy_handles[i]);
   for(u32 i=0;i<request.meta.num_move_handles;i++)svcCloseHandle(request.data.move_handles[i]);
   drop(handles,&count,index);continue;
  }
  CmifInHeader *in=cmifGetAlignedDataStart(request.data.data_words,tls);size_t raw=(u8*)in-(u8*)tls;
  if(request.meta.num_data_words<4 || raw+sizeof(*in)>0x100 || (u8*)(in+1)>(u8*)(request.data.data_words+request.meta.num_data_words) || in->magic!=CMIF_IN_HEADER_MAGIC){drop(handles,&count,index);continue;}
  u32 command_id=in->command_id;u32 token=in->token;bool pointer_query=request.meta.type==CmifCommandType_Control && in->command_id==3;
  QRReply quick={0};bool quick_call=request.meta.type==CmifCommandType_Request && command_id==QR_RPC;
  Result result;
  if(quick_call){
   if(request.meta.num_recv_buffers || raw+sizeof(*in)+sizeof(QRRequest)>0x100 || (u8*)(in+1)+sizeof(QRRequest)>(u8*)(request.data.data_words+request.meta.num_data_words))quick.error=QR_BAD;
   else{QRRequest q;memcpy(&q,in+1,sizeof(q));qr_rpc(&q,&quick);}result=0;
  }else result=pointer_query?0:request.meta.type==CmifCommandType_Control?MAKERESULT(345,2):command(command_id,&request);
  memset(tls,0,0x100);HipcRequest response=hipcMakeRequest(tls,(HipcMetadata){.type=0,.num_data_words=(sizeof(ProbeStatus)+32)/4});
  CmifOutHeader *out=cmifGetAlignedDataStart(response.data_words,tls);*out=(CmifOutHeader){.magic=CMIF_OUT_HEADER_MAGIC,.result=result,.token=token};
  if(pointer_query)*(u16*)(out+1)=0;
  else if(quick_call)memcpy(out+1,&quick,sizeof(quick));
  else if(R_SUCCEEDED(result) && command_id==PROBE_STATUS)memcpy(out+1,&state,sizeof(state));
  reply=handles[index];
 }
 alert_join();for(int i=0;i<count;i++)svcCloseHandle(handles[i]);smExit();return 0;
}
