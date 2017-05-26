/*
 * ++Copyright++ 1991, 1993
 * -
 * Copyright (c) 1991, 1993
 *    The Regents of the University of California.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

/*
 *	@(#)cdefs.h	8.1 (Berkeley) 6/2/93
 *	$Id: cdefs.h,v 1.1.150.2 2004/07/19 05:53:39 marka Exp $
 */

#ifndef	_CDEFS_H_
#define	_CDEFS_H_

#if defined(__cplusplus)
#define	__BEGIN_DECLS	extern "C" {
#define	__END_DECLS	};
#else
#define	__BEGIN_DECLS
#define	__END_DECLS
#endif

#include <time.h>
#include <errno.h>
#include <assert.h>

#if _MSC_VER <= 1400
#define ENOTSOCK		WSAENOTSOCK
#define EPROTONOSUPPORT	WSAEPROTONOSUPPORT
#define EAFNOSUPPORT	WSAEAFNOSUPPORT
#define ECONNRESET		WSAECONNRESET
#define EMSGSIZE		WSAEMSGSIZE
#define ETIMEDOUT		WSAETIMEDOUT
#define ECONNREFUSED	WSAECONNREFUSED
#define EDESTADDRREQ	WSAEDESTADDRREQ
#define EPROTOTYPE		WSAEPROTOTYPE
#endif

/*
 * The __CONCAT macro is used to concatenate parts of symbol names, e.g.
 * with "#define OLD(foo) __CONCAT(old,foo)", OLD(foo) produces oldfoo.
 * The __CONCAT macro is a bit tricky -- make sure you don't put spaces
 * in between its arguments.  __CONCAT can also concatenate double-quoted
 * strings produced by the __STRING macro, but this only works with ANSI C.
 */
#if defined(__STDC__) || defined(__cplusplus)
#define	__P(protos)	protos		/* full-blown ANSI C */
#define	__CONCAT(x,y)	x ## y
#define	__STRING(x)	#x

#define	__const		const		/* define reserved names to standard */
#define	__signed	signed
#if _MSC_VER <= 1400
#define	__volatile	volatile
#endif
#if defined(__cplusplus)
#define	__inline	inline		/* convert to C++ keyword */
#else
#define	__inline			/* delete GCC keyword */
#endif /* !__cplusplus */
#endif	/* !(__STDC__ || __cplusplus) */

#define ISC_FORMAT_PRINTF(fmt, args)
#define INSIST(x) assert(x)

//typedef /*signed*/ char            int8_t;
typedef signed char            int8_t;
typedef unsigned char            u_int8_t;
typedef short                     int16_t;
typedef unsigned short          u_int16_t;
typedef int                       int32_t;
typedef unsigned int            u_int32_t;

struct timeval evNowTime(void);
struct timeval evConsTime(time_t sec, long usec);
struct timeval evAddTime(struct timeval, struct timeval);
struct timeval evSubTime(struct timeval, struct timeval);
int            evCmpTime(struct timeval, struct timeval);


#endif /* !_CDEFS_H_ */
