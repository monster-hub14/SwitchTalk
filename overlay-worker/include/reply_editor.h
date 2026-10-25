#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
typedef struct {unsigned row,col;bool upper,symbols;} ReplyEditor;
static unsigned reply_columns(unsigned row){return row==4?5:10;}
static void reply_move(ReplyEditor *e,int dx,int dy){if(dy)e->row=(e->row+5+dy)%5;if(e->col>=reply_columns(e->row))e->col=reply_columns(e->row)-1;if(dx)e->col=(e->col+reply_columns(e->row)+dx)%reply_columns(e->row);}
static char reply_character(const ReplyEditor *e){
 static const char *letters[]={"1234567890","qwertyuiop","asdfghjkl;","zxcvbnm,.?"};
 static const char *symbols[]={"!@#$%^&*()","-_=+[]{}<>","/\\|:;\"'`~?",".,?!@#%&*_"};
 if(e->row>=4||e->col>=10)return 0;char c=(e->symbols?symbols:letters)[e->row][e->col];if(e->upper && c>='a'&&c<='z')c-=32;return c;
}
static bool reply_append(char *text,size_t cap,char ch){size_t n=strlen(text);if(!ch||n+1>=cap)return false;text[n]=ch;text[n+1]=0;return true;}
static void reply_delete(char *text){size_t n=strlen(text);if(!n)return;do{--n;}while(n && ((unsigned char)text[n]&0xc0)==0x80);text[n]=0;}
