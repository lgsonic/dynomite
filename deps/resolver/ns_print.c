/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef lint
static const char rcsid[] = "$Id: ns_print.c,v 1.3.2.1.4.7 2004/09/16 07:01:12 marka Exp $";
#endif


#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <resolv.h>
#include <arpa\inet.h>

#ifdef SPRINTF_CHAR
# define SPRINTF(x) strlen(sprintf/**/x)
#else
# define SPRINTF(x) ((size_t)sprintf x)
#endif

/* Forward. */

static size_t	prune_origin(const char *name, const char *origin);
static int	charstr(const u_char *rdata, const u_char *edata,
			char **buf, size_t *buflen);
static ptrdiff_t	addname(const u_char *msg, size_t msglen,
			const u_char **p, const char *origin,
			char **buf, size_t *buflen);
static void	addlen(size_t len, char **buf, size_t *buflen);
static int	addstr(const char *src, size_t len,
		       char **buf, size_t *buflen);
static int	addtab(size_t len, size_t target, int spaced,
		       char **buf, size_t *buflen);

/* Macros. */

#define	VALIDATE(x) do { if ((x) < 0) return (-1); } while (0)

/* Public. */

/*
 * int
 * ns_sprintrr(handle, rr, name_ctx, origin, buf, buflen)
 *	Convert an RR to presentation format.
 * return:
 *	Number of characters written to buf, or -1 (check errno).
 */
int
ns_sprintrr(const ns_msg *handle, const ns_rr *rr,
	    const char *name_ctx, const char *origin,
	    char *buf, size_t buflen)
{
	int n;

	n = ns_sprintrrf(ns_msg_base(*handle), ns_msg_size(*handle),
			 ns_rr_name(*rr), ns_rr_class(*rr), ns_rr_type(*rr),
			 ns_rr_ttl(*rr), ns_rr_rdata(*rr), ns_rr_rdlen(*rr),
			 name_ctx, origin, buf, buflen);
	return (n);
}

/*
 * int
 * ns_sprintrrf(msg, msglen, name, class, type, ttl, rdata, rdlen,
 *	       name_ctx, origin, buf, buflen)
 *	Convert the fields of an RR into presentation format.
 * return:
 *	Number of characters written to buf, or -1 (check errno).
 */
