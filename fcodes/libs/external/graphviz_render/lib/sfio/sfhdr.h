/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

/*	Internal definitions for sfio.
**	Written by Kiem-Phong Vo
*/

#include	<sfio/sfio.h>
#include	"config.h"

#include	<limits.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdint.h>
#include	<stddef.h>

#include	<fcntl.h>

#include	<unistd.h>

#include	<errno.h>
#include	<ctype.h>

/* short-hands */
#ifndef uchar
#define uchar		unsigned char
#endif
#ifndef ulong
#define ulong		uint64_t
#endif
#ifndef ushort
#define ushort		unsigned short
#endif

/* function to get the decimal point for local environment */
#include	<locale.h>
#define SFSETLOCALE(decimal,thousand) \
	{ struct lconv*	lv_; \
	  if((decimal) == 0) \
	  { (decimal) = '.'; \
	    if((lv_ = localeconv())) \
	    { if(lv_->decimal_point && lv_->decimal_point[0]) \
	    	(decimal) = lv_->decimal_point[0]; \
	      if(lv_->thousands_sep && lv_->thousands_sep[0]) \
	    	(thousand) = lv_->thousands_sep[0]; \
	    } \
	  } \
	}

/* extensions to sfvprintf/sfvscanf */
#define FP_SET(fp,fn)	(fp < 0 ? (fn += 1) : (fn = fp) )
#define FP_WIDTH	0
#define FP_PRECIS	1
#define FP_BASE		2
#define FP_STR		3
#define FP_SIZE		4
#define FP_INDEX	5	/* index size   */

    typedef struct _fmt_s Fmt_t;
    typedef struct _fmtpos_s Fmtpos_t;
    typedef union {
	int i, *ip;
	long l, *lp;
	short h, *hp;
	unsigned ui;
	ulong ul;
	ushort uh;
	Sflong_t ll, *llp;
	Sfulong_t lu;
	Sfdouble_t ld;
	double d;
	float f;
	char c, *s, **sp;
	void *vp;
	Sffmt_t *ft;
    } Argv_t;

    struct _fmt_s {
	char *form;		/* format string                */
	va_list args;		/* corresponding arglist        */

	char *oform;		/* original format string       */
	va_list oargs;		/* original arg list            */
	int argn;		/* number of args already used  */
	Fmtpos_t *fp;		/* position list                */

	Sffmt_t *ft;		/* formatting environment       */
	Fmt_t *next;		/* stack frame pointer          */
    };

    struct _fmtpos_s {
	Sffmt_t ft;		/* environment                  */
	Argv_t argv;		/* argument value               */
	int fmt;		/* original format              */
	int need[FP_INDEX];	/* positions depending on       */
    };

#define LEFTP		'('
#define RIGHTP		')'
#define QUOTE		'\''

#define FMTSET(ft, frm,ags, fv, sz, flgs, wid,pr,bs, ts,ns) \
	((ft->form = (char*)frm), va_copy(ft->args,ags), \
	 (ft->fmt = fv), (ft->size = sz), \
	 (ft->flags = (flgs&SFFMT_SET)), \
	 (ft->width = wid), (ft->precis = pr), (ft->base = bs), \
	 (ft->t_str = ts), (ft->n_str = ns) )
#define FMTGET(ft, frm,ags, fv, sz, flgs, wid,pr,bs) \
	((frm = ft->form), va_copy(ags,ft->args), (fv = ft->fmt), (sz = ft->size), \
	 (flgs = (flgs&~(SFFMT_SET))|(ft->flags&SFFMT_SET)), \
	 (wid = ft->width), (pr = ft->precis), (bs = ft->base) )
#define FMTCMP(sz, type, maxtype) \
	(sz == sizeof(type) || (sz == 0 && sizeof(type) == sizeof(maxtype)) || \
	 (sz == 64 && sz == sizeof(type)*CHAR_BIT) )

/* format flags&types, must coexist with those in sfio.h */
#define SFFMT_EFORMAT	01000000000	/* sfcvt converting %e  */
#define SFFMT_MINUS	02000000000	/* minus sign           */

#define SFFMT_TYPES	(SFFMT_SHORT|SFFMT_SSHORT | SFFMT_LONG|SFFMT_LLONG|\
			 SFFMT_LDOUBLE | SFFMT_IFLAG|SFFMT_JFLAG| \
			 SFFMT_TFLAG | SFFMT_ZFLAG )

