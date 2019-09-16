/* **********************************************************
 * Copyright (c) 2014-2019 Google, Inc.  All rights reserved.
 * **********************************************************/

/* Dr. Memory: the memory debugger
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License, and no later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* MacOS BSD system call table */

#include "dr_api.h"
#include "drsyscall.h"
#include "drsyscall_os.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/ev.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/semaphore.h>
#include <sys/event.h>
#include <poll.h>
#ifdef X64
struct mac {
  size_t m_buflen;
  char *m_string;
};
#else
# include <security/mac.h>
#endif
#include <mach/shared_region.h>

#ifdef MACOS
# define _XOPEN_SOURCE 700 /* required to get POSIX, etc. defines out of ucontext.h */
# define __need_struct_ucontext64 /* seems to be missing from Mac headers */
#endif
#include <signal.h>
#include <ucontext.h>

#include <sys/syscall.h>

/* Old syscalls that have been removed in 10.9.1 */
#ifndef SYS_profil
# define SYS_profil          44
#endif
#ifndef SYS_add_profil
# define SYS_add_profil     176
#endif
#ifndef SYS_ATsocket
# define SYS_ATsocket       206
#endif
#ifndef SYS_ATgetmsg
# define SYS_ATgetmsg       207
#endif
#ifndef SYS_ATputmsg
# define SYS_ATputmsg       208
#endif
#ifndef SYS_ATPsndreq
# define SYS_ATPsndreq      209
#endif
#ifndef SYS_ATPsndrsp
# define SYS_ATPsndrsp      210
#endif
#ifndef SYS_ATPgetreq
# define SYS_ATPgetreq      211
#endif
#ifndef SYS_ATPgetrsp
# define SYS_ATPgetrsp      212
#endif
#ifndef SYS_mkcomplex
# define SYS_mkcomplex      216
#endif
#ifndef SYS_statv
# define SYS_statv          217
#endif
#ifndef SYS_lstatv
# define SYS_lstatv         218
#endif
#ifndef SYS_fstatv
# define SYS_fstatv         219
#endif
#ifndef SYS_getaudit
# define SYS_getaudit       355
#endif
#ifndef SYS_setaudit
# define SYS_setaudit       356
#endif
#ifndef SYS_pid_hibernate
# define SYS_pid_hibernate  435
#endif
#ifndef SYS_pid_shutdown_sockets
# define SYS_pid_shutdown_sockets 436
#endif

/* Syscalls changed in Yosemite */
#ifndef SYS___sysctl
# define SYS___sysctl SYS_sysctl
#endif
#ifndef SYS_sem_getvalue
/* XXX i#1659: Yosemite changed 274 from SYS_sem_getvalue to SYS_sysctlbyname
 * rather than simply deprecating the number.
 * We need to have a dynamic handler that swaps based on the underlying OS ver.
 */
# define SYS_sem_getvalue   274
#endif
#ifndef SYS_sem_init
# define SYS_sem_init       275
#endif
#ifndef SYS_sem_destroy
# define SYS_sem_destroy    276
#endif

/* Syscalls changed in El Capitan */
#ifndef SYS___mac_get_lcid
# define SYS___mac_get_lcid 391
#endif
#ifndef SYS___mac_get_lctx
# define SYS___mac_get_lctx 392
#endif
#ifndef SYS___mac_set_lctx
# define SYS___mac_set_lctx 393
#endif
#ifndef SYS_setlcid
# define SYS_setlcid        394
#endif
#ifndef SYS_getlcid
# define SYS_getlcid        395
#endif

/* Syscalls changed in Sierra */
#ifndef SYS_chud
# define SYS_chud           185
#endif
#ifndef SYS_stack_snapshot
# define SYS_stack_snapshot 365
#endif

#include "table_defines.h"

