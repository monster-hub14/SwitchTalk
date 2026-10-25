#pragma once
#include <stdbool.h>
#include "talk.h"
/* A missing file is normal. Errors leave the supplied credentials untouched. */
bool saved_login_load(TalkClient *client);
bool saved_login_save(const TalkClient *client);
bool saved_login_forget(void);
const char *saved_login_error(void);
