#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifndef QR_ENABLE_SEND
#define QR_ENABLE_SEND 1
#endif
#define QR_RPC 10u
#define QR_MESSAGES 10
#define QR_TEXT 512
#define QR_DATA 160
/* Inline CMIF packets, without mapped buffers. */
typedef struct {uint32_t op,arg,offset,size;char bytes[128];} QRRequest;
typedef struct {uint32_t error,total,revision,state;char bytes[QR_DATA];} QRReply;
typedef struct {char token[128],title[160],preview[160];uint64_t last_id;} QRRoom;
typedef struct {uint64_t id;char sender[128],body[1024];} QRMessage;
typedef struct {uint32_t rooms,messages,job_id,state;uint64_t instance;char token[128],notice[128];} QRMeta;
typedef struct {uint32_t id,kind;char token[128],text[QR_TEXT+1];} QRJob;
enum {QR_META,QR_ROOM,QR_MESSAGE,QR_OPEN,QR_DRAFT_BEGIN,QR_DRAFT_WRITE,QR_SEND,QR_DRAFT_READ,QR_VISIBLE};
enum {QR_OFFLINE,QR_IDLE,QR_PENDING,QR_READY,QR_SENT,QR_FAILED};
enum {QR_OK,QR_BAD,QR_BUSY,QR_STALE,QR_NOT_READY,QR_RANGE};
enum {QR_HISTORY_JOB=1,QR_SEND_JOB=2};
bool qr_room_visible(const char*,uint64_t);
void qr_init(uint64_t instance);
void qr_reset(const char *notice);
void qr_rpc(const QRRequest*,QRReply*);
bool qr_has_job(void);
bool qr_take_job(QRJob*);
void qr_complete_history(uint32_t,const QRMessage*,int,const char*);
void qr_complete_send(uint32_t,const QRMessage*,const char*);
#ifdef __cplusplus
}
#endif
