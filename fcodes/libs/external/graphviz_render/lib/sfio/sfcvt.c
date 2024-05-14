/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include	<limits.h>
#include	<sfio/sfhdr.h>

/*	Convert a floating point value to ASCII
**
**	Written by Kiem-Phong Vo
*/

static char *Inf = "Inf", *Zero = "0";
#define SF_INTPART	(SF_IDIGITS/2)
#define SF_INFINITE	((_Sfi = 3), Inf)
#define SF_ZERO		((_Sfi = 1), Zero)

/**
 * @param dv value to convert
 * @param n_digit number of digits wanted
 * @param decpt return decimal point
 * @param sign return sign
 * @param format conversion format
 */
char *_sfcvt(void * dv, int n_digit, int *decpt, int *sign, int format)
{
    char *sp;
    long n, v;
    char *ep, *buf, *endsp;
    static char Buf[SF_MAXDIGITS];

    *sign = *decpt = 0;
    {
	double dval = *(double *)dv;

	if (dval == 0.)
	    return SF_ZERO;
	else if ((*sign = dval < 0.))	/* assignment = */
	    dval = -dval;

	n = 0;
	if (dval >= (double)LONG_MAX) {	/* scale to a small enough number to fit an int */
	    v = SF_MAXEXP10 - 1;
	    do {
		if (dval < _Sfpos10[v])
		    v -= 1;
		else {
		    dval *= _Sfneg10[v];
		    if ((n += 1 << v) >= SF_IDIGITS)
			return SF_INFINITE;
		}
	    } while (dval >= (double)LONG_MAX);
	}
	*decpt = (int) n;

	buf = sp = Buf + SF_INTPART;
	if ((v = (int) dval) != 0) {	/* translate the integer part */
	    dval -= (double) v;

	    sfucvt(v, sp, n, ep, long, ulong);

	    n = buf - sp;
	    if ((*decpt += (int) n) >= SF_IDIGITS)
		return SF_INFINITE;
	    buf = sp;
	    sp = Buf + SF_INTPART;
	} else
	    n = 0;

	/* remaining number of digits to compute; add 1 for later rounding */
	n = (((format & SFFMT_EFORMAT)
	      || *decpt <= 0) ? 1 : *decpt + 1) - n;
	if (n_digit > 0)
	    n += n_digit;

	if ((ep = sp + n) > (endsp = Buf + (SF_MAXDIGITS - 2)))
	    ep = endsp;
	if (sp > ep)
	    sp = ep;
	else {
	    if ((format & SFFMT_EFORMAT) && *decpt == 0 && dval > 0.) {
		double d;
		while ((int) (d = dval * 10.) == 0) {
		    dval = d;
		    *decpt -= 1;
		}
	    }

	    while (sp < ep) {	/* generate fractional digits */
		if (dval <= 0.) {	/* fill with 0's */
		    do {
			*sp++ = '0';
		    } while (sp < ep);
		    goto done;
		} else if ((n = (int) (dval *= 10.)) < 10) {
		    *sp++ = (char) ('0' + n);
		    dval -= n;
		} else {	/* n == 10 */
		    do {
			*sp++ = '9';
		    } while (sp < ep);
		}
	    }
	}
    }

    if (ep <= buf)
	ep = buf + 1;
    else if (ep < endsp) {	/* round the last digit */
	*--sp += 5;
	while (*sp > '9') {
	    *sp = '0';
	    if (sp > buf)
		*--sp += 1;
	    else {		/* next power of 10 */
		*sp = '1';
		*decpt += 1;
		if (!(format & SFFMT_EFORMAT)) {	/* add one more 0 for %f precision */
		    ep[-1] = '0';
		    ep += 1;
		}
	    }
	}
    }

  done:
    *--ep = '\0';
    _Sfi = ep - buf;
    return buf;
}