/* type of elements to be converted */
#define SFFMT_INT	001	/* %d,%i                */
#define SFFMT_UINT	002	/* %u,o,x etc.          */
#define SFFMT_FLOAT	004	/* %f,e,g etc.          */
#define SFFMT_BYTE	010	/* %c                   */
#define SFFMT_POINTER	020	/* %p, %n               */
#define SFFMT_CLASS	040	/* %[                   */

#define	SF_RADIX	64	/* maximum integer conversion base */

/* floating point to ascii conversion */
#define SF_MAXEXP10	6
#define SF_FDIGITS	256	/* max allowed fractional digits */
#define SF_IDIGITS	1024	/* max number of digits in int part */
#define SF_MAXDIGITS	(((SF_FDIGITS+SF_IDIGITS)/sizeof(int) + 1)*sizeof(int))

/* tables for numerical translation */
#define _Sfpos10	(_Sftable.sf_pos10)
#define _Sfneg10	(_Sftable.sf_neg10)
#define _Sfdec		(_Sftable.sf_dec)
#define _Sfdigits	(_Sftable.sf_digits)
#define _Sfcvinit	(_Sftable.sf_cvinit)
#define _Sffmtintf	(_Sftable.sf_fmtintf)
#define _Sfcv36		(_Sftable.sf_cv36)
#define _Sfcv64		(_Sftable.sf_cv64)
#define _Sftype		(_Sftable.sf_type)
    typedef struct _sftab_ {
	Sfdouble_t sf_pos10[SF_MAXEXP10];	/* positive powers of 10        */
	Sfdouble_t sf_neg10[SF_MAXEXP10];	/* negative powers of 10        */
	uchar sf_dec[200];	/* ascii reps of values < 100   */
	char *sf_digits;	/* digits for general bases     */
	int sf_cvinit;		/* initialization state         */
	char *(*sf_fmtintf) (const char *, int *);
	uchar sf_cv36[UCHAR_MAX + 1];	/* conversion for base [2-36]   */
	uchar sf_cv64[UCHAR_MAX + 1];	/* conversion for base [37-64]  */
	uchar sf_type[UCHAR_MAX + 1];	/* conversion formats&types     */
    } Sftab_t;

/* sfucvt() converts decimal integers to ASCII */
#define SFDIGIT(v,scale,digit) \
	{ if(v < 5*scale) \
		if(v < 2*scale) \
			if(v < 1*scale) \
				{ digit = '0'; } \
			else	{ digit = '1'; v -= 1*scale; } \
		else	if(v < 3*scale) \
				{ digit = '2'; v -= 2*scale; } \
			else if(v < 4*scale) \
				{ digit = '3'; v -= 3*scale; } \
			else	{ digit = '4'; v -= 4*scale; } \
	  else	if(v < 7*scale) \
			if(v < 6*scale) \
				{ digit = '5'; v -= 5*scale; } \
			else	{ digit = '6'; v -= 6*scale; } \
		else	if(v < 8*scale) \
				{ digit = '7'; v -= 7*scale; } \
			else if(v < 9*scale) \
				{ digit = '8'; v -= 8*scale; } \
			else	{ digit = '9'; v -= 9*scale; } \
	}
#define sfucvt(v,s,n,list,type,utype) \
	{ while((utype)v >= 10000) \
	  {	n = v; v = (type)(((utype)v)/10000); \
		n = (type)((utype)n - ((utype)v)*10000); \
	  	s -= 4; SFDIGIT(n,1000,s[0]); SFDIGIT(n,100,s[1]); \
			s[2] = *(list = (char*)_Sfdec + (n <<= 1)); s[3] = *(list+1); \
	  } \
	  if(v < 100) \
	  { if(v < 10) \
	    { 	s -= 1; s[0] = (char)('0'+v); \
	    } else \
	    { 	s -= 2; s[0] = *(list = (char*)_Sfdec + (v <<= 1)); s[1] = *(list+1); \
	    } \
	  } else \
	  { if(v < 1000) \
	    { 	s -= 3; SFDIGIT(v,100,s[0]); \
			s[1] = *(list = (char*)_Sfdec + (v <<= 1)); s[2] = *(list+1); \
	    } else \
	    {	s -= 4; SFDIGIT(v,1000,s[0]); SFDIGIT(v,100,s[1]); \
			s[2] = *(list = (char*)_Sfdec + (v <<= 1)); s[3] = *(list+1); \
	    } \
	  } \
	}

// handy function
#undef min
#define min(x,y)	((x) < (y) ? (x) : (y))

    extern Sftab_t _Sftable;

    extern char *_sfcvt(void *, int, int *, int *, int);

#ifdef __cplusplus
}
#endif