int
ns_sprintrrf(const u_char *msg, size_t msglen,
	    const char *name, ns_class class, ns_type type,
	    u_long ttl, const u_char *rdata, size_t rdlen,
	    const char *name_ctx, const char *origin,
	    char *buf, size_t buflen)
{
	const char *obuf = buf;
	const u_char *edata = rdata + rdlen;
	int spaced = 0;

	const char *comment;
	char tmp[100];
	ptrdiff_t len;
	int x;

	/*
	 * Owner.
	 */
	if (name_ctx != NULL && ns_samename(name_ctx, name) == 1) {
		VALIDATE(addstr("\t\t\t", 3, &buf, &buflen));
	} else {
		len = prune_origin(name, origin);
		if (*name == '\0') {
			goto root;
		} else if (len == 0) {
			VALIDATE(addstr("@\t\t\t", 4, &buf, &buflen));
		} else {
			VALIDATE(addstr(name, len, &buf, &buflen));
			/* Origin not used or not root, and no trailing dot? */
			if (((origin == NULL || origin[0] == '\0') ||
			    (origin[0] != '.' && origin[1] != '\0' &&
			    name[len] == '\0')) && name[len - 1] != '.') {
 root:
				VALIDATE(addstr(".", 1, &buf, &buflen));
				len++;
			}
			VALIDATE(spaced = addtab(len, 24, spaced, &buf, &buflen));
		}
	}

	/*
	 * TTL, Class, Type.
	 */
	VALIDATE(x = ns_format_ttl(ttl, buf, buflen));
	addlen(x, &buf, &buflen);
	len = SPRINTF((tmp, " %s %s", p_class(class), p_type(type)));
	VALIDATE(addstr(tmp, len, &buf, &buflen));
	VALIDATE(spaced = addtab(x + len, 16, spaced, &buf, &buflen));

	/*
	 * RData.
	 */
	switch (type) {
	case ns_t_a:
		if (rdlen != (size_t)NS_INADDRSZ)
			goto formerr;
		(void) inet_ntop(AF_INET, (u_char*)rdata, buf, buflen);
		addlen(strlen(buf), &buf, &buflen);
		break;

	case ns_t_cname:
	case ns_t_mb:
	case ns_t_mg:
	case ns_t_mr:
	case ns_t_ns:
	case ns_t_ptr:
	case ns_t_dname:
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		break;

	case ns_t_hinfo:
	case ns_t_isdn:
		/* First word. */
		VALIDATE(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		VALIDATE(addstr(" ", 1, &buf, &buflen));

		    
		/* Second word, optional in ISDN records. */
		if (type == ns_t_isdn && rdata == edata)
			break;
		    
		VALIDATE(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		break;

	case ns_t_soa: {
		u_long t;

		/* Server name. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		VALIDATE(addstr(" ", 1, &buf, &buflen));

		/* Administrator name. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		VALIDATE(addstr(" (\n", 3, &buf, &buflen));
		spaced = 0;

		if ((edata - rdata) != 5*NS_INT32SZ)
			goto formerr;

		/* Serial number. */
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		VALIDATE(addstr("\t\t\t\t\t", 5, &buf, &buflen));
		len = SPRINTF((tmp, "%lu", t));
		VALIDATE(addstr(tmp, len, &buf, &buflen));
		VALIDATE(spaced = addtab(len, 16, spaced, &buf, &buflen));
		VALIDATE(addstr("; serial\n", 9, &buf, &buflen));
		spaced = 0;

		/* Refresh interval. */
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		VALIDATE(addstr("\t\t\t\t\t", 5, &buf, &buflen));
		VALIDATE(len = ns_format_ttl(t, buf, buflen));
		addlen(len, &buf, &buflen);
		VALIDATE(spaced = addtab(len, 16, spaced, &buf, &buflen));
		VALIDATE(addstr("; refresh\n", 10, &buf, &buflen));
		spaced = 0;

		/* Retry interval. */
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		VALIDATE(addstr("\t\t\t\t\t", 5, &buf, &buflen));
		VALIDATE(len = ns_format_ttl(t, buf, buflen));
		addlen(len, &buf, &buflen);
		VALIDATE(spaced = addtab(len, 16, spaced, &buf, &buflen));
		VALIDATE(addstr("; retry\n", 8, &buf, &buflen));
		spaced = 0;

		/* Expiry. */
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		VALIDATE(addstr("\t\t\t\t\t", 5, &buf, &buflen));
		VALIDATE(len = ns_format_ttl(t, buf, buflen));
		addlen(len, &buf, &buflen);
		VALIDATE(spaced = addtab(len, 16, spaced, &buf, &buflen));
		VALIDATE(addstr("; expiry\n", 9, &buf, &buflen));
		spaced = 0;

		/* Minimum TTL. */
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		VALIDATE(addstr("\t\t\t\t\t", 5, &buf, &buflen));
		VALIDATE(len = ns_format_ttl(t, buf, buflen));
		addlen(len, &buf, &buflen);
		VALIDATE(addstr(" )", 2, &buf, &buflen));
		VALIDATE(spaced = addtab(len, 16, spaced, &buf, &buflen));
		VALIDATE(addstr("; minimum\n", 10, &buf, &buflen));

		break;
	    }

	case ns_t_mx:
	case ns_t_afsdb:
	case ns_t_rt: {
		u_int t;

		if (rdlen < (size_t)NS_INT16SZ)
			goto formerr;

		/* Priority. */
		t = ns_get16(rdata);
		rdata += NS_INT16SZ;
		len = SPRINTF((tmp, "%u ", t));
		VALIDATE(addstr(tmp, len, &buf, &buflen));

		/* Target. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		break;
	    }

	case ns_t_px: {
		u_int t;

		if (rdlen < (size_t)NS_INT16SZ)
			goto formerr;

		/* Priority. */
		t = ns_get16(rdata);
		rdata += NS_INT16SZ;
		len = SPRINTF((tmp, "%u ", t));
		VALIDATE(addstr(tmp, len, &buf, &buflen));

		/* Name1. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		VALIDATE(addstr(" ", 1, &buf, &buflen));

		/* Name2. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		break;
	    }

	case ns_t_x25:
		VALIDATE(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		break;

	case ns_t_txt:
		while (rdata < edata) {
			VALIDATE(len = charstr(rdata, edata, &buf, &buflen));
			if (len == 0)
				goto formerr;
			rdata += len;
			if (rdata < edata)
				VALIDATE(addstr(" ", 1, &buf, &buflen));
		}
		break;


	case ns_t_aaaa:
		if (rdlen != (size_t)NS_IN6ADDRSZ)
			goto formerr;
		(void) inet_ntop(AF_INET6, (u_char*)rdata, buf, buflen);
		addlen(strlen(buf), &buf, &buflen);
		break;

	case ns_t_loc: {
		char t[255];

		/* XXX protocol format checking? */
		(void) loc_ntoa(rdata, t);
		VALIDATE(addstr(t, strlen(t), &buf, &buflen));
		break;
	    }

	case ns_t_naptr: {
		u_int order, preference;
		char t[50];

		if (rdlen < 2U*NS_INT16SZ)
			goto formerr;

		/* Order, Precedence. */
		order = ns_get16(rdata);	rdata += NS_INT16SZ;
		preference = ns_get16(rdata);	rdata += NS_INT16SZ;
		len = SPRINTF((t, "%u %u ", order, preference));
		VALIDATE(addstr(t, len, &buf, &buflen));

		/* Flags. */
		VALIDATE(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		VALIDATE(addstr(" ", 1, &buf, &buflen));

		/* Service. */
		VALIDATE(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		VALIDATE(addstr(" ", 1, &buf, &buflen));

		/* Regexp. */
		VALIDATE(len = charstr(rdata, edata, &buf, &buflen));
		if (len < 0)
			return (-1);
		if (len == 0)
			goto formerr;
		rdata += len;
		VALIDATE(addstr(" ", 1, &buf, &buflen));

		/* Server. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		break;
	    }

	case ns_t_srv: {
		u_int priority, weight, port;
		char t[50];

		if (rdlen < 3U*NS_INT16SZ)
			goto formerr;

		/* Priority, Weight, Port. */
		priority = ns_get16(rdata);  rdata += NS_INT16SZ;
		weight   = ns_get16(rdata);  rdata += NS_INT16SZ;
		port     = ns_get16(rdata);  rdata += NS_INT16SZ;
		len = SPRINTF((t, "%u %u %u ", priority, weight, port));
		VALIDATE(addstr(t, len, &buf, &buflen));

		/* Server. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		break;
	    }

	case ns_t_minfo:
	case ns_t_rp:
		/* Name1. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		VALIDATE(addstr(" ", 1, &buf, &buflen));

		/* Name2. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		break;

	case ns_t_wks: {
		int n, lcnt;

		if (rdlen < 1U + NS_INT32SZ)
			goto formerr;

		/* Address. */
		(void) inet_ntop(AF_INET, (u_char*)rdata, buf, buflen);
		addlen(strlen(buf), &buf, &buflen);
		rdata += NS_INADDRSZ;

		/* Protocol. */
		len = SPRINTF((tmp, " %u ( ", *rdata));
		VALIDATE(addstr(tmp, len, &buf, &buflen));
		rdata += NS_INT8SZ;

		/* Bit map. */
		n = 0;
		lcnt = 0;
		while (rdata < edata) {
			u_int c = *rdata++;
			do {
				if (c & 0200) {
					if (lcnt == 0) {
						VALIDATE(addstr("\n\t\t\t\t", 5,
							 &buf, &buflen));
						lcnt = 10;
						spaced = 0;
					}
					len = SPRINTF((tmp, "%d ", n));
					VALIDATE(addstr(tmp, len, &buf, &buflen));
					lcnt--;
				}
				c <<= 1;
			} while (++n & 07);
		}
		VALIDATE(addstr(")", 1, &buf, &buflen));

		break;
	    }



	case ns_t_nxt: {
		ptrdiff_t n;
		int c;

		/* Next domain name. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		/* Type bit map. */
		n = edata - rdata;
		for (c = 0; c < n*8; c++)
			if (NS_NXT_BIT_ISSET(c, rdata)) {
				len = SPRINTF((tmp, " %s", p_type(c)));
				VALIDATE(addstr(tmp, len, &buf, &buflen));
			}
		break;
	    }


	case ns_t_tkey: {
		/* KJD - need to complete this */
		u_long t;
		int mode, err, keysize;

		/* Algorithm name. */
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		VALIDATE(addstr(" ", 1, &buf, &buflen));

		/* Inception. */
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		len = SPRINTF((tmp, "%s ", p_secstodate(t)));
		VALIDATE(addstr(tmp, len, &buf, &buflen));

		/* Experation. */
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		len = SPRINTF((tmp, "%s ", p_secstodate(t)));
		VALIDATE(addstr(tmp, len, &buf, &buflen));

		/* Mode , Error, Key Size. */
		/* Priority, Weight, Port. */
		mode = ns_get16(rdata);  rdata += NS_INT16SZ;
		err  = ns_get16(rdata);  rdata += NS_INT16SZ;
		keysize  = ns_get16(rdata);  rdata += NS_INT16SZ;
		len = SPRINTF((tmp, "%u %u %u ", mode, err, keysize));
		VALIDATE(addstr(tmp, len, &buf, &buflen));

		/* XXX need to dump key, print otherdata length & other data */
		break;
	    }

	case ns_t_tsig: {
		/* BEW - need to complete this */
		int n;

		VALIDATE(len = addname(msg, msglen, &rdata, origin, &buf, &buflen));
		VALIDATE(addstr(" ", 1, &buf, &buflen));
		rdata += 8; /* time */
		NS_GET16( n, rdata );
		rdata += n; /* sig */
		NS_GET16( n, rdata ); /* original id */
		NS_GET16( n, rdata );
		sprintf(buf, "%d", n);
		addlen(strlen(buf), &buf, &buflen);
		break;
	    }

	case ns_t_a6: {
		struct in6_addr a;
		int pbyte, pbit;

		/* prefix length */
		if (rdlen == 0U) goto formerr;
		len = SPRINTF((tmp, "%d ", *rdata));
		VALIDATE(addstr(tmp, len, &buf, &buflen));
		pbit = *rdata;
		if (pbit > 128) goto formerr;
		pbyte = (pbit & ~7) / 8;
		rdata++;

		/* address suffix: provided only when prefix len != 128 */
		if (pbit < 128) {
			if (rdata + pbyte >= edata) goto formerr;
			memset(&a, 0, sizeof(a));
			memcpy(&a.s6_addr[pbyte], rdata, sizeof(a) - pbyte);
			(void) inet_ntop(AF_INET6, &a, buf, buflen);
			addlen(strlen(buf), &buf, &buflen);
			rdata += sizeof(a) - pbyte;
		}

		/* prefix name: provided only when prefix len > 0 */
		if (pbit == 0)
			break;
		if (rdata >= edata) goto formerr;
		VALIDATE(addstr(" ", 1, &buf, &buflen));
		VALIDATE(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		
		break;
	    }

	case ns_t_opt: {
		len = SPRINTF((tmp, "%u bytes", class));
		VALIDATE(addstr(tmp, len, &buf, &buflen));
		break;
	    }

	default:
		comment = "unknown RR type";
		goto hexify;
	}
	return (int)(buf - obuf); // MM-5297: Ptr arithmetic unlikely to exceed 4GB, as it's in same buffer.
 formerr:
	comment = "RR format error";
hexify: {
	ptrdiff_t n;
	int m;
	char *p;

	len = SPRINTF((tmp, "\\# %u%s\t; %s", (unsigned)(edata - rdata),
		       rdlen != 0U ? " (" : "", comment));
	VALIDATE(addstr(tmp, len, &buf, &buflen));
	while (rdata < edata) {
		p = tmp;
		p += SPRINTF((p, "\n\t"));
		spaced = 0;
		n = min(16, edata - rdata);
		for (m = 0; m < n; m++)
			p += SPRINTF((p, "%02x ", rdata[m]));
		VALIDATE(addstr(tmp, p - tmp, &buf, &buflen));
		if (n < 16) {
			VALIDATE(addstr(")", 1, &buf, &buflen));
			VALIDATE(addtab(p - tmp + 1, 48, spaced, &buf, &buflen));
		}
		p = tmp;
		p += SPRINTF((p, "; "));
		for (m = 0; m < n; m++)
			*p++ = (isascii(rdata[m]) && isprint(rdata[m]))
				? rdata[m]
				: '.';
		VALIDATE(addstr(tmp, p - tmp, &buf, &buflen));
		rdata += n;
	}
	return (int)(buf - obuf); // MM-5297: Ptr arithmetic unlikely to exceed 4GB, as it's in same buffer.
    }
}

/* Private. */

/*
 * size_t
 * prune_origin(name, origin)
 *	Find out if the name is at or under the current origin.
 * return:
 *	Number of characters in name before start of origin,
 *	or length of name if origin does not match.
 * notes:
 *	This function should share code with samedomain().
 */
static size_t
prune_origin(const char *name, const char *origin) {
	const char *oname = name;

	while (*name != '\0') {
		if (origin != NULL && ns_samename(name, origin) == 1)
			return (name - oname - (name > oname));
		while (*name != '\0') {
			if (*name == '\\') {
				name++;
				/* XXX need to handle \nnn form. */
				if (*name == '\0')
					break;
			} else if (*name == '.') {
				name++;
				break;
			}
			name++;
		}
	}
	return (name - oname);
}

/*
 * int
 * charstr(rdata, edata, buf, buflen)
 *	Format a <character-string> into the presentation buffer.
 * return:
 *	Number of rdata octets consumed
 *	0 for protocol format error
 *	-1 for output buffer error
 * side effects:
 *	buffer is advanced on success.
 */
static int
charstr(const u_char *rdata, const u_char *edata, char **buf, size_t *buflen) {
	const u_char *odata = rdata;
	size_t save_buflen = *buflen;
	char *save_buf = *buf;

	if (addstr("\"", 1, buf, buflen) < 0)
		goto enospc;
	if (rdata < edata) {
		int n = *rdata;

		if (rdata + 1 + n <= edata) {
			rdata++;
			while (n-- > 0) {
				if (strchr("\n\"\\", *rdata) != NULL)
					if (addstr("\\", 1, buf, buflen) < 0)
						goto enospc;
				if (addstr((const char *)rdata, 1,
					   buf, buflen) < 0)
					goto enospc;
				rdata++;
			}
		}
	}
	if (addstr("\"", 1, buf, buflen) < 0)
		goto enospc;
	return (int)(rdata - odata); // MM-5297: Ptr arithmetic unlikely to exceed 4GB, as it's in same buffer.
 enospc:
	errno = ENOSPC;
	*buf = save_buf;
	*buflen = save_buflen;
	return (-1);
}

static ptrdiff_t
addname(const u_char *msg, size_t msglen,
	const u_char **pp, const char *origin,
	char **buf, size_t *buflen)
{
	size_t newlen, save_buflen = *buflen;
	char *save_buf = *buf;
	int n;

	n = dn_expand(msg, msg + msglen, *pp, *buf, *buflen);
	if (n < 0)
		goto enospc;	/* Guess. */
	newlen = prune_origin(*buf, origin);
	if (**buf == '\0') {
		goto root;
	} else if (newlen == 0U) {
		/* Use "@" instead of name. */
		if (newlen + 2 > *buflen)
			goto enospc;        /* No room for "@\0". */
		(*buf)[newlen++] = '@';
		(*buf)[newlen] = '\0';
	} else {
		if (((origin == NULL || origin[0] == '\0') ||
		    (origin[0] != '.' && origin[1] != '\0' &&
		    (*buf)[newlen] == '\0')) && (*buf)[newlen - 1] != '.') {
			/* No trailing dot. */
 root:
			if (newlen + 2 > *buflen)
				goto enospc;	/* No room for ".\0". */
			(*buf)[newlen++] = '.';
			(*buf)[newlen] = '\0';
		}
	}
	*pp += n;
	addlen(newlen, buf, buflen);
	**buf = '\0';
	return (newlen);
 enospc:
	errno = ENOSPC;
	*buf = save_buf;
	*buflen = save_buflen;
	return (-1);
}

static void
addlen(size_t len, char **buf, size_t *buflen) {
	INSIST(len <= *buflen);
	*buf += len;
	*buflen -= len;
}

static int
addstr(const char *src, size_t len, char **buf, size_t *buflen) {
	if (len >= *buflen) {
		errno = ENOSPC;
		return (-1);
	}
	memcpy(*buf, src, len);
	addlen(len, buf, buflen);
	**buf = '\0';
	return (0);
}

static int
addtab(size_t len, size_t target, int spaced, char **buf, size_t *buflen) {
	size_t save_buflen = *buflen;
	char *save_buf = *buf;
	long long t;

	if (spaced || len >= target - 1) {
		VALIDATE(addstr("  ", 2, buf, buflen));
		spaced = 1;
	} else {
		for (t = (target - len - 1) / 8; t >= 0; t--)
			if (addstr("\t", 1, buf, buflen) < 0) {
				*buflen = save_buflen;
				*buf = save_buf;
				return (-1);
			}
		spaced = 0;
	}
	return (spaced);
}
