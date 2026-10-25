#pragma once
#include "json.h"
#include <stddef.h>
/* Render Nextcloud Rich Object Strings as plain text, never HTML or recursive templates. */
void talk_format_message(const JsonDoc *doc,int row,char *out,size_t capacity);
