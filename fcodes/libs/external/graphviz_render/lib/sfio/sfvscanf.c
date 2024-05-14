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
#include	<limits.h>
#include	<sfio/sfhdr.h>
#include	<stdbool.h>
#include	<stddef.h>
#include	<stdio.h>

/*	The main engine for reading formatted data
**
**	Written by Kiem-Phong Vo.
*/

#define MAXWIDTH	INT_MAX	// max amount to scan

/**
 * @param form format string
 * @param accept accepted characters are set to 1
 */
static const unsigned char *setclass(const unsigned char *form, bool *accept) {
    int fmt, c;
    bool yes;

    if ((fmt = *form++) == '^') {	/* we want the complement of this set */
	yes = false;
	fmt = *form++;
    } else
	yes = true;

    for (c = 0; c <= UCHAR_MAX; ++c)
	accept[c] = !yes;

    if (fmt == ']' || fmt == '-') {	/* special first char */
	accept[fmt] = yes;
	fmt = *form++;
    }

    for (; fmt != ']'; fmt = *form++) {	/* done */
	if (!fmt)
	    return (form - 1);

	/* interval */
	if (fmt != '-' || form[0] == ']' || form[-2] > form[0])
	    accept[fmt] = yes;
	else
	    for (c = form[-2] + 1; c < form[0]; ++c)
		accept[c] = yes;
    }

    return form;
}

/**
 * @param f file to be scanned
 * @param form scanning format
 * @param args
 */
