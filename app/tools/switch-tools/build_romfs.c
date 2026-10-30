#include <stdio.h>
#include "romfs.h"
int main(int argc,char **argv){if(argc!=3)return 1;build_romfs_by_paths(argv[1],argv[2]);return 0;}
