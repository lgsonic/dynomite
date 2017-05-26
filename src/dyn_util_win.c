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

#include "dyn_util_win.h"

void
sleep(int seconds)
{
	Sleep(seconds * 1000);
}

void
usleep(__int64 usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

int
gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const time_t EPOCH = ((time_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	time_t      time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((time_t)file_time.dwLowDateTime);
	time += ((time_t)file_time.dwHighDateTime) << 32;
	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

	return 0;
}

char *strndup(char *str, int chars)
{
	char *buffer;
	int n;

	buffer = (char *)malloc(chars + 1);
	if (buffer)
	{
		for (n = 0; ((n < chars) && (str[n] != 0)); n++) buffer[n] = str[n];
		buffer[n] = 0;
	}

	return buffer;
}

int pthread_create(pthread_t *tid, void *attr, void(*func)(void *), void *arg)
{
	*tid = _beginthread(func, 0, arg);
	return *tid;
}

ssize_t
writev(int fd, struct iovec const* vector, int count)
{
	int             i;
	size_t          total;
	void*           pv;
	ssize_t         ret;

	for (i = 0, total = 0; i < count; ++i)
	{
		total += vector[i].iov_len;
	}

	pv = malloc(total);

	if (pv == NULL)
	{
		ret = -1;
	}
	else
	{
		for (i = 0, ret = 0; i < count; ++i)
		{
			memcpy((char*)pv + ret, vector[i].iov_base, vector[i].iov_len);
			ret += (ssize_t)vector[i].iov_len;
		}

		ret = send(fd, pv, total, 0);

		free(pv);
	}

	return ret;
}

static void
usage_to_timeval(FILETIME *ft, struct timeval *tv)
{
	ULARGE_INTEGER time;
	time.LowPart = ft->dwLowDateTime;
	time.HighPart = ft->dwHighDateTime;

	tv->tv_sec = (long)(time.QuadPart / 10000000);
	tv->tv_usec = (time.QuadPart % 10000000) / 10;
}

int
getrusage(int who, struct rusage *usage)
{
	FILETIME creation_time, exit_time, kernel_time, user_time;
	PROCESS_MEMORY_COUNTERS pmc;

	memset(usage, 0, sizeof(struct rusage));

	if (who == RUSAGE_SELF)
	{
		if (!GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time))
		{
			return -1;
		}

		if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		{
			return -1;
		}

		usage_to_timeval(&kernel_time, &usage->ru_stime);
		usage_to_timeval(&user_time, &usage->ru_utime);
		usage->ru_majflt = pmc.PageFaultCount;
		usage->ru_maxrss = pmc.PeakWorkingSetSize / 1024;
		return 0;
	}
	else if (who == RUSAGE_THREAD)
	{
		if (!GetThreadTimes(GetCurrentThread(), &creation_time, &exit_time, &kernel_time, &user_time))
		{
			return -1;
		}

		usage_to_timeval(&kernel_time, &usage->ru_stime);
		usage_to_timeval(&user_time, &usage->ru_utime);
		return 0;
	}
	else
	{
		return -1;
	}
}

const char *
socket_strerror(err)
{
	const char *message = NULL;
	if (err >= sys_nerr)
	{
		static char *buffer = NULL;
		if (buffer != NULL)
			LocalFree(buffer);
		if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL, err, 0, (LPTSTR)&buffer, 0, NULL) != 0)
		{
			char* p = strchr(buffer, '\r');
			if (p != NULL)
				*p = '\0';
		}
		message = buffer;
	}
	if (message == NULL)
		message = strerror(err);
	return message;
}
