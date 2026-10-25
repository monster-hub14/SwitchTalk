#ifndef SWITCHTALK_JSON_H
#define SWITCHTALK_JSON_H
#include <stddef.h>
#define JSON_MAX_TOKENS 32768

typedef enum { JSON_INVALID=0, JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_PRIMITIVE } JsonType;
typedef struct { JsonType type; int start, end, next, count; } JsonToken;
typedef struct { const char *text; int used; JsonToken tokens[JSON_MAX_TOKENS]; } JsonDoc;
/* Returns 0 on success. Does not allocate or own the input string. */
int json_parse(JsonDoc *doc, const char *text);
/* Returns token index or -1. */
int json_get(const JsonDoc *doc, int object, const char *key);
int json_item(const JsonDoc *doc, int array, int idx);
int json_count(const JsonDoc *doc, int container);
void json_string(const JsonDoc *doc, int token, char *out, size_t capacity);
long json_long(const JsonDoc *doc, int token);
#endif