syscall_info_t syscall_info_bsd[] = {
    /* FIXME i#1440: this table was auto-generated, and the memory
     * parameters are all marked W.  We need to go through and update
     * each one.
     */
    {{SYS_exit /*1*/}, "exit", OK, DRSYS_TYPE_VOID, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fork /*2*/}, "fork", OK, RLONG, 0, },
    {{SYS_read /*3*/}, "read", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, -2, W},
         {1, RET, W},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_write /*4*/}, "write", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_open /*5*/}, "open", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         /* 3rd arg is optional: special-cased */
     }
    },
    {{SYS_close /*6*/}, "close", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_wait4 /*7*/}, "wait4", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_link /*9*/}, "link", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_unlink /*10*/}, "unlink", OK, RLONG, 1,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_chdir /*12*/}, "chdir", OK, RLONG, 1,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_fchdir /*13*/}, "fchdir", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_mknod /*14*/}, "mknod", OK, RLONG, 3,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_chmod /*15*/}, "chmod", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_chown /*16*/}, "chown", OK, RLONG, 3,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getfsstat /*18*/}, "getfsstat", OK, RLONG, 3,
     {
         {0, -1, W},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getpid /*20*/}, "getpid", OK, RLONG, 0, },
    {{SYS_setuid /*23*/}, "setuid", OK, RLONG, 1,
     {
         {0, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getuid /*24*/}, "getuid", OK, RLONG, 0, },
    {{SYS_geteuid /*25*/}, "geteuid", OK, RLONG, 0, },
    {{SYS_ptrace /*26*/}, "ptrace", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_recvmsg /*27*/}, "recvmsg", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct msghdr), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sendmsg /*28*/}, "sendmsg", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_recvfrom /*29*/}, "recvfrom", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(struct sockaddr), W|HT, DRSYS_TYPE_STRUCT},
         {5, sizeof(int), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_accept /*30*/}, "accept", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(socklen_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getpeername /*31*/}, "getpeername", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(socklen_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getsockname /*32*/}, "getsockname", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(socklen_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_access /*33*/}, "access", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_chflags /*34*/}, "chflags", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fchflags /*35*/}, "fchflags", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sync /*36*/}, "sync", OK, RLONG, 0, },
    {{SYS_kill /*37*/}, "kill", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getppid /*39*/}, "getppid", OK, RLONG, 0, },
    {{SYS_dup /*41*/}, "dup", OK, RLONG, 1,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_pipe /*42*/}, "pipe", OK, RLONG, 0, },
    {{SYS_getegid /*43*/}, "getegid", OK, RLONG, 0, },
    {{SYS_profil /*44*/}, "profil", OK, RLONG, 4,
     {
         {0, sizeof(short), W|HT, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_sigaction /*46*/}, "sigaction", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct __sigaction), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(struct sigaction), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_getgid /*47*/}, "getgid", OK, RLONG, 0, },
    {{SYS_sigprocmask /*48*/}, "sigprocmask", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_getlogin /*49*/}, "getlogin", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_setlogin /*50*/}, "setlogin", OK, RLONG, 1,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_acct /*51*/}, "acct", OK, RLONG, 1,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_sigpending /*52*/}, "sigpending", OK, RLONG, 1,
     {
         {0, sizeof(struct sigvec), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_sigaltstack /*53*/}, "sigaltstack", OK, RLONG, 2,
     {
         {0, sizeof(stack_t), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(stack_t), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_ioctl /*54*/}, "ioctl", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_reboot /*55*/}, "reboot", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_revoke /*56*/}, "revoke", OK, RLONG, 1,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_symlink /*57*/}, "symlink", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_readlink /*58*/}, "readlink", OK, RLONG, 3,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_execve /*59*/}, "execve", OK, RLONG, 3,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(char **), W|HT, DRSYS_TYPE_CSTRARRAY},
         {2, sizeof(char **), W|HT, DRSYS_TYPE_CSTRARRAY},
     }
    },
    {{SYS_umask /*60*/}, "umask", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_chroot /*61*/}, "chroot", OK, RLONG, 1,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_msync /*65*/}, "msync", OK, RLONG, 3,
     {
         {0, sizeof(caddr_t), SYSARG_INLINED, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_vfork /*66*/}, "vfork", OK, RLONG, 0, },
    {{SYS_munmap /*73*/}, "munmap", OK, RLONG, 2,
     {
         {0, sizeof(caddr_t), SYSARG_INLINED, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_mprotect /*74*/}, "mprotect", OK, RLONG, 3,
     {
         {0, sizeof(caddr_t), SYSARG_INLINED, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_madvise /*75*/}, "madvise", OK, RLONG, 3,
     {
         {0, sizeof(caddr_t), SYSARG_INLINED, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_mincore /*78*/}, "mincore", OK, RLONG, 3,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_getgroups /*79*/}, "getgroups", OK, RLONG, 2,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(gid_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setgroups /*80*/}, "setgroups", OK, RLONG, 2,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(gid_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getpgrp /*81*/}, "getpgrp", OK, RLONG, 0, },
    {{SYS_setpgid /*82*/}, "setpgid", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setitimer /*83*/}, "setitimer", OK, RLONG, 3,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(struct itimerval), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(struct itimerval), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_swapon /*85*/}, "swapon", OK, RLONG, 0, },
    {{SYS_getitimer /*86*/}, "getitimer", OK, RLONG, 2,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(struct itimerval), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_getdtablesize /*89*/}, "getdtablesize", OK, RLONG, 0, },
    {{SYS_dup2 /*90*/}, "dup2", OK, RLONG, 2,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_fcntl /*92*/}, "fcntl", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(long), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_select /*93*/}, "select", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(u_int32_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(u_int32_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(u_int32_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_fsync /*95*/}, "fsync", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setpriority /*96*/}, "setpriority", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(id_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_socket /*97*/}, "socket", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_connect /*98*/}, "connect", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(socklen_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getpriority /*100*/}, "getpriority", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(id_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_bind /*104*/}, "bind", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(socklen_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setsockopt /*105*/}, "setsockopt", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {4, sizeof(socklen_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_listen /*106*/}, "listen", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sigsuspend /*111*/}, "sigsuspend", OK, RLONG, 1,
     {
         {0, sizeof(sigset_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_gettimeofday /*116*/}, "gettimeofday", OK, RLONG, 2,
     {
         {0, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(struct timezone), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_getrusage /*117*/}, "getrusage", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct rusage), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_getsockopt /*118*/}, "getsockopt", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {4, sizeof(socklen_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_readv /*120*/}, "readv", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct iovec), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_writev /*121*/}, "writev", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct iovec), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_settimeofday /*122*/}, "settimeofday", OK, RLONG, 2,
     {
         {0, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(struct timezone), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_fchown /*123*/}, "fchown", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fchmod /*124*/}, "fchmod", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setreuid /*126*/}, "setreuid", OK, RLONG, 2,
     {
         {0, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setregid /*127*/}, "setregid", OK, RLONG, 2,
     {
         {0, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_rename /*128*/}, "rename", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_flock /*131*/}, "flock", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_mkfifo /*132*/}, "mkfifo", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sendto /*133*/}, "sendto", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {5, sizeof(socklen_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_shutdown /*134*/}, "shutdown", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_socketpair /*135*/}, "socketpair", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_mkdir /*136*/}, "mkdir", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_rmdir /*137*/}, "rmdir", OK, RLONG, 1,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_utimes /*138*/}, "utimes", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_futimes /*139*/}, "futimes", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_adjtime /*140*/}, "adjtime", OK, RLONG, 2,
     {
         {0, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_gethostuuid /*142*/}, "gethostuuid", OK, RLONG, 2,
     {
         {0, sizeof(unsigned char *), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(struct timespec), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_setsid /*147*/}, "setsid", OK, RLONG, 0, },
    {{SYS_getpgid /*151*/}, "getpgid", OK, RLONG, 1,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setprivexec /*152*/}, "setprivexec", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_pread /*153*/}, "pread", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_pwrite /*154*/}, "pwrite", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_nfssvc /*155*/}, "nfssvc", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_statfs /*157*/}, "statfs", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct statfs), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_fstatfs /*158*/}, "fstatfs", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct statfs), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_unmount /*159*/}, "unmount", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getfh /*161*/}, "getfh", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(fhandle_t), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_quotactl /*165*/}, "quotactl", OK, RLONG, 4,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_mount /*167*/}, "mount", OK, RLONG, 4,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_csops /*169*/}, "csops", OK, RLONG, 4,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_waitid /*173*/}, "waitid", OK, RLONG, 4,
     {
         {0, sizeof(idtype_t), SYSARG_INLINED, DRSYS_TYPE_STRUCT},
         {1, sizeof(id_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(siginfo_t), W|HT, DRSYS_TYPE_STRUCT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_add_profil /*176*/}, "add_profil", OK, RLONG, 4,
     {
         {0, sizeof(short), W|HT, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_kdebug_trace /*180*/}, "kdebug_trace", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {5, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setgid /*181*/}, "setgid", OK, RLONG, 1,
     {
         {0, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setegid /*182*/}, "setegid", OK, RLONG, 1,
     {
         {0, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_seteuid /*183*/}, "seteuid", OK, RLONG, 1,
     {
         {0, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sigreturn /*184*/}, "sigreturn", OK, RLONG, 2,
     {
         {0, sizeof(ucontext_t), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_chud /*185*/}, "chud", OK, RLONG, 6,
     {
         {0, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_fdatasync /*187*/}, "fdatasync", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_stat /*188*/}, "stat", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_fstat /*189*/}, "fstat", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_lstat /*190*/}, "lstat", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_pathconf /*191*/}, "pathconf", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fpathconf /*192*/}, "fpathconf", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getrlimit /*194*/}, "getrlimit", OK, RLONG, 2,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(struct rlimit), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_setrlimit /*195*/}, "setrlimit", OK, RLONG, 2,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(struct rlimit), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_getdirentries /*196*/}, "getdirentries", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, -2, W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(long), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_mmap /*197*/}, "mmap", OK, DRSYS_TYPE_POINTER, 6,
     {
         {0, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {5, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_lseek /*199*/}, "lseek", OK|SYSINFO_RET_64BIT, RLONG/*64-bit*/, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_truncate /*200*/}, "truncate", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_ftruncate /*201*/}, "ftruncate", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS___sysctl /*202*/}, "__sysctl", OK, RLONG, 6,
     {
         {0, -1, R|SYSARG_SIZE_IN_ELEMENTS, sizeof(int)},
         {1, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, -3, WI},
         {3, sizeof(size_t), R|W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {4, -5, R},
         {5, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_mlock /*203*/}, "mlock", OK, RLONG, 2,
     {
         {0, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_munlock /*204*/}, "munlock", OK, RLONG, 2,
     {
         {0, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_undelete /*205*/}, "undelete", OK, RLONG, 1,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_ATsocket /*206*/}, "ATsocket", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_ATgetmsg /*207*/}, "ATgetmsg", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(int), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_ATputmsg /*208*/}, "ATputmsg", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_ATPsndreq /*209*/}, "ATPsndreq", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(unsigned char *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_ATPsndrsp /*210*/}, "ATPsndrsp", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(unsigned char *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_ATPgetreq /*211*/}, "ATPgetreq", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(unsigned char *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_ATPgetrsp /*212*/}, "ATPgetrsp", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(unsigned char *), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_mkcomplex /*216*/}, "mkcomplex", OK, RLONG, 3,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(mode_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_statv /*217*/}, "statv", UNKNOWN, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         /* FIXME i#1440: sys/vstat.h is obsolete; may need own decl, or don't support */
     }
    },
    {{SYS_lstatv /*218*/}, "lstatv", UNKNOWN, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         /* FIXME i#1440: sys/vstat.h is obsolete; may need own decl, or don't support */
     }
    },
    {{SYS_fstatv /*219*/}, "fstatv", UNKNOWN, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         /* FIXME i#1440: sys/vstat.h is obsolete; may need own decl, or don't support */
     }
    },
    {{SYS_getattrlist /*220*/}, "getattrlist", OK, RLONG, 5,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct attrlist), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_setattrlist /*221*/}, "setattrlist", OK, RLONG, 5,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct attrlist), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_getdirentriesattr /*222*/}, "getdirentriesattr", OK, RLONG, 8,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct attrlist), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(u_long), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(u_long), W|HT, DRSYS_TYPE_UNSIGNED_INT},
