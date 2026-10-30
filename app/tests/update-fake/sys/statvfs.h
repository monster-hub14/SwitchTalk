#pragma once
#include <cstdint>
namespace fakeSpace {inline uint64_t available=1024*1024*1024;}
struct statvfs {uint64_t f_bavail=0,f_frsize=1;};
inline int statvfs(const char*,struct statvfs*out){out->f_bavail=fakeSpace::available;out->f_frsize=1;return 0;}
