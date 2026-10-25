#include "json.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void skip_space(const char *s, int *p) { while (s[*p] && isspace((unsigned char)s[*p])) ++*p; }
static int parse_value(JsonDoc *d, int *pos, int depth);
static int parse_string(JsonDoc *d, int *pos) {
    const char *s = d->text;
    if (s[*pos] != '"' || d->used >= JSON_MAX_TOKENS) return -1;
    int t = d->used++;
    d->tokens[t] = (JsonToken){ .type=JSON_STRING, .start=++*pos, .count=0 };
    while (s[*pos]) {
        if (s[*pos] == '"') {
            d->tokens[t].end = *pos;
            ++*pos; d->tokens[t].next=d->used;
            return t;
        }
        if ((unsigned char)s[*pos] < 32) return -1;
        if (s[*pos] == '\\') {
            ++*pos;
            if (!s[*pos]) return -1;
            if (s[*pos] == 'u') {
                for (int i=0; i<4; ++i) {
                    ++*pos;
                    if (!isxdigit((unsigned char)s[*pos])) return -1;
                }
            } else if (!strchr("\"\\/bfnrt", s[*pos])) return -1;
        }
        ++*pos;
    }
    return -1;
}
static int parse_value(JsonDoc *d, int *pos, int depth) {
    if (depth > 40 || d->used >= JSON_MAX_TOKENS) return -1;
    const char *s=d->text;
    skip_space(s,pos);
    if (s[*pos]=='"') return parse_string(d,pos);
    int t=d->used++;
    d->tokens[t] = (JsonToken){.start=*pos,.type=JSON_PRIMITIVE};
    if (s[*pos]=='{' || s[*pos]=='[') {
        int object=(s[*pos]=='{');
        d->tokens[t].type=object ? JSON_OBJECT : JSON_ARRAY;
        ++*pos; skip_space(s,pos);
        char close=object?'}':']';
        while (s[*pos] && s[*pos]!=close) {
            if (object) {
                if (parse_string(d,pos)<0) return -1;
                skip_space(s,pos);
                if (s[(*pos)++]!=':') return -1;
                if (parse_value(d,pos,depth+1)<0) return -1;
            } else if (parse_value(d,pos,depth+1)<0) return -1;
            ++d->tokens[t].count;
            skip_space(s,pos);
            if (s[*pos]==',') { ++*pos; skip_space(s,pos); if(s[*pos]==close) return -1; }
            else if (s[*pos]!=close) return -1;
        }
        if(s[*pos]!=close) return -1;
        ++*pos;
    } else {
        int start=*pos;
        while (s[*pos] && !isspace((unsigned char)s[*pos]) && !strchr(",}]", s[*pos])) ++*pos;
        int len=*pos-start;
        if (len==0) return -1;
        if (!(len==4&&!strncmp(s+start,"true",4)) &&
            !(len==5&&!strncmp(s+start,"false",5)) &&
            !(len==4&&!strncmp(s+start,"null",4))) {
            char *endptr;
            (void)strtod(s+start,&endptr);
            if(endptr!=s+*pos) return -1;
        }
    }
    d->tokens[t].end=*pos; d->tokens[t].next=d->used;
    return t;
}
int json_parse(JsonDoc *d, const char *s) {
    if (!d || !s) return -1;
    d->text=s; d->used=0; int p=0;
    if (parse_value(d,&p,0)!=0) return -1;
    skip_space(s,&p);
    return s[p] ? -1:0;
}
static void append_utf8(char *out,size_t cap,size_t *n,uint32_t cp) {
    unsigned char b[4]; int m=0;
    if(cp<0x80) { b[m++]=(unsigned char)cp; }
    else if(cp<0x800) { b[m++]=0xC0|(cp>>6); b[m++]=0x80|(cp&63); }
    else if(cp<0x10000) { b[m++]=0xE0|(cp>>12); b[m++]=0x80|((cp>>6)&63); b[m++]=0x80|(cp&63); }
    else if(cp<=0x10ffff) { b[m++]=0xF0|(cp>>18); b[m++]=0x80|((cp>>12)&63); b[m++]=0x80|((cp>>6)&63); b[m++]=0x80|(cp&63); }
    for(int j=0;j<m && *n+1<cap;j++) out[(*n)++]=(char)b[j];
}
static unsigned hexval(char ch) { if(ch>='0'&&ch<='9')return (unsigned)(ch-'0'); ch=(char)tolower((unsigned char)ch);return (unsigned)(ch-'a'+10); }
static uint32_t decode_u4(const char *s,int p) { return (hexval(s[p])<<12)|(hexval(s[p+1])<<8)|(hexval(s[p+2])<<4)|hexval(s[p+3]); }
void json_string(const JsonDoc *d,int t,char *out,size_t cap) {
    if(!out||!cap)return;
    out[0]=0;
    if(!d||t<0||t>=d->used) return;
    const JsonToken *q=&d->tokens[t]; size_t n=0;
    for(int i=q->start; i<q->end&&n+1<cap; ++i) {
        unsigned char c=(unsigned char)d->text[i];
        if(c=='\\' && i+1<q->end) {
            char e=d->text[++i];
            if(e=='u' && i+4<q->end) {
                uint32_t cp=decode_u4(d->text,i+1); i+=4;
                if(cp>=0xD800&&cp<=0xDBFF && i+6<q->end && d->text[i+1]=='\\'&&d->text[i+2]=='u') {
                    uint32_t low=decode_u4(d->text,i+3);
                    if(low>=0xDC00&&low<=0xDFFF){cp=0x10000+((cp-0xD800)<<10)+(low-0xDC00);i+=6;}
                }
                if(cp>=0xD800&&cp<=0xDFFF) cp='?';
                if(cp>=32) append_utf8(out,cap,&n,cp);
                continue;
            }
            c=(unsigned char)(e=='n'||e=='r'||e=='t'?' ':e=='b'||e=='f'?' ':e);
        }
        /* Strip control/ANSI bytes so incoming messages cannot control terminal display. */
        if (c<32 || c==127) c=' ';
        out[n++]=(char)c;
    }
    out[n]=0;
}
int json_get(const JsonDoc *d,int obj,const char *key) {
    if(!d||obj<0||obj>=d->used||d->tokens[obj].type!=JSON_OBJECT)return -1;
    for(int p=obj+1;p<d->tokens[obj].next;) {
        if(p+1>=d->used)return -1;
        char tmp[128]; json_string(d,p,tmp,sizeof(tmp));
        if(!strcmp(tmp,key))return p+1;
        p=d->tokens[p+1].next;
    }
    return -1;
}
int json_item(const JsonDoc *d,int arr,int index) {
    if(!d||arr<0||arr>=d->used||d->tokens[arr].type!=JSON_ARRAY||index<0)return -1;
    int p=arr+1;
    for(int i=0;i<index && p<d->tokens[arr].next;i++)p=d->tokens[p].next;
    return p<d->tokens[arr].next ? p:-1;
}
int json_count(const JsonDoc *d,int container) {
    if(!d||container<0||container>=d->used)return 0;
    return d->tokens[container].count;
}
long json_long(const JsonDoc *d,int t) {
    if(!d||t<0||t>=d->used)return 0;
    char val[64];json_string(d,t,val,sizeof(val));return strtol(val,NULL,10);
}
