#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <winbase.h>

#include <resolv.h>


time_t FileTime2TimeT( const FILETIME* ft )
{
	__int64 t = *(__int64*)ft;
	t /= 10000000;		// get rid of 100 nanosecond units
	t -= 11644473600;	// shift epoch from 1601 to 1970;
	return (time_t)t;
}

struct timeval FileTime2TimeVal( const FILETIME* ft )
{
	// 11644473600 shift in seconds epoch from 1601 to 1970;
	struct timeval tv;
	__int64* t = (__int64*)ft;
	tv.tv_usec = (long)((*t % 10000000) / 10);
	tv.tv_sec = (long)(*t / 10000000 - 11644473600);
	return tv;
}

struct timeval evNowTime() 
{
	FILETIME ft;
	GetSystemTimeAsFileTime( &ft );
	return FileTime2TimeVal( &ft );
}


struct timeval evConsTime(time_t sec, long usec)
{
	struct timeval tv;
	tv.tv_sec = (long)sec;
	tv.tv_usec = usec;
	return tv;
}

struct timeval evAddTime(struct timeval addend1, struct timeval addend2)
{
	struct timeval x;
	x.tv_sec = addend1.tv_sec + addend2.tv_sec;
	x.tv_usec = addend1.tv_usec + addend2.tv_usec;
	if (x.tv_usec >= 1000000) {
		x.tv_sec++;
		x.tv_usec -= 1000000;
	}
	return (x);
}

struct timeval evSubTime(struct timeval minuend, struct timeval subtrahend) 
{
	struct timeval x;
	x.tv_sec = minuend.tv_sec - subtrahend.tv_sec;
	if (minuend.tv_usec >= subtrahend.tv_usec)
		x.tv_usec = minuend.tv_usec - subtrahend.tv_usec;
	else {
		x.tv_usec = 1000000 - subtrahend.tv_usec + minuend.tv_usec;
		x.tv_sec--;
	}
	return (x);
}

int evCmpTime(struct timeval a, struct timeval b)
{
	long x = a.tv_sec - b.tv_sec;

	if (x == 0L)
		x = a.tv_usec - b.tv_usec;
	return (x < 0L ? (-1) : x > 0L ? (1) : (0));
}
