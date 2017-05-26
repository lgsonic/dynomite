/*
 * Dynomite - A thin, distributed replication layer for multi non-distributed storages.
 * Copyright (C) 2014 Netflix, Inc.
 */ 

/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _DYN_UTIL_WIN_H_
#define _DYN_UTIL_WIN_H_

#pragma warning(disable:4996 4090 4098 4703)

#define FD_SETSIZE 1024

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <psapi.h>
#include <process.h>
#include <io.h>
#include <resolv.h>

typedef unsigned char u_char;
typedef unsigned short u_int16_t;
typedef unsigned int pid_t;
typedef unsigned int pthread_t;
typedef SSIZE_T ssize_t;
typedef time_t suseconds_t;

#define inline __inline
#define snprintf _snprintf
#define strncasecmp _strnicmp
#define random rand
#define open _open
#define close closesocket
#define sockaddr_un sockaddr_in
#define SSIZE_MAX SIZE_MAX
#define F_OK 0
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define C_IN ns_c_in
#define T_TXT ns_t_txt
#undef errno
#define errno WSAGetLastError()
#define VERSION "v0.5.8"


void sleep(int seconds);

void usleep(__int64 usec);

int gettimeofday(struct timeval * tp, struct timezone * tzp);

char *strndup(char *str, int chars);

int pthread_create(pthread_t *tid, void *attr, void(*func)(void *), void *arg);

#define IOV_MAX 1024
struct iovec
{
	void*   iov_base;
	size_t  iov_len;
};

ssize_t writev(int fd, struct iovec const* vector, int count);

#define	RUSAGE_SELF	0
#define	RUSAGE_CHILDREN	(-1)
#define RUSAGE_BOTH	(-2)
#define	RUSAGE_THREAD	1	

struct rusage {
	struct timeval ru_utime; /* user CPU time used */
	struct timeval ru_stime; /* system CPU time used */
	long   ru_maxrss;        /* maximum resident set size */
	long   ru_ixrss;         /* integral shared memory size */
	long   ru_idrss;         /* integral unshared data size */
	long   ru_isrss;         /* integral unshared stack size */
	long   ru_minflt;        /* page reclaims (soft page faults) */
	long   ru_majflt;        /* page faults (hard page faults) */
	long   ru_nswap;         /* swaps */
	long   ru_inblock;       /* block input operations */
	long   ru_oublock;       /* block output operations */
	long   ru_msgsnd;        /* IPC messages sent */
	long   ru_msgrcv;        /* IPC messages received */
	long   ru_nsignals;      /* signals received */
	long   ru_nvcsw;         /* voluntary context switches */
	long   ru_nivcsw;        /* involuntary context switches */
};

int getrusage(int who, struct rusage *usage);

const char* socket_strerror(err);

#endif
