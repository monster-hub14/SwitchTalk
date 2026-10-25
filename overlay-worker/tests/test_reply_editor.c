#include "reply_editor.h"
#include <assert.h>
int main(void){ReplyEditor e={0};assert(reply_character(&e)=='1');reply_move(&e,-1,0);assert(e.col==9);reply_move(&e,0,-1);assert(e.row==4 && e.col==4);reply_move(&e,1,0);assert(e.col==0);
 for(unsigned mode=0;mode<2;mode++)for(unsigned row=0;row<4;row++)for(unsigned col=0;col<10;col++){e=(ReplyEditor){.row=row,.col=col,.symbols=mode};assert(reply_character(&e)!=0);}
 e=(ReplyEditor){.row=1,.upper=true};assert(reply_character(&e)=='Q');char b[5]="";assert(reply_append(b,5,'a'));assert(reply_append(b,5,'b'));assert(reply_append(b,5,'c'));assert(reply_append(b,5,'d'));assert(!reply_append(b,5,'e'));reply_delete(b);assert(!strcmp(b,"abc"));strcpy(b,"\xc3\xa9");reply_delete(b);assert(!b[0]);reply_delete(b);return 0;}
