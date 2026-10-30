#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {bool automatic,busy;int phase,countdown;uint64_t downloaded,total;char message[240],version[32];} GuiUpdateModel;
#ifdef __cplusplus
extern "C" {
#endif
bool st_update_prepare(const char *path);
const char *st_update_error(void);
void st_update_boot_ok(void);
bool st_update_tick(bool idle);
void st_update_input(uint64_t keys);
void st_update_model(GuiUpdateModel *out);
bool st_update_notice(char *out,size_t cap);
void st_update_shutdown(void);
#ifdef __cplusplus
}
#endif
