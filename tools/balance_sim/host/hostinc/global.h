/* Shadow of include/global.h for the host build (x86-64 shared library).
   Bytecode pointers in the assembled battle scripts are 32-bit pseudo addresses; HostPtr maps them. */
#include "../../../../include/global.h"
extern void *HostPtr(unsigned int a);
#undef T1_READ_PTR
#undef T2_READ_PTR
#define T1_READ_PTR(ptr) ((u8 *) HostPtr((u32)T1_READ_32(ptr)))
#define T2_READ_PTR(ptr) ((void *) HostPtr((u32)T2_READ_32(ptr)))