#if !(defined(MACOS) && defined(X64))
         /* FIXME i#1438: how are 7th and 8th args passed on Mac64?!? */
         {6, sizeof(u_long), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {7, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#endif
     }
    },
    {{SYS_exchangedata /*223*/}, "exchangedata", OK, RLONG, 3,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_searchfs /*225*/}, "searchfs", OK, RLONG, 6,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct fssearchblock), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(uint32_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(struct searchstate), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_delete /*226*/}, "delete", OK, RLONG, 1,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_copyfile /*227*/}, "copyfile", OK, RLONG, 4,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fgetattrlist /*228*/}, "fgetattrlist", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct attrlist), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_fsetattrlist /*229*/}, "fsetattrlist", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct attrlist), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_poll /*230*/}, "poll", OK, RLONG, 3,
     {
         {0, sizeof(struct pollfd), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_watchevent /*231*/}, "watchevent", OK, RLONG, 2,
     {
         {0, sizeof(struct eventreq), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_waitevent /*232*/}, "waitevent", OK, RLONG, 2,
     {
         {0, sizeof(struct eventreq), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_modwatch /*233*/}, "modwatch", OK, RLONG, 2,
     {
         {0, sizeof(struct eventreq), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getxattr /*234*/}, "getxattr", OK, RLONG, 6,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fgetxattr /*235*/}, "fgetxattr", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setxattr /*236*/}, "setxattr", OK, RLONG, 6,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fsetxattr /*237*/}, "fsetxattr", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_removexattr /*238*/}, "removexattr", OK, RLONG, 3,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fremovexattr /*239*/}, "fremovexattr", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_listxattr /*240*/}, "listxattr", OK, RLONG, 4,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_flistxattr /*241*/}, "flistxattr", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fsctl /*242*/}, "fsctl", OK, RLONG, 4,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {3, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_initgroups /*243*/}, "initgroups", OK, RLONG, 3,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(gid_t), W|HT, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_posix_spawn /*244*/}, "posix_spawn", UNKNOWN, RLONG, 5,
     {
         {0, sizeof(pid_t), W|HT, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         /* FIXME i#1440: non-public struct in arg #2 => UNKNOWN above */
         {3, sizeof(char **), W|HT, DRSYS_TYPE_CSTRARRAY},
         {4, sizeof(char **), W|HT, DRSYS_TYPE_CSTRARRAY},
     }
    },
    {{SYS_ffsctl /*245*/}, "ffsctl", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(u_long), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {3, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_nfsclnt /*247*/}, "nfsclnt", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_fhopen /*248*/}, "fhopen", OK, RLONG, 2,
     {
         {0, sizeof(struct fhandle), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_minherit /*250*/}, "minherit", OK, RLONG, 3,
     {
         {0, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_semsys /*251*/}, "semsys", OK, RLONG, 5,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_msgsys /*252*/}, "msgsys", OK, RLONG, 5,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_shmsys /*253*/}, "shmsys", OK, RLONG, 4,
     {
         {0, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_semctl /*254*/}, "semctl", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(semun_t), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_semget /*255*/}, "semget", OK, RLONG, 3,
     {
         {0, sizeof(key_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_semop /*256*/}, "semop", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct sembuf), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_msgctl /*258*/}, "msgctl", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(struct	msqid_ds), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_msgget /*259*/}, "msgget", OK, RLONG, 2,
     {
         {0, sizeof(key_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_msgsnd /*260*/}, "msgsnd", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_msgrcv /*261*/}, "msgrcv", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(long), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_shmat /*262*/}, "shmat", OK, DRSYS_TYPE_POINTER, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_shmctl /*263*/}, "shmctl", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(struct shmid_ds), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_shmdt /*264*/}, "shmdt", OK, RLONG, 1,
     {
         {0, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_shmget /*265*/}, "shmget", OK, RLONG, 3,
     {
         {0, sizeof(key_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_shm_open /*266*/}, "shm_open", OK, RLONG, 3,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_shm_unlink /*267*/}, "shm_unlink", OK, RLONG, 1,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_sem_open /*268*/}, "sem_open", OK, DRSYS_TYPE_POINTER, 4,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sem_close /*269*/}, "sem_close", OK, RLONG, 1,
     {
         {0, sizeof(sem_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sem_unlink /*270*/}, "sem_unlink", OK, RLONG, 1,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_sem_wait /*271*/}, "sem_wait", OK, RLONG, 1,
     {
         {0, sizeof(sem_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sem_trywait /*272*/}, "sem_trywait", OK, RLONG, 1,
     {
         {0, sizeof(sem_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sem_post /*273*/}, "sem_post", OK, RLONG, 1,
     {
         {0, sizeof(sem_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sem_getvalue /*274*/}, "sem_getvalue", OK, RLONG, 2,
     {
         {0, sizeof(sem_t), W|HT, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sem_init /*275*/}, "sem_init", OK, RLONG, 3,
     {
         {0, sizeof(sem_t), W|HT, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_sem_destroy /*276*/}, "sem_destroy", OK, RLONG, 1,
     {
         {0, sizeof(sem_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_open_extended /*277*/}, "open_extended", OK, RLONG, 6,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {5, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_umask_extended /*278*/}, "umask_extended", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_stat_extended /*279*/}, "stat_extended", OK, RLONG, 4,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_lstat_extended /*280*/}, "lstat_extended", OK, RLONG, 4,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_fstat_extended /*281*/}, "fstat_extended", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_chmod_extended /*282*/}, "chmod_extended", OK, RLONG, 5,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_fchmod_extended /*283*/}, "fchmod_extended", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_access_extended /*284*/}, "access_extended", OK, RLONG, 4,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_settid /*285*/}, "settid", OK, RLONG, 2,
     {
         {0, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_gettid /*286*/}, "gettid", OK, RLONG, 2,
     {
         {0, sizeof(uid_t), W|HT, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(gid_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setsgroups /*287*/}, "setsgroups", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_getsgroups /*288*/}, "getsgroups", OK, RLONG, 2,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_setwgroups /*289*/}, "setwgroups", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_getwgroups /*290*/}, "getwgroups", OK, RLONG, 2,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_mkfifo_extended /*291*/}, "mkfifo_extended", OK, RLONG, 5,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_mkdir_extended /*292*/}, "mkdir_extended", OK, RLONG, 5,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_identitysvc /*293*/}, "identitysvc", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_shared_region_check_np /*294*/}, "shared_region_check_np", OK, RLONG, 1,
     {
         {0, sizeof(uint64_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_vm_pressure_monitor /*296*/}, "vm_pressure_monitor", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(uint32_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_psynch_rw_longrdlock /*297*/}, "psynch_rw_longrdlock", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_psynch_rw_yieldwrlock /*298*/}, "psynch_rw_yieldwrlock", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_psynch_rw_downgrade /*299*/}, "psynch_rw_downgrade", OK, RLONG, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_psynch_rw_upgrade /*300*/}, "psynch_rw_upgrade", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_psynch_mutexwait /*301*/}, "psynch_mutexwait", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_psynch_mutexdrop /*302*/}, "psynch_mutexdrop", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_psynch_cvbroad /*303*/}, "psynch_cvbroad", OK, DRSYS_TYPE_UNSIGNED_INT, 7,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {5, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#if !(defined(MACOS) && defined(X64))
         /* FIXME i#1438: how are 7th and 8th args passed on Mac64?!? */
         {6, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#endif
     }
    },
    {{SYS_psynch_cvsignal /*304*/}, "psynch_cvsignal", OK, DRSYS_TYPE_UNSIGNED_INT, 8,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {5, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#if !(defined(MACOS) && defined(X64))
         /* FIXME i#1438: how are 7th and 8th args passed on Mac64?!? */
         {6, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {7, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#endif
     }
    },
    {{SYS_psynch_cvwait /*305*/}, "psynch_cvwait", OK, DRSYS_TYPE_UNSIGNED_INT, 8,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {4, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#if !(defined(MACOS) && defined(X64))
         /* FIXME i#1438: how are 7th and 8th args passed on Mac64?!? */
         {6, sizeof(int64_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {7, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#endif
     }
    },
    {{SYS_psynch_rw_rdlock /*306*/}, "psynch_rw_rdlock", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_psynch_rw_wrlock /*307*/}, "psynch_rw_wrlock", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_psynch_rw_unlock /*308*/}, "psynch_rw_unlock", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_psynch_rw_unlock2 /*309*/}, "psynch_rw_unlock2", OK, DRSYS_TYPE_UNSIGNED_INT, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getsid /*310*/}, "getsid", OK, RLONG, 1,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_settid_with_pid /*311*/}, "settid_with_pid", OK, RLONG, 2,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_psynch_cvclrprepost /*312*/}, "psynch_cvclrprepost", OK, RLONG, 7,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#if !(defined(MACOS) && defined(X64))
         /* FIXME i#1438: how are 7th and 8th args passed on Mac64?!? */
         {6, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#endif
     }
    },
    {{SYS_aio_fsync /*313*/}, "aio_fsync", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_aio_return /*314*/}, "aio_return", OK, RLONG, 1,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_aio_suspend /*315*/}, "aio_suspend", OK, RLONG, 3,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_aio_cancel /*316*/}, "aio_cancel", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_aio_error /*317*/}, "aio_error", OK, RLONG, 1,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_aio_read /*318*/}, "aio_read", OK, RLONG, 1,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_aio_write /*319*/}, "aio_write", OK, RLONG, 1,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_lio_listio /*320*/}, "lio_listio", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_iopolicysys /*322*/}, "iopolicysys", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_process_policy /*323*/}, "process_policy", OK, RLONG, 7,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {5, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
#if !(defined(MACOS) && defined(X64))
         /* FIXME i#1438: how are 7th and 8th args passed on Mac64?!? */
         {6, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#endif
     }
    },
    {{SYS_mlockall /*324*/}, "mlockall", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_munlockall /*325*/}, "munlockall", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_issetugid /*327*/}, "issetugid", OK, RLONG, 0, },
    {{SYS___pthread_kill /*328*/}, "__pthread_kill", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS___pthread_sigmask /*329*/}, "__pthread_sigmask", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS___sigwait /*330*/}, "__sigwait", OK, RLONG, 2,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS___disable_threadsignal /*331*/}, "__disable_threadsignal", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS___pthread_markcancel /*332*/}, "__pthread_markcancel", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS___pthread_canceled /*333*/}, "__pthread_canceled", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS___semwait_signal /*334*/}, "__semwait_signal", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int64_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {5, sizeof(int32_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_proc_info /*336*/}, "proc_info", OK, RLONG, 6,
     {
         {0, sizeof(int32_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int32_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {5, sizeof(int32_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sendfile /*337*/}, "sendfile", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(off_t), W|HT, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(struct sf_hdtr), W|HT, DRSYS_TYPE_STRUCT},
         {5, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_stat64 /*338*/}, "stat64", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct stat64), W},
     }
    },
    {{SYS_fstat64 /*339*/}, "fstat64", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct stat64), W},
     }
    },
    {{SYS_lstat64 /*340*/}, "lstat64", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_stat64_extended /*341*/}, "stat64_extended", OK, RLONG, 4,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_lstat64_extended /*342*/}, "lstat64_extended", OK, RLONG, 4,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_fstat64_extended /*343*/}, "fstat64_extended", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_getdirentries64 /*344*/}, "getdirentries64", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, -2, W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(off_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_statfs64 /*345*/}, "statfs64", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct statfs64), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_fstatfs64 /*346*/}, "fstatfs64", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct statfs64), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_getfsstat64 /*347*/}, "getfsstat64", OK, RLONG, 3,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS___pthread_chdir /*348*/}, "__pthread_chdir", OK, RLONG, 1,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS___pthread_fchdir /*349*/}, "__pthread_fchdir", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_audit /*350*/}, "audit", OK, RLONG, 2,
     {
         {0, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_auditon /*351*/}, "auditon", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getauid /*353*/}, "getauid", OK, RLONG, 1,
     {
         {0, sizeof(au_id_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setauid /*354*/}, "setauid", OK, RLONG, 1,
     {
         {0, sizeof(au_id_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getaudit /*355*/}, "getaudit", OK, RLONG, 1,
     {
         {0, sizeof(struct auditinfo), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_setaudit /*356*/}, "setaudit", OK, RLONG, 1,
     {
         {0, sizeof(struct auditinfo), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_getaudit_addr /*357*/}, "getaudit_addr", OK, RLONG, 2,
     {
         {0, sizeof(struct auditinfo_addr), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_setaudit_addr /*358*/}, "setaudit_addr", OK, RLONG, 2,
     {
         {0, sizeof(struct auditinfo_addr), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_auditctl /*359*/}, "auditctl", OK, RLONG, 1,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
     }
    },
    {{SYS_bsdthread_create /*360*/}, "bsdthread_create", OK, DRSYS_TYPE_POINTER, 5,
     {
         {0, sizeof(void*), SYSARG_INLINED, DRSYS_TYPE_POINTER},
         {1, sizeof(void*), SYSARG_INLINED, DRSYS_TYPE_VOID},
         {2, sizeof(void*), SYSARG_INLINED, DRSYS_TYPE_POINTER},
         {3, sizeof(void*), SYSARG_INLINED, DRSYS_TYPE_POINTER},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_bsdthread_terminate /*361*/}, "bsdthread_terminate", OK, RLONG, 4,
     {
         {0, sizeof(void*), SYSARG_INLINED, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_kqueue /*362*/}, "kqueue", OK, RLONG, 0, },
    {{SYS_kevent /*363*/}, "kevent", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct kevent), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(struct kevent), W|HT, DRSYS_TYPE_STRUCT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {5, sizeof(struct timespec), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_lchown /*364*/}, "lchown", OK, RLONG, 3,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(uid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(gid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_stack_snapshot /*365*/}, "stack_snapshot", OK, RLONG, 5,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_bsdthread_register /*366*/}, "bsdthread_register", OK, RLONG, 6,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {4, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {5, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_workq_open /*367*/}, "workq_open", OK, RLONG, 0, },
    {{SYS_workq_kernreturn /*368*/}, "workq_kernreturn", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_kevent64 /*369*/}, "kevent64", OK, RLONG, 7,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct kevent64_s), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(struct kevent64_s), W|HT, DRSYS_TYPE_STRUCT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {5, sizeof(unsigned int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
#if !(defined(MACOS) && defined(X64))
         /* FIXME i#1438: how are 7th and 8th args passed on Mac64?!? */
         {6, sizeof(struct timespec), W|HT, DRSYS_TYPE_STRUCT},
#endif
     }
    },
    {{SYS___old_semwait_signal /*370*/}, "__old_semwait_signal", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(struct timespec), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___old_semwait_signal_nocancel /*371*/}, "__old_semwait_signal_nocancel", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(struct timespec), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_thread_selfid /*372*/}, "thread_selfid", OK|SYSINFO_RET_64BIT, DRSYS_TYPE_UNSIGNED_INT, 0, },
    {{SYS___mac_execve /*380*/}, "__mac_execve", OK, RLONG, 4,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(char **), W|HT, DRSYS_TYPE_CSTRARRAY},
         {2, sizeof(char **), W|HT, DRSYS_TYPE_CSTRARRAY},
         {3, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_syscall /*381*/}, "__mac_syscall", OK, RLONG, 3,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS___mac_get_file /*382*/}, "__mac_get_file", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_set_file /*383*/}, "__mac_set_file", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_get_link /*384*/}, "__mac_get_link", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_set_link /*385*/}, "__mac_set_link", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_get_proc /*386*/}, "__mac_get_proc", OK, RLONG, 1,
     {
         {0, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_set_proc /*387*/}, "__mac_set_proc", OK, RLONG, 1,
     {
         {0, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_get_fd /*388*/}, "__mac_get_fd", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_set_fd /*389*/}, "__mac_set_fd", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_get_pid /*390*/}, "__mac_get_pid", OK, RLONG, 2,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_get_lcid /*391*/}, "__mac_get_lcid", OK, RLONG, 2,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_get_lctx /*392*/}, "__mac_get_lctx", OK, RLONG, 1,
     {
         {0, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_set_lctx /*393*/}, "__mac_set_lctx", OK, RLONG, 1,
     {
         {0, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_setlcid /*394*/}, "setlcid", OK, RLONG, 2,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_getlcid /*395*/}, "getlcid", OK, RLONG, 1,
     {
         {0, sizeof(pid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_read_nocancel /*396*/}, "read_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, -2, W},
         {1, RET, W},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_write_nocancel /*397*/}, "write_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, -2, R},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_open_nocancel /*398*/}, "open_nocancel", OK, RLONG, 2,
     {
         {0, 0, R|CT, DRSYS_TYPE_CSTRING},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         /* 3rd arg is optional: special-cased */
     }
    },
    {{SYS_close_nocancel /*399*/}, "close_nocancel", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_wait4_nocancel /*400*/}, "wait4_nocancel", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_recvmsg_nocancel /*401*/}, "recvmsg_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct msghdr), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sendmsg_nocancel /*402*/}, "sendmsg_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_recvfrom_nocancel /*403*/}, "recvfrom_nocancel", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(struct sockaddr), W|HT, DRSYS_TYPE_STRUCT},
         {5, sizeof(int), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_accept_nocancel /*404*/}, "accept_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(socklen_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_msync_nocancel /*405*/}, "msync_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fcntl_nocancel /*406*/}, "fcntl_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(long), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_select_nocancel /*407*/}, "select_nocancel", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(u_int32_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(u_int32_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(u_int32_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(struct timeval), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS_fsync_nocancel /*408*/}, "fsync_nocancel", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_connect_nocancel /*409*/}, "connect_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(socklen_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sigsuspend_nocancel /*410*/}, "sigsuspend_nocancel", OK, RLONG, 1,
     {
         {0, sizeof(sigset_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_readv_nocancel /*411*/}, "readv_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct iovec), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_writev_nocancel /*412*/}, "writev_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(struct iovec), W|HT, DRSYS_TYPE_STRUCT},
         {2, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_sendto_nocancel /*413*/}, "sendto_nocancel", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {5, sizeof(socklen_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_pread_nocancel /*414*/}, "pread_nocancel", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_pwrite_nocancel /*415*/}, "pwrite_nocancel", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(off_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_waitid_nocancel /*416*/}, "waitid_nocancel", OK, RLONG, 4,
     {
         {0, sizeof(idtype_t), SYSARG_INLINED, DRSYS_TYPE_STRUCT},
         {1, sizeof(id_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(siginfo_t), W|HT, DRSYS_TYPE_STRUCT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_poll_nocancel /*417*/}, "poll_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(struct pollfd), W|HT, DRSYS_TYPE_STRUCT},
         {1, sizeof(u_int), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_msgsnd_nocancel /*418*/}, "msgsnd_nocancel", OK, RLONG, 4,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_msgrcv_nocancel /*419*/}, "msgrcv_nocancel", OK, RLONG, 5,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void *), W|HT, DRSYS_TYPE_POINTER},
         {2, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {3, sizeof(long), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_sem_wait_nocancel /*420*/}, "sem_wait_nocancel", OK, RLONG, 1,
     {
         {0, sizeof(sem_t), W|HT, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_aio_suspend_nocancel /*421*/}, "aio_suspend_nocancel", OK, RLONG, 3,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS___sigwait_nocancel /*422*/}, "__sigwait_nocancel", OK, RLONG, 2,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS___semwait_signal_nocancel /*423*/}, "__semwait_signal_nocancel", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int64_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {5, sizeof(int32_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS___mac_mount /*424*/}, "__mac_mount", OK, RLONG, 5,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {2, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {3, sizeof(caddr_t), W|HT, DRSYS_TYPE_CSTRING},
         {4, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_get_mount /*425*/}, "__mac_get_mount", OK, RLONG, 2,
     {
         {0, sizeof(char *), W|HT, DRSYS_TYPE_CSTRING},
         {1, sizeof(struct mac), W|HT, DRSYS_TYPE_STRUCT},
     }
    },
    {{SYS___mac_getfsstat /*426*/}, "__mac_getfsstat", OK, RLONG, 5,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {4, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_fsgetpath /*427*/}, "fsgetpath", OK, RLONG, 4,
     {
         {0, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {1, sizeof(size_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
         {3, sizeof(uint64_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_audit_session_self /*428*/}, "audit_session_self", OK, DRSYS_TYPE_UNSIGNED_INT, 0, },
    {{SYS_audit_session_join /*429*/}, "audit_session_join", OK, RLONG, 1,
     {
         {0, sizeof(mach_port_name_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_fileport_makeport /*430*/}, "fileport_makeport", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_fileport_makefd /*431*/}, "fileport_makefd", OK, RLONG, 1,
     {
         {0, sizeof(mach_port_name_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
    {{SYS_audit_session_port /*432*/}, "audit_session_port", OK, RLONG, 2,
     {
         {0, sizeof(au_asid_t), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(void*), W|HT, DRSYS_TYPE_POINTER},
     }
    },
    {{SYS_pid_suspend /*433*/}, "pid_suspend", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_pid_resume /*434*/}, "pid_resume", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_pid_hibernate /*435*/}, "pid_hibernate", OK, RLONG, 1,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_pid_shutdown_sockets /*436*/}, "pid_shutdown_sockets", OK, RLONG, 2,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
     }
    },
    {{SYS_shared_region_map_and_slide_np /*438*/}, "shared_region_map_and_slide_np", OK, RLONG, 6,
     {
         {0, sizeof(int), SYSARG_INLINED, DRSYS_TYPE_SIGNED_INT},
         {1, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {2, sizeof(struct shared_file_mapping_np), W|HT, DRSYS_TYPE_STRUCT},
         {3, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
         {4, sizeof(uint64_t), W|HT, DRSYS_TYPE_UNSIGNED_INT},
         {5, sizeof(uint32_t), SYSARG_INLINED, DRSYS_TYPE_UNSIGNED_INT},
     }
    },
};

size_t count_syscall_info_bsd = sizeof(syscall_info_bsd)/sizeof(syscall_info_bsd[0]);
