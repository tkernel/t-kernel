/*
 *----------------------------------------------------------------------
 *    T-Kernel 2.0 Software Package
 *
 *    Copyright 2011 by Ken Sakamura.
 *    This software is distributed under the latest version of T-License 2.x.
 *----------------------------------------------------------------------
 *
 *    Released by T-Engine Forum(http://www.t-engine.org/) at 2011/05/17.
 *    Modified by TRON Forum(http://www.tron.org/) at 2015/06/01.
 *
 *    Modified for APP-M150(MIPS32) at 2015/10/19.
 *
 *----------------------------------------------------------------------
 */

/*
 *	@(#)printf.c	( T-Monitor for APP-M150 )
 *
 *	printf() / sprintf() routine
 *
 *	- Unsupported specifiers: floating point, long long and others.
 *		Coversion:	 a, A, e, E, f, F, g, G, n
 *		Size qualifier:  hh, ll, j, z, t, L
 *	- No limitation of output string length.
 *	- Minimize stack usage.
 *		Depending on available stack size, define OUTBUF_SZ by
 *		appropriate value.
 */
#include <stdarg.h>
#include <basic.h>

/* Output Buffer size in stack */
#define	OUTBUF_SZ	0

extern	int	putChar(int c);
extern	int	printk(unsigned char *s);

/* Output function */
typedef	struct {
	short	len;		/* Total output length */
	short	cnt;		/* Buffer counts */
	unsigned char	*bufp;		/* Buffer pointer for sprintf */
} OutPar;
typedef	void	(*OutFn)(unsigned char *str, int len, OutPar *par);

/*
 *	Output integer value
 */
static	unsigned char	*outint(unsigned char *ep, unsigned long val, unsigned char base)
{
static const unsigned char  digits[] = "0123456789abcdef0123456789ABCDEF";
	unsigned char	caps;

	caps = (base & 0x40) >> 2;		/* 'a' or 'A' */
	for (base &= 0x3F; val >= base; val /= base) {
		*--ep = digits[(val % base) + caps];
	}
	*--ep = digits[val + caps];
	return ep;				/* buffer top pointer */
}

/*
 *	Output with format (limitted version)
 */
static	void	_vsprintf(OutFn ostr, OutPar *par,
					const unsigned char *fmt, va_list ap)
{
#define	MAX_DIGITS	14
	unsigned long	v;
	short	wid, prec, n;
	unsigned char	*fms, *cbs, *cbe, cbuf[MAX_DIGITS];
	unsigned char	c, base, flg, sign, qual;

/* flg */
#define	F_LEFT		0x01
#define	F_PLUS		0x02
#define	F_SPACE		0x04
#define	F_PREFIX	0x08
#define	F_ZERO		0x10

	for (fms = NULL; (c = *fmt++) != '\0'; ) {

		if (c != '%') {	/* Fixed string */
			if (fms == NULL) fms = (unsigned char*)fmt - 1;
			continue;
		}

		/* Output fix string */
		if (fms != NULL) {
			(*ostr)(fms, fmt - fms - 1, par);
			fms = NULL;
		}

		/* Get flags */
		for (flg = 0; ; ) {
			switch (c = *fmt++) {
			case '-': flg |= F_LEFT;	continue;
			case '+': flg |= F_PLUS;	continue;
			case ' ': flg |= F_SPACE;	continue;
			case '#': flg |= F_PREFIX;	continue;
			case '0': flg |= F_ZERO;	continue;
			}
			break;
		}

		/* Get field width */
		if (c == '*') {
			wid = va_arg(ap, int);
			if (wid < 0) {
				wid = -wid;
				flg |= F_LEFT;
			}
			c = *fmt++;
		} else {
			for (wid = 0; c >= '0' && c <= '9'; c = *fmt++)
				wid = wid * 10 + c - '0';
		}

		/* Get precision */
		prec = -1;
		if (c == '.') {
			c = *fmt++;
			if (c == '*') {
				prec = va_arg(ap, int);
				if (prec < 0) prec = 0;
				c = *fmt++;
			} else {
				for (prec = 0;c >= '0' && c <= '9';c = *fmt++)
					prec = prec * 10 + c - '0';
			}
			flg &= ~F_ZERO;		/* No ZERO padding */
		}

		/* Get qualifier */
		qual = 0;
		if (c == 'h' || c == 'l') {
			qual = c;
			c = *fmt++;
		}

		/* Format items */
		base = 10;
		sign = 0;
		cbe = &cbuf[MAX_DIGITS];	/* buffer end pointer */

		switch (c) {
		case 'i':
		case 'd':
		case 'u':
		case 'X':
		case 'x':
		case 'o':
			if (qual == 'l') {
				v = va_arg(ap, unsigned long);
			} else {
				v = va_arg(ap, unsigned int);
				if (qual == 'h') {
					v = (c == 'i' || c == 'd') ?
						(short)v :(unsigned short)v;
				}
			}
			switch (c) {
			case 'i':
			case 'd':
				if ((long)v < 0) {
					v = - (long)v;
					sign = '-';
				} else if ((flg & F_PLUS) != 0) {
					sign = '+';
				} else if ((flg & F_SPACE) != 0) {
					sign = ' ';
				} else {
					break;
				}
				wid--;		/* for sign */
			case 'u':
				break;
			case 'X':
				base += 0x40;	/* base = 16 + 0x40 */
			case 'x':
				base += 8;	/* base = 16 */
			case 'o':
				base -= 2;	/* base = 8 */
				if ((flg & F_PREFIX) != 0 && v != 0) {
					wid -= (base == 8) ? 1 : 2;
					base |= 0x80;
				}
				break;
			}
			/* Note: None outputs when v == 0 && prec == 0 */
			cbs = (v == 0 && prec == 0) ?
						cbe : outint(cbe, v, base);
			break;
		case 'p':
			v = (unsigned long)va_arg(ap, void *);
			if (v != 0) {
				base = 16 | 0x80;
				wid -= 2;
			}
			cbs = outint(cbe, v, base);
			break;
		case 's':
			cbe = cbs = va_arg(ap, unsigned char *);
			if (prec < 0) {
				while (*cbe != '\0') cbe++;
			} else {
				while (--prec >= 0 && *cbe != '\0') cbe++;
			}
			break;
		case 'c':
			cbs = cbe;
			*--cbs = (unsigned char)va_arg(ap, int);
			prec = 0;
			break;
		case '\0':
			fmt--;
			continue;
		default:
			/* Output as fixed string */
			fms = (unsigned char*)fmt - 1;
			continue;
		}

		n = cbe - cbs;				/* item length */
		if ((prec -= n) > 0) n += prec;
		wid -= n;				/* pad length */

		/* Output preceding spaces */
		if ((flg & (F_LEFT | F_ZERO)) == 0 ) {
			while (--wid >= 0) (*ostr)((unsigned char*)" ", 1, par);
		}

		/* Output sign */
		if (sign != 0) {
			(*ostr)(&sign, 1, par);
		}

		/* Output prefix "0x", "0X" or "0" */
		if ((base & 0x80) != 0) {
			(*ostr)((unsigned char*)"0", 1, par);
			if ((base & 0x10) != 0) {
				(*ostr)((base & 0x40) ?
					(unsigned char*)"X" : (unsigned char*)"x", 1, par);
			}
		}

		/* Output preceding zeros for precision or padding */
		if ((n = prec) <= 0) {
			if ((flg & (F_LEFT | F_ZERO)) == F_ZERO ) {
				n = wid;
				wid = 0;
			}
		}
		while (--n >= 0) (*ostr)((unsigned char*)"0", 1, par);

		/* Output item string */
		(*ostr)(cbs, cbe - cbs, par);

		/* Output tailing spaces */
		while (--wid >= 0) (*ostr)((unsigned char*)" ", 1, par);
	}

	/* Output last fix string */
	if (fms != NULL) {
		(*ostr)(fms, fmt - fms - 1, par);
	}
#if	OUTBUF_SZ > 0
	/* Flush output */
	(*ostr)(NULL, 0, par);
#endif
}

