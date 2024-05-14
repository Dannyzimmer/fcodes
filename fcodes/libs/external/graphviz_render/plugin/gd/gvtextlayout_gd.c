/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"
#include <cgraph/strview.h>
#include "gd_psfontResolve.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gvc/gvplugin_textlayout.h>
#include <gd.h>
#include <cgraph/strcasecmp.h>
#include <common/const.h>

#ifdef HAVE_GD_FREETYPE

/* fontsize at which text is omitted entirely */
#define FONTSIZE_MUCH_TOO_SMALL 0.15
/* fontsize at which text is rendered by a simple line */
#define FONTSIZE_TOO_SMALL 1.5

#ifndef HAVE_GD_FONTCONFIG
/* gd_alternate_fontlist;
 * Sometimes fonts are stored under a different name,
 * especially on Windows. Without fontconfig, we provide
 * here some rudimentary name mapping.
 */
char *gd_alternate_fontlist(const char *font) {
    char *p;

    /* fontbuf to contain font without style descriptions like -Roman or -Italic */
    strview_t fontlist = strview(font, '\0');
    if ((p = strchr(font, '-')) || (p = strchr(font, '_')))
	fontlist.size = (size_t)(p - font);

    if ((strcasecmp(font, "times-bold") == 0)
	|| strview_case_str_eq(fontlist, "timesbd")
	|| strview_case_str_eq(fontlist, "timesb"))
	fontlist = strview("timesbd;Timesbd;TIMESBD;timesb;Timesb;TIMESB", '\0');

    else if ((strcasecmp(font, "times-italic") == 0)
	     || strview_case_str_eq(fontlist, "timesi"))
	fontlist = strview("timesi;Timesi;TIMESI", '\0');

    else if ((strcasecmp(font, "timesnewroman") == 0)
	     || (strcasecmp(font, "timesnew") == 0)
	     || (strcasecmp(font, "timesroman") == 0)
	     || strview_case_str_eq(fontlist, "times"))
	fontlist = strview("times;Times;TIMES", '\0');

    else if ((strcasecmp(font, "arial-bold") == 0)
	     || strview_case_str_eq(fontlist, "arialb"))
	fontlist = strview("arialb;Arialb;ARIALB", '\0');

    else if ((strcasecmp(font, "arial-italic") == 0)
	     || strview_case_str_eq(fontlist, "ariali"))
	fontlist = strview("ariali;Ariali;ARIALI", '\0');

    else if (strview_case_str_eq(fontlist, "helvetica"))
	fontlist = strview("helvetica;Helvetica;HELVETICA;arial;Arial;ARIAL", '\0');

    else if (strview_case_str_eq(fontlist, "arial"))
	fontlist = strview("arial;Arial;ARIAL", '\0');

    else if (strview_case_str_eq(fontlist, "courier"))
	fontlist = strview("courier;Courier;COURIER;cour", '\0');

    return strview_str(fontlist);
}
#endif				/* HAVE_GD_FONTCONFIG */

/* gd_psfontResolve:
 *  * Construct alias for postscript fontname.
 *   * NB. Uses a static array - non-reentrant.
 *    */

#define ADD_ATTR(a) \
  if (a) { \
        strcat(buf, comma ? " " : ", "); \
        comma = 1; \
        strcat(buf, a); \
  }

char* gd_psfontResolve (PostscriptAlias* pa)
{
    static char buf[1024];
    int comma=0;
    strcpy(buf, pa->family);

    ADD_ATTR(pa->weight);
    ADD_ATTR(pa->stretch);
    ADD_ATTR(pa->style);
   
    return buf;
}

static bool gd_textlayout(textspan_t * span, char **fontpath)
{
    char *err, *fontlist, *fontname;
    double fontsize;
    int brect[8];
    gdFTStringExtra strex;
#ifdef HAVE_GD_FONTCONFIG
    PostscriptAlias *pA;
#endif

    fontname = span->font->name;
    fontsize = span->font->size;

    strex.fontpath = NULL;
    strex.flags = gdFTEX_RETURNFONTPATHNAME | gdFTEX_RESOLUTION;
    strex.hdpi = strex.vdpi = POINTS_PER_INCH;

    if (strchr(fontname, '/'))
	strex.flags |= gdFTEX_FONTPATHNAME;
    else
	strex.flags |= gdFTEX_FONTCONFIG;

    span->size.x = 0.0;
    span->size.y = 0.0;
    span->yoffset_layout = 0.0;

    span->layout = NULL;
    span->free_layout = NULL;

    span->yoffset_centerline = 0.05 * fontsize;

    if (fontname) {
	if (fontsize <= FONTSIZE_MUCH_TOO_SMALL) {
	    return true; /* OK, but ignore text entirely */
	} else if (fontsize <= FONTSIZE_TOO_SMALL) {
	    /* draw line in place of text */
	    /* fake a finite fontsize so that line length is calculated */
	    fontsize = FONTSIZE_TOO_SMALL;
	}
	/* call gdImageStringFT with null *im to get brect and to set font cache */
#ifdef HAVE_GD_FONTCONFIG
	gdFTUseFontConfig(1);  /* tell gd that we really want to use fontconfig, 'cos it s not the default */
	pA = span->font->postscript_alias;
	if (pA)
	    fontlist = gd_psfontResolve (pA);
	else
	    fontlist = fontname;
#else
	fontlist = gd_alternate_fontlist(fontname);
#endif

	err = gdImageStringFTEx(NULL, brect, -1, fontlist,
				fontsize, 0, 0, 0, span->str, &strex);
#ifndef HAVE_GD_FONTCONFIG
	free(fontlist);
#endif

	if (err) {
	    agerr(AGERR,"%s\n", err);
	    return false; /* indicate error */
	}

	if (fontpath)
	    *fontpath = strex.fontpath;
	else
	    free (strex.fontpath); /* strup'ed in libgd */

	if (span->str && span->str[0]) {
	    /* can't use brect on some archtectures if strlen 0 */
	    span->size.x = (double) (brect[4] - brect[0]);
	    // LINESPACING specifies how much extra space to leave between lines
	    span->size.y = fontsize * LINESPACING;
	}
    }
    return true;
}

static gvtextlayout_engine_t gd_textlayout_engine = {
    gd_textlayout,
};
#endif

gvplugin_installed_t gvtextlayout_gd_types[] = {
#ifdef HAVE_GD_FREETYPE
    {0, "textlayout", 2, &gd_textlayout_engine, NULL},
#endif
    {0, NULL, 0, NULL, NULL}
};