int sfvscanf(FILE *f, const char *form, va_list args)
{
    int inp, shift, base, width;
    ssize_t size;
    int fmt, flags, dot, n_assign, v, n;
    char *sp;
    char accept[SF_MAXDIGITS];

    Argv_t argv;
    Sffmt_t *ft;
    Fmt_t *fm, *fmstk;

    char *oform;
    va_list oargs;
    int argp, argn;

    void *value;		/* location to assign scanned value */
    const char *t_str;
    ssize_t n_str;

#define SFGETC(f,c)	((c) = getc(f))
#define SFUNGETC(f,c)	ungetc((c), (f))

    assert(f != NULL);

    if (!form)
	return -1;

    n_assign = 0;

    inp = -1;

    fmstk = NULL;
    ft = NULL;

    argn = -1;
    oform = (char *) form;
    va_copy(oargs, args);

  loop_fmt:
    while ((fmt = *form++)) {
	if (fmt != '%') {
	    if (isspace(fmt)) {
		if (fmt != '\n')
		    fmt = -1;
		for (;;) {
		    if (SFGETC(f, inp) < 0 || inp == fmt)
			goto loop_fmt;
		    else if (!isspace(inp)) {
			SFUNGETC(f, inp);
			goto loop_fmt;
		    }
		}
	    } else {
	      match_1:
		if (SFGETC(f, inp) != fmt) {
		    if (inp >= 0)
			SFUNGETC(f, inp);
		    goto pop_fmt;
		}
	    }
	    continue;
	}

	if (*form == '%') {
	    form += 1;
	    goto match_1;
	}

	if (*form == '\0')
	    goto pop_fmt;

	if (*form == '*') {
	    flags = SFFMT_SKIP;
	    form += 1;
	} else
	    flags = 0;

	/* matching some pattern */
	base = 10;
	size = -1;
	width = dot = 0;
	t_str = NULL;
	n_str = 0;
	value = NULL;
	argp = -1;

      loop_flags:		/* LOOP FOR FLAGS, WIDTH, BASE, TYPE */
	switch ((fmt = *form++)) {
	case LEFTP:		/* get the type which is enclosed in balanced () */
	    t_str = form;
	    for (v = 1;;) {
		switch (*form++) {
		case 0:	/* not balanceable, retract */
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

			if (ft && ft->extf) {
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

	case '#':		/* alternative format */
	    flags |= SFFMT_ALTER;
	    goto loop_flags;

	case '.':		/* width & base */
	    dot += 1;
	    if (isdigit((int)*form)) {
		fmt = *form++;
		goto dot_size;
	    } else if (*form == '*') {
		form = _Sffmtintf(form + 1, &n);
		n = FP_SET(-1, argn);

		if (ft && ft->extf) {
		    FMTSET(ft, form, args, '.', dot, 0, 0, 0, 0,
			   NULL, 0);
		    if (ft->extf(&argv, ft) < 0)
			goto pop_fmt;
		    if (ft->flags & SFFMT_VALUE)
			v = argv.i;
		    else
			v = (dot <= 2) ? va_arg(args, int) : 0;
		} else
		    v = (dot <= 2) ? va_arg(args, int) : 0;
		if (v < 0)
		    v = 0;
		goto dot_set;
	    } else
		goto loop_flags;

	case '0':
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
	    if (dot == 0 || dot == 1)
		width = v;
	    else if (dot == 2)
		base = v;
	    goto loop_flags;

	case 'I':		/* object size */
	    size = 0;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_IFLAG;
	    if (isdigit((int)*form)) {
		for (n = *form; isdigit(n); n = *++form)
		    size = size * 10 + (n - '0');
	    } else if (*form == '*') {
		form = _Sffmtintf(form + 1, &n);
		n = FP_SET(-1, argn);

		if (ft && ft->extf) {
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

	/* set object size */
	if (flags & (SFFMT_TYPES & ~SFFMT_IFLAG)) {
	    if ((_Sftype[fmt] & (SFFMT_INT | SFFMT_UINT)) || fmt == 'n') {
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
	if (ft && ft->extf) {
	    FMTSET(ft, form, args, fmt, size, flags, width, 0, base, t_str,
		   n_str);
	    v = ft->extf(&argv, ft);

	    if (v < 0)
		goto pop_fmt;
	    else if (v == 0) {	/* extf did not use input stream */
		FMTGET(ft, form, args, fmt, size, flags, width, n, base);
		if ((ft->flags & SFFMT_VALUE) && !(ft->flags & SFFMT_SKIP))
		    value = argv.vp;
	    } else {		/* v > 0: number of input bytes consumed */
		if (!(ft->flags & SFFMT_SKIP))
		    n_assign += 1;
		continue;
	    }
	}

	if (_Sftype[fmt] == 0)	/* unknown pattern */
	    continue;

	if (fmt == '!') {
	    if (!(argv.ft = va_arg(args, Sffmt_t *)))
		continue;
	    if (!argv.ft->form && ft) {	/* change extension functions */
		fmstk->ft = ft = argv.ft;
	    } else {		/* stack a new environment */
		if (!(fm = malloc(sizeof(Fmt_t))))
		    goto done;

		if (argv.ft->form) {
		    fm->form = (char *) form;
		    va_copy(fm->args, args);

		    fm->oform = oform;
		    va_copy(fm->oargs, oargs);
		    fm->argn = argn;

		    form = argv.ft->form;
		    va_copy(args, argv.ft->args);
		    argn = -1;
		} else
		    fm->form = NULL;

		fm->ft = ft;
		fm->next = fmstk;
		fmstk = fm;
		ft = argv.ft;
	    }
	    continue;
	}

	/* get the address to assign value */
	if (!value && !(flags & SFFMT_SKIP))
	    value = va_arg(args, void *);

	/* if get here, start scanning input */
	if (width == 0)
	    width = fmt == 'c' ? 1 : MAXWIDTH;

	/* define the first input character */
	if (fmt == 'c' || fmt == '[')
	    SFGETC(f, inp);
	else {
	    do {
		SFGETC(f, inp);
	    }
	    while (isspace(inp))	/* skip starting blanks */
	    ;
	}
	if (inp < 0)
	    goto done;

	if (_Sftype[fmt] == SFFMT_FLOAT) {
	    char *val;

	    val = accept;
	    if (width >= SF_MAXDIGITS)
		width = SF_MAXDIGITS - 1;
	    int exponent = 0;
	    bool seen_dot = false;
	    do {
		if (isdigit(inp))
		    *val++ = inp;
		else if (inp == '.') {	/* too many dots */
		    if (seen_dot)
			break;
		    seen_dot = true;
		    *val++ = '.';
		} else if (inp == 'e' || inp == 'E') {	/* too many e,E */
		    if (exponent++ > 0)
			break;
		    *val++ = inp;
		    if (--width <= 0 || SFGETC(f, inp) < 0 ||
			(inp != '-' && inp != '+' && !isdigit(inp)))
			break;
		    *val++ = inp;
		} else if (inp == '-' || inp == '+') {	/* too many signs */
		    if (val > accept)
			break;
		    *val++ = inp;
		} else
		    break;

	    } while (--width > 0 && SFGETC(f, inp) >= 0);

	    if (value) {
		*val = '\0';
		    argv.d = strtod(accept, NULL);

		n_assign += 1;
		if (FMTCMP(size, double, Sfdouble_t))
		    *((double *) value) = argv.d;
		else
		    *((float *) value) = (float) argv.d;
	    }
	} else if (_Sftype[fmt] == SFFMT_UINT || fmt == 'p') {
	    if (inp == '-') {
		SFUNGETC(f, inp);
		goto pop_fmt;
	    } else
		goto int_cvt;
	} else if (_Sftype[fmt] == SFFMT_INT) {
	  int_cvt:
	    if (inp == '-' || inp == '+') {
		if (inp == '-')
		    flags |= SFFMT_MINUS;
		while (--width > 0 && SFGETC(f, inp) >= 0)
		    if (!isspace(inp))
			break;
	    }
	    if (inp < 0)
		goto done;

	    if (fmt == 'o')
		base = 8;
	    else if (fmt == 'x' || fmt == 'p')
		base = 16;
	    else if (fmt == 'i' && inp == '0') {	/* self-described data */
		base = 8;
		if (width > 1) {	/* peek to see if it's a base-16 */
		    if (SFGETC(f, inp) >= 0) {
			if (inp == 'x' || inp == 'X')
			    base = 16;
			SFUNGETC(f, inp);
		    }
		    inp = '0';
		}
	    }

	    /* now convert */
	    argv.lu = 0;
	    if (base == 16) {
		sp = (char *) _Sfcv36;
		shift = 4;
		if (sp[inp] >= 16) {
		    SFUNGETC(f, inp);
		    goto pop_fmt;
		}
		if (inp == '0' && --width > 0) {	/* skip leading 0x or 0X */
		    if (SFGETC(f, inp) >= 0 &&
			(inp == 'x' || inp == 'X') && --width > 0)
			SFGETC(f, inp);
		}
		if (inp >= 0 && sp[inp] < 16)
		    goto base_shift;
	    } else if (base == 10) {	/* fast base 10 conversion */
		if (inp < '0' || inp > '9') {
		    SFUNGETC(f, inp);
		    goto pop_fmt;
		}

		do {
		    argv.lu =
			(argv.lu << 3) + (argv.lu << 1) + (inp - '0');
		} while (--width > 0 && SFGETC(f, inp) >= '0'
			 && inp <= '9');

		if (fmt == 'i' && inp == '#' && !(flags & SFFMT_ALTER)) {
		    base = (int) argv.lu;
		    if (base < 2 || base > SF_RADIX)
			goto pop_fmt;
		    argv.lu = 0;
		    sp = base <= 36 ? (char *) _Sfcv36 : (char *) _Sfcv64;
		    if (--width > 0 &&
			SFGETC(f, inp) >= 0 && sp[inp] < base)
			goto base_conv;
		}
	    } else {		/* other bases */
		sp = base <= 36 ? (char *) _Sfcv36 : (char *) _Sfcv64;
		if (base < 2 || base > SF_RADIX || sp[inp] >= base) {
		    SFUNGETC(f, inp);
		    goto pop_fmt;
		}

	      base_conv:	/* check for power of 2 conversions */
		if ((base & ~(base - 1)) == base) {
		    if (base < 8)
			shift = base < 4 ? 1 : 2;
		    else if (base < 32)
			shift = base < 16 ? 3 : 4;
		    else
			shift = base < 64 ? 5 : 6;

		  base_shift:do {
			argv.lu = (argv.lu << shift) + sp[inp];
		    } while (--width > 0 &&
			     SFGETC(f, inp) >= 0 && sp[inp] < base);
		} else {
		    do {
			argv.lu = (argv.lu * base) + sp[inp];
		    } while (--width > 0 &&
			     SFGETC(f, inp) >= 0 && sp[inp] < base);
		}
	    }

	    if (flags & SFFMT_MINUS)
		argv.ll = -argv.ll;

	    if (value) {
		n_assign += 1;

		if (fmt == 'p') {
		    *((void **) value) = (void *)(uintptr_t)argv.lu;
		} else if (sizeof(long) > sizeof(int) &&
			 FMTCMP(size, long, Sflong_t)) {
		    if (fmt == 'd' || fmt == 'i')
			*((long *) value) = (long) argv.ll;
		    else
			*((ulong *) value) = (ulong) argv.lu;
		} else if (sizeof(short) < sizeof(int) &&
			   FMTCMP(size, short, Sflong_t)) {
		    if (fmt == 'd' || fmt == 'i')
			*((short *) value) = (short) argv.ll;
		    else
			*((ushort *) value) = (ushort) argv.lu;
		} else if (size == sizeof(char)) {
		    if (fmt == 'd' || fmt == 'i')
			*((char *) value) = (char) argv.ll;
		    else
			*((uchar *) value) = (uchar) argv.lu;
		} else {
		    if (fmt == 'd' || fmt == 'i')
			*((int *) value) = (int) argv.ll;
		    else
			*((unsigned*)value) = (unsigned)argv.lu;
		}
	    }
	} else if (fmt == 's' || fmt == 'c' || fmt == '[') {
	    if (size < 0)
		size = MAXWIDTH;
	    if (value) {
		argv.s = (char *) value;
		if (fmt != 'c')
		    size -= 1;
	    } else
		size = 0;

	    n = 0;
	    if (fmt == 's') {
		do {
		    if (isspace(inp))
			break;
		    if ((n += 1) <= size)
			*argv.s++ = inp;
		} while (--width > 0 && SFGETC(f, inp) >= 0);
	    } else if (fmt == 'c') {
		do {
		    if ((n += 1) <= size)
			*argv.s++ = inp;
		} while (--width > 0 && SFGETC(f, inp) >= 0);
	    } else {		/* if(fmt == '[') */
		bool accepted[UCHAR_MAX + 1];
		form = (const char*)setclass((const unsigned char*)form, accepted);
		do {
		    if (!accepted[inp]) {
			if (n > 0 || (flags & SFFMT_ALTER))
			    break;
			else {
			    SFUNGETC(f, inp);
			    goto pop_fmt;
			}
		    }
		    if ((n += 1) <= size)
			*argv.s++ = inp;
		} while (--width > 0 && SFGETC(f, inp) >= 0);
	    }

	    if (value && (n > 0 || fmt == '[')) {
		n_assign += 1;
		if (fmt != 'c' && size >= 0)
		    *argv.s = '\0';
	    }
	}

	if (width > 0 && inp >= 0)
	    SFUNGETC(f, inp);
    }

  pop_fmt:
    while ((fm = fmstk)) {	/* pop the format stack and continue */
	fmstk = fm->next;
	if ((form = fm->form)) {
	    va_copy(args, fm->args);
	    oform = fm->oform;
	    va_copy(oargs, fm->oargs);
	    argn = fm->argn;
	}
	ft = fm->ft;
	free(fm);
	if (form && form[0])
	    goto loop_fmt;
    }

  done:
    while ((fm = fmstk)) {
	fmstk = fm->next;
	free(fm);
    }

    if (n_assign == 0 && inp < 0)
	n_assign = -1;

    return n_assign;
}
