/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <assert.h>
#include <inttypes.h>
#include	<sfio/sfhdr.h>
#include	<stddef.h>
#include	<stdio.h>
#include	<string.h>

/*	The engine for formatting data
**
**	Written by Kiem-Phong Vo.
*/

#define HIGHBITI	(~((~0u) >> 1))
#define HIGHBITL	(~((~((Sfulong_t)0)) >> 1))

#define SFFMT_PREFIX	(SFFMT_MINUS|SFFMT_SIGN|SFFMT_BLANK)

#define FPRECIS		6	/* default precision for floats         */


/**
 * @param f file to print to
 * @param format Structure describing how to print
 */
int sfprint(FILE *f, Sffmt_t *format) {
    int v = 0, n_s, base, fmt, flags;
    Sflong_t lv;
    char *sp, *ssp, *endsp, *ep, *endep;
    int dot, width, precis, n, n_output = 0;
    int sign, decpt;
    ssize_t size;
    double dval;
    char *tls[2], **ls;		/* for %..[separ]s              */
    const char *t_str;		/* stuff between ()             */
    ssize_t n_str;		/* its length                   */

    Argv_t argv;		/* for extf to return value     */
    Sffmt_t *ft;		/* format environment           */
    Fmt_t *fm, *fmstk;		/* stack contexts               */

    Fmtpos_t *fp;		/* arg position list            */
    int argp, argn;		/* arg position and number      */

#define SLACK		1024
    char buf[SF_MAXDIGITS + SLACK];
    char decimal = 0, thousand = 0;

#define SFputc(f,c) do { \
    if (putc((c), (f)) == EOF) { \
      goto done; \
    } \
} while (0)
#define SFnputc(f,c,n)  do { \
    for (int i_ = 0; i_ < (n); ++i_) { \
      if (putc((c), (f)) == EOF) { \
        goto done; \
      } \
    } \
    (n) = 0; \
} while (0)
#define SFwrite(f,s,n) do { \
  if (fwrite((s), (n), 1, (f)) < 1) { \
    goto done; \
  } \
} while (0)

    tls[1] = NULL;

    fmstk = NULL;
    ft = NULL;

    fp = NULL;
    argn = -1;

    // stack a new environment
    argv.ft = format;
    assert(argv.ft != NULL);
    assert(argv.ft->form != NULL);
    if (!(fm = malloc(sizeof(Fmt_t))))
	goto done;

    fm->form = "";
    memset(&fm->args, 0, sizeof(fm->args));

    fm->argn = argn;
    fm->fp = NULL;

    const char *form = argv.ft->form;
    va_list args;
    va_copy(args, argv.ft->args);
    fp = NULL;

    fm->ft = ft;
    fm->next = fmstk;
    fmstk = fm;
    ft = argv.ft;

  loop_fmt:
    while ((n = *form)) {
	if (n != '%') {		/* collect the non-pattern chars */
	    sp = (char *) form++;
	    while (*form && *form != '%')
		form += 1;
	    n = form - sp;
	    SFwrite(f, sp, n);
	    continue;
	} else
	    form += 1;

	flags = 0;
	size = width = precis = base = n_s = argp = -1;
	ssp = _Sfdigits;
	endep = ep = NULL;
	endsp = sp = buf + (sizeof(buf) - 1);
	t_str = NULL;
	n_str = dot = 0;

      loop_flags:		/* LOOP FOR \0, %, FLAGS, WIDTH, PRECISION, BASE, TYPE */
	switch ((fmt = *form++)) {
	case '\0':
	    SFputc(f, '%');
	    goto pop_fmt;
	case '%':
	    SFputc(f, '%');
	    continue;

	case LEFTP:		/* get the type enclosed in balanced parens */
	    t_str = form;
	    for (v = 1;;) {
		switch (*form++) {
		case 0:	/* not balancable, retract */
		    form = t_str;
		    t_str = NULL;
		    n_str = 0;
		    goto loop_flags;
		case LEFTP:	/* increasing nested level */
		    v += 1;
		    continue;
		case RIGHTP:	/* decreasing nested level */
		    if ((v -= 1) != 0)
			continue;
		    if (*t_str != '*')
			n_str = (form - 1) - t_str;
		    else {
			t_str = _Sffmtintf(t_str + 1, &n);
			n = FP_SET(-1, argn);

			if (fp) {
			    t_str = fp[n].argv.s;
			    n_str = fp[n].ft.size;
			} else if (ft && ft->extf) {
			    FMTSET(ft, form, args,
				   LEFTP, 0, 0, 0, 0, 0, NULL, 0);
			    n = ft->extf(&argv, ft);
			    if (n < 0)
				goto pop_fmt;
			    if (!(ft->flags & SFFMT_VALUE))
				goto t_arg;
			    if ((t_str = argv.s) &&
				(n_str = (int) ft->size) < 0)
				n_str = (ssize_t)strlen(t_str);
			} else {
			  t_arg:
			    if ((t_str = va_arg(args, char *)))
				 n_str = (ssize_t)strlen(t_str);
			}
		    }
		    goto loop_flags;
		default:
		    // skip over
		    break;
		}
	    }

	case '-':
	    flags = (flags & ~SFFMT_ZERO) | SFFMT_LEFT;
	    goto loop_flags;
	case '0':
	    if (!(flags & SFFMT_LEFT))
		flags |= SFFMT_ZERO;
	    goto loop_flags;
	case ' ':
	    if (!(flags & SFFMT_SIGN))
		flags |= SFFMT_BLANK;
	    goto loop_flags;
	case '+':
	    flags = (flags & ~SFFMT_BLANK) | SFFMT_SIGN;
	    goto loop_flags;
	case '#':
	    flags |= SFFMT_ALTER;
	    goto loop_flags;
	case QUOTE:
	    SFSETLOCALE(decimal, thousand);
	    if (thousand)
		flags |= SFFMT_THOUSAND;
	    goto loop_flags;

	case '.':
	    dot += 1;
	    if (dot == 1) {	/* so base can be defined without setting precis */
		if (*form != '.')
		    precis = 0;
	    } else if (dot == 2) {
		base = 0;	/* for %s,%c */
		if (*form == 'c' || *form == 's')
		    goto loop_flags;
		if (*form && !isalnum((int)*form) &&
		    (form[1] == 'c' || form[1] == 's')) {
		    if (*form == '*')
			goto do_star;
		    else {
			base = *form++;
			goto loop_flags;
		    }
		}
	    }

	    if (isdigit((int)*form)) {
		fmt = *form++;
		goto dot_size;
	    } else if (*form != '*')
		goto loop_flags;
	  do_star:
	    form += 1;		/* fall through */
	case '*':
	    form = _Sffmtintf(form, &n);
	    n = FP_SET(-1, argn);

	    if (fp)
		v = fp[n].argv.i;
	    else if (ft && ft->extf) {
		FMTSET(ft, form, args, '.', dot, 0, 0, 0, 0, NULL,
		       0);
		if (ft->extf(&argv, ft) < 0)
		    goto pop_fmt;
		if (ft->flags & SFFMT_VALUE)
		    v = argv.i;
		else
		    v = (dot <= 2) ? va_arg(args, int) : 0;
	    } else
		v = dot <= 2 ? va_arg(args, int) : 0;
	    goto dot_set;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  dot_size:
	    for (v = fmt - '0'; isdigit((int)*form); ++form)
		v = v * 10 + (*form - '0');
	  dot_set:
	    if (dot == 0) {
		if ((width = v) < 0) {
		    width = -width;
		    flags = (flags & ~SFFMT_ZERO) | SFFMT_LEFT;
		}
	    } else if (dot == 1)
		precis = v;
	    else if (dot == 2)
		base = v;
	    goto loop_flags;

	case 'I':		/* object length */
	    size = 0;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_IFLAG;
	    if (isdigit((int)*form)) {
		for (n = *form; isdigit(n); n = *++form)
		    size = size * 10 + (n - '0');
	    } else if (*form == '*') {
		form = _Sffmtintf(form + 1, &n);
		n = FP_SET(-1, argn);

		if (fp)		/* use position list */
		    size = fp[n].argv.i;
		else if (ft && ft->extf) {
		    FMTSET(ft, form, args, 'I', sizeof(int), 0, 0, 0, 0,
			   NULL, 0);
		    if (ft->extf(&argv, ft) < 0)
			goto pop_fmt;
		    if (ft->flags & SFFMT_VALUE)
			size = argv.i;
		    else
			size = va_arg(args, int);
		} else
		    size = va_arg(args, int);
	    }
	    goto loop_flags;

	case 'l':
	    size = -1;
	    flags &= ~SFFMT_TYPES;
	    if (*form == 'l') {
		form += 1;
		flags |= SFFMT_LLONG;
	    } else
		flags |= SFFMT_LONG;
	    goto loop_flags;
	case 'h':
	    size = -1;
	    flags &= ~SFFMT_TYPES;
	    if (*form == 'h') {
		form += 1;
		flags |= SFFMT_SSHORT;
	    } else
		flags |= SFFMT_SHORT;
	    goto loop_flags;
	case 'L':
	    size = -1;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_LDOUBLE;
	    goto loop_flags;

	case 'j':
	    size = -1;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_JFLAG;
	    goto loop_flags;
	case 'z':
	    size = -1;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_ZFLAG;
	    goto loop_flags;
	case 't':
	    size = -1;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_TFLAG;
	    goto loop_flags;
	}

	/* set the correct size */
	if (flags & (SFFMT_TYPES & ~SFFMT_IFLAG)) {
	    if (_Sftype[fmt] & (SFFMT_INT | SFFMT_UINT)) {
		size = (flags & SFFMT_LLONG) ? sizeof(Sflong_t) :
		    (flags & SFFMT_LONG) ? sizeof(long) :
		    (flags & SFFMT_SHORT) ? sizeof(short) :
		    (flags & SFFMT_SSHORT) ? sizeof(char) :
		    (flags & SFFMT_JFLAG) ? sizeof(Sflong_t) :
		    (flags & SFFMT_TFLAG) ? sizeof(ptrdiff_t) :
		    (flags & SFFMT_ZFLAG) ? sizeof(size_t) : -1;
	    } else if (_Sftype[fmt] & SFFMT_FLOAT) {
		size = (flags & SFFMT_LDOUBLE) ? sizeof(Sfdouble_t) :
		    (flags & (SFFMT_LONG | SFFMT_LLONG)) ?
		    sizeof(double) : -1;
	    }
	}

	argp = FP_SET(argp, argn);
	if (fp) {
	    if (ft && ft->extf && fp[argp].ft.fmt != fp[argp].fmt)
		fmt = fp[argp].ft.fmt;
	    argv = fp[argp].argv;
	    size = fp[argp].ft.size;
	} else if (ft && ft->extf) {	/* extended processing */
	    FMTSET(ft, form, args, fmt, size, flags, width, precis, base,
		   t_str, n_str);
	    v = ft->extf(&argv, ft);

	    if (v < 0)
		goto pop_fmt;
	    else if (v == 0) {	/* extf did not output */
		FMTGET(ft, form, args, fmt, size, flags, width, precis,
		       base);
		if (!(ft->flags & SFFMT_VALUE))
		    goto get_val;
	    } else if (v > 0) {	/* extf output v bytes */
		n_output += v;
		continue;
	    }
	} else {
	  get_val:
	    switch (_Sftype[fmt]) {
	    case SFFMT_INT:
	    case SFFMT_UINT:
		if (FMTCMP(size, long, Sflong_t))
		     argv.l = va_arg(args, long);
		else
		    argv.i = va_arg(args, int);
		break;
	    case SFFMT_FLOAT:
		    argv.d = va_arg(args, double);
		break;
	    case SFFMT_POINTER:
		argv.vp = va_arg(args, void *);
		break;
	    case SFFMT_BYTE:
		if (base >= 0)
		    argv.s = va_arg(args, char *);
		else
		    argv.c = (char) va_arg(args, int);
		break;
	    default:		/* unknown pattern */
		break;
	    }
	}

	switch (fmt) {		/* PRINTF DIRECTIVES */
	default:		/* unknown directive */
	    form -= 1;
	    argn -= 1;
	    continue;

	case 's':
	    if (base >= 0) {	/* list of strings */
		if (!(ls = argv.sp) || !ls[0])
		    continue;
	    } else {
		if (!(sp = argv.s))
		    sp = "(null)";
		ls = tls;
		tls[0] = sp;
	    }
	    for (sp = *ls;;) {
		if ((v = size) >= 0) {
		    if (precis >= 0 && v > precis)
			v = precis;
		} else if (precis < 0)
		    v = strlen(sp);
		else {		/* precis >= 0 means min(strlen,precis) */
		    for (v = 0; v < precis; ++v)
			if (sp[v] == 0)
			    break;
		}
		if ((n = width - v) > 0) {
		    if (flags & SFFMT_ZERO) {
			SFnputc(f, '0', n);
		    } else if (!(flags & SFFMT_LEFT)) {
			SFnputc(f, ' ', n);
		    }
		}
		SFwrite(f, sp, v);
		if (n > 0) {
		    SFnputc(f, ' ', n);
		}
		if (!(sp = *++ls))
		    break;
		else if (base > 0) {
		    SFputc(f, base);
		}
	    }
	    continue;

	case 'c':		/* an array of characters */
	    if (base >= 0) {
		if (!(sp = argv.s) || !sp[0])
		    continue;
	    } else {
		fmt = (int) argv.c;
		sp = buf;
		buf[0] = fmt;
		buf[1] = 0;
	    }
	    if (precis <= 0)
		precis = 1;
	    for (fmt = *sp;;) {
		if ((n = width - precis) > 0 && !(flags & SFFMT_LEFT)) {
		    SFnputc(f, ' ', n);
		}
		v = precis;
		SFnputc(f, fmt, v);
		if (n > 0) {
		    SFnputc(f, ' ', n);
		}
		if (!(fmt = *++sp))
		    break;
		else if (base > 0) {
		    SFputc(f, base);
		}
	    }
	    continue;

	case 'p':		/* pointer value */
	    fmt = 'x';
	    base = 16;
	    n_s = 15;
	    n = 4;
	    flags =
		(flags & ~(SFFMT_SIGN | SFFMT_BLANK | SFFMT_ZERO)) |
		SFFMT_ALTER;
	    if (sizeof(void*) > sizeof(int)) {
		lv = (Sflong_t)(intptr_t)argv.vp;
		goto long_cvt;
	    } else {
		v = (int)(intptr_t)argv.vp;
		goto int_cvt;
	    }
	case 'o':
	    base = 8;
	    n_s = 7;
	    n = 3;
	    flags &= ~(SFFMT_SIGN | SFFMT_BLANK);
	    goto int_arg;
	case 'X':
	    ssp = "0123456789ABCDEF";
	    // fall through
	case 'x':
	    base = 16;
	    n_s = 15;
	    n = 4;
	    flags &= ~(SFFMT_SIGN | SFFMT_BLANK);
	    goto int_arg;
	case 'i':
	    fmt = 'd';
	    goto d_format;
	case 'u':
	    flags &= ~(SFFMT_SIGN | SFFMT_BLANK);
	    // fall through
	case 'd':
	  d_format:
	    if (base < 2 || base > SF_RADIX)
		base = 10;
	    if ((base & (n_s = base - 1)) == 0) {
		if (base < 8)
		    n = base < 4 ? 1 : 2;
		else if (base < 32)
		    n = base < 16 ? 3 : 4;
		else
		    n = base < 64 ? 5 : 6;
	    } else
		n_s = base == 10 ? -1 : 0;

	  int_arg:
	    if ((sizeof(long) > sizeof(int) || sizeof(void*) > sizeof(int))
	        && FMTCMP(size, Sflong_t, Sflong_t)) {
		lv = argv.ll;
		goto long_cvt;
	    } else if ((sizeof(long) > sizeof(int) || sizeof(void*) > sizeof(int))
	               && FMTCMP(size, long, Sflong_t)) {
		if (fmt == 'd')
		    lv = (Sflong_t) argv.l;
		else
		    lv = (Sflong_t) ((ulong) argv.l);
	      long_cvt:
		if (lv == 0 && precis == 0)
		    break;
		if (lv < 0 && fmt == 'd') {
		    flags |= SFFMT_MINUS;
		    if (lv == HIGHBITL) {	/* avoid overflow */
			lv = (Sflong_t) (HIGHBITL / base);
			*--sp = _Sfdigits[HIGHBITL -
					  ((Sfulong_t) lv) * base];
		    } else
			lv = -lv;
		}
		if (n_s < 0) {	/* base 10 */
		    Sflong_t nv;
		    sfucvt(lv, sp, nv, ssp, Sflong_t, Sfulong_t);
		} else if (n_s > 0) {	/* base power-of-2 */
		    do {
			*--sp = ssp[lv & n_s];
		    } while ((lv = ((Sfulong_t) lv) >> n));
		} else {	/* general base */
		    do {
			*--sp = ssp[((Sfulong_t) lv) % base];
		    } while ((lv = ((Sfulong_t) lv) / base));
		}
	    } else
	    if (sizeof(short) < sizeof(int)
		    && FMTCMP(size, short, Sflong_t)) {
		if (ft && ft->extf && (ft->flags & SFFMT_VALUE)) {
		    if (fmt == 'd')
			v = (int) ((short) argv.h);
		    else
			v = (int) ((ushort) argv.h);
		} else {
		    if (fmt == 'd')
			v = (int) ((short) argv.i);
		    else
			v = (int) ((ushort) argv.i);
		}
		goto int_cvt;
	    } else if (size == sizeof(char)) {
		if (ft && ft->extf && (ft->flags & SFFMT_VALUE)) {
		    if (fmt == 'd')
			v = (int) ((char) argv.c);
		    else
			v = (int) ((uchar) argv.c);
		} else {
		    if (fmt == 'd')
			v = (int) ((char) argv.i);
		    else
			v = (int) ((uchar) argv.i);
		}
		goto int_cvt;
	    } else {
		v = argv.i;
	      int_cvt:
		if (v == 0 && precis == 0)
		    break;
		if (v < 0 && fmt == 'd') {
		    flags |= SFFMT_MINUS;
		    if (v == HIGHBITI) {	/* avoid overflow */
			v = (int) (HIGHBITI / base);
			*--sp = _Sfdigits[HIGHBITI - (unsigned)v * base];
		    } else
			v = -v;
		}
		if (n_s < 0) {	/* base 10 */
		    sfucvt(v, sp, n, ssp, int, unsigned);
		} else if (n_s > 0) {	/* base power-of-2 */
		    do {
			*--sp = ssp[v & n_s];
		    } while ((v = (unsigned)v >> n));
		} else {	/* n_s == 0, general base */
		    do {
			*--sp = ssp[(unsigned)v % base];
		    } while ((v = (unsigned)v / base));
		}
	    }

	    if (n_s < 0 && (flags & SFFMT_THOUSAND)
		&& (n = endsp - sp) > 3) {
		if ((n %= 3) == 0)
		    n = 3;
		for (ep = buf + SLACK, endep = ep + n;;) {
		    while (ep < endep)
			*ep++ = *sp++;
		    if (sp == endsp)
			break;
		    if (sp <= endsp - 3)
			*ep++ = thousand;
		    endep = ep + 3;
		}
		sp = buf + SLACK;
		endsp = ep;
	    }

	    /* zero padding for precision if have room in buffer */
	    if (precis > 0 && (precis -= (endsp - sp)) < (sp - buf) - 64)
		while (precis-- > 0)
		    *--sp = '0';

	    if (flags & SFFMT_ALTER) {	/* prefix */
		if (fmt == 'o') {
		    if (*sp != '0')
			*--sp = '0';
		} else {
		    if (width > 0 && (flags & SFFMT_ZERO)) {	/* do 0 padding first */
			if (fmt == 'x' || fmt == 'X')
			    n = 0;
			else if (dot < 2)
			    n = width;
			else
			    n = base < 10 ? 2 : 3;
			n += (flags & (SFFMT_MINUS | SFFMT_SIGN)) ? 1 : 0;
			n = width - (n + (endsp - sp));
			while (n-- > 0)
			    *--sp = '0';
		    }
		    if (fmt == 'x' || fmt == 'X') {
			*--sp = (char) fmt;
			*--sp = '0';
		    } else if (dot >= 2) {	/* base#value notation */
			*--sp = '#';
			if (base < 10)
			    *--sp = (char) ('0' + base);
			else {
			    *--sp = _Sfdec[(base <<= 1) + 1];
			    *--sp = _Sfdec[base];
			}
		    }
		}
	    }

	    break;

	case 'g':
	case 'G':		/* these ultimately become %e or %f */
	case 'e':
	case 'E':
	case 'f':
	    if (!(ft && ft->extf && (ft->flags & SFFMT_VALUE)) ||
		    FMTCMP(size, double, Sfdouble_t))
		 dval = argv.d;
	    else
		dval = (double) argv.f;

	    if (fmt == 'e' || fmt == 'E') {
		n = (precis = precis < 0 ? FPRECIS : precis) + 1;
		{
		    ep = _sfcvt(&dval, min(n, SF_FDIGITS),
				&decpt, &sign, SFFMT_EFORMAT);
		}
		goto e_format;
	    } else if (fmt == 'f' || fmt == 'F') {
		precis = precis < 0 ? FPRECIS : precis;
		{
		    ep = _sfcvt(&dval, min(precis, SF_FDIGITS),
				&decpt, &sign, 0);
		}
		goto f_format;
	    }

	    /* 'g' or 'G' format */
	    precis = precis < 0 ? FPRECIS : precis == 0 ? 1 : precis;
	    {
		ep = _sfcvt(&dval, min(precis, SF_FDIGITS),
			    &decpt, &sign, SFFMT_EFORMAT);
		if (dval == 0.)
		    decpt = 1;
		else if (*ep == 'I')
		    goto infinite;
	    }

	    if (!(flags & SFFMT_ALTER)) {	/* zap trailing 0s */
		if ((n = sfslen()) > precis)
		    n = precis;
		while ((n -= 1) >= 1 && ep[n] == '0');
		n += 1;
	    } else
		n = precis;

	    if (decpt < -3 || decpt > precis) {
		precis = n - 1;
		goto e_format;
	    } else {
		precis = n - decpt;
		goto f_format;
	    }

	  e_format:		/* build the x.yyyy string */
	    if (isalpha((int)*ep))
		goto infinite;
	    sp = endsp = buf + 1;	/* reserve space for sign */
	    *endsp++ = *ep ? *ep++ : '0';

	    SFSETLOCALE(decimal, thousand);
	    if (precis > 0 || (flags & SFFMT_ALTER))
		*endsp++ = decimal;
	    ssp = endsp;
	    endep = ep + precis;
	    while ((*endsp++ = *ep++) && ep <= endep);
	    precis -= (endsp -= 1) - ssp;

	    /* build the exponent */
	    ep = endep = buf + (sizeof(buf) - 1);
	    if (dval != 0.) {
		if ((n = decpt - 1) < 0)
		    n = -n;
		while (n > 9) {
		    v = n;
		    n /= 10;
		    *--ep = (char) ('0' + (v - n * 10));
		}
	    } else
		n = 0;
	    *--ep = (char) ('0' + n);
	    if (endep - ep <= 1)	/* at least 2 digits */
		*--ep = '0';

	    /* the e/Exponent separator and sign */
	    *--ep = (decpt > 0 || dval == 0.) ? '+' : '-';
	    *--ep = isupper(fmt) ? 'E' : 'e';

	    goto end_efg;

	  f_format:		/* data before the decimal point */
	    if (isalpha((int)*ep)) {
	      infinite:
		endsp = (sp = ep) + sfslen();
		ep = endep;
		precis = 0;
		goto end_efg;
	    }

	    SFSETLOCALE(decimal, thousand);
	    endsp = sp = buf + 1;	/* save a space for sign */
	    endep = ep + decpt;
	    if (decpt > 3 && (flags & SFFMT_THOUSAND)) {
		if ((n = decpt % 3) == 0)
		    n = 3;
		while (ep < endep && (*endsp++ = *ep++)) {
		    if (--n == 0 && (ep <= endep - 3)) {
			*endsp++ = thousand;
			n = 3;
		    }
		}
	    } else {
		while (ep < endep && (*endsp++ = *ep++));
	    }
	    if (endsp == sp)
		*endsp++ = '0';

	    if (precis > 0 || (flags & SFFMT_ALTER))
		*endsp++ = decimal;

	    if ((n = -decpt) > 0) {	/* output zeros for negative exponent */
		ssp = endsp + min(n, precis);
		precis -= n;
		while (endsp < ssp)
		    *endsp++ = '0';
	    }

	    ssp = endsp;
	    endep = ep + precis;
	    while ((*endsp++ = *ep++) && ep <= endep);
	    precis -= (endsp -= 1) - ssp;
	    ep = endep;
	  end_efg:
	    flags |= SFFMT_FLOAT;
	    if (sign)
		flags |= SFFMT_MINUS;
	    break;
	}

	if (flags == 0 && width <= 0)
	    goto do_output;

	if (flags & SFFMT_PREFIX)
	    fmt =
		(flags & SFFMT_MINUS) ? '-' : (flags & SFFMT_SIGN) ? '+' :
		' ';

	n = (endsp - sp) + (endep - ep) + (precis <= 0 ? 0 : precis) +
	    ((flags & SFFMT_PREFIX) ? 1 : 0);
	if ((v = width - n) <= 0)
	    v = 0;
	else if (!(flags & SFFMT_ZERO)) {	/* right padding */
	    if (flags & SFFMT_LEFT)
		v = -v;
	    else if (flags & SFFMT_PREFIX) {	/* blank padding, output prefix now */
		*--sp = fmt;
		flags &= ~SFFMT_PREFIX;
	    }
	}

	if (flags & SFFMT_PREFIX) {	/* put out the prefix */
	    SFputc(f, fmt);
	    if (fmt != ' ')
		flags |= SFFMT_ZERO;
	}

	if ((n = v) > 0) {	/* left padding */
	    v = (flags & SFFMT_ZERO) ? '0' : ' ';
	    SFnputc(f, v, n);
	}

	if ((n = precis) > 0 && !(flags & SFFMT_FLOAT)) {	/* padding for integer precision */
	    SFnputc(f, '0', n);
	    precis = 0;
	}

      do_output:
	if ((n = endsp - sp) > 0)
	    SFwrite(f, sp, n);

	if (flags & (SFFMT_FLOAT | SFFMT_LEFT)) {	/* SFFMT_FLOAT: right padding for float precision */
	    if ((n = precis) > 0)
		SFnputc(f, '0', n);

	    /* SFFMT_FLOAT: the exponent of %eE */
	    if ((n = endep - (sp = ep)) > 0)
		SFwrite(f, sp, n);

	    /* SFFMT_LEFT: right padding */
	    if ((n = -v) > 0)
		SFnputc(f, ' ', n);
	}
    }

  pop_fmt:
    free(fp);
    fp = NULL;
    while ((fm = fmstk)) {	/* pop the format stack and continue */
	fmstk = fm->next;
	if ((form = fm->form)) {
	    va_copy(args, fm->args);
	    argn = fm->argn;
	    fp = fm->fp;
	}
	ft = fm->ft;
	free(fm);
	if (form && form[0])
	    goto loop_fmt;
    }

  done:
    free(fp);
    while ((fm = fmstk)) {
	fmstk = fm->next;
	free(fm);
    }

    return n_output;
}
