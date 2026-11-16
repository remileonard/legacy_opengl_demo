/* sys/sysmp.h - the IRIX multiprocessor control: one processor, available */
#ifndef PERFORMER_SYS_SYSMP_H
#define PERFORMER_SYS_SYSMP_H
#ifndef _WIN32
#include <unistd.h>
#else
static int geteuid(void) { return 1; }
#endif
#define MP_NPROCS 1
#define MP_NAPROCS 2
#define sysmp(cmd) 1
#endif