/*
 *	Output to console
 */
static	void	out_cons(unsigned char *str, int len,  OutPar *par)
{
#if	OUTBUF_SZ == 0
	/* Direct output to console */
	par->len += len;
	while (--len >= 0) putChar(*str++);
#else
	/* Buffered output to console */
	if (str == NULL) {	/* Flush */
		if (par->cnt > 0) {
			par->bufp[par->cnt] = '\0';
			printk(par->bufp);
			par->cnt = 0;
		}
	} else {
		par->len += len;
		while (--len >= 0) {
			if (par->cnt >= OUTBUF_SZ - 1) {
				par->bufp[par->cnt] = '\0';
				printk(par->bufp);
				par->cnt = 0;
			}
			par->bufp[par->cnt++] = *str++;
		}
	}
#endif
}

int	vprintf(const char *format, va_list ap)
{
#if	OUTBUF_SZ == 0
	short	len = 0;
	_vsprintf(out_cons, (OutPar*)&len, (const unsigned char *)format, ap);
	return len;
#else
	char	obuf[OUTBUF_SZ];
	OutPar	par;

	par.len = par.cnt = 0;
	par.bufp = obuf;
	_vsprintf(out_cons, (OutPar*)&par, (const unsigned char *)format, ap);
	va_end(ap);
	return par.len;
#endif
}

int	printf(const char *format, ...)
{
	va_list	ap;
	int	len;

	va_start(ap, format);
	len = vprintf(format, ap);
	va_end(ap);

	return len;
}

/*
 *	Output to buffer
 */
static	void	out_buf(unsigned char *str, int len, OutPar *par)
{
	par->len += len;
	while (--len >= 0) *(par->bufp)++ = *str++;
}

int	vsprintf(char *str, const char *format, va_list ap)
{
	OutPar	par;

	par.len = 0;
	par.bufp = (unsigned char*)str;
	_vsprintf(out_buf, &par, (const unsigned char *)format, ap);
	str[par.len] = '\0';
	return par.len;
}

int	sprintf(char *str, const char *format, ...)
{
	va_list	ap;
	int	len;

	va_start(ap, format);
	len = vsprintf(str, format, ap);
	va_end(ap);

	return len;
}

