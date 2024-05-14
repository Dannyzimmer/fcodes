#pragma once

#include <common/geom.h>
#include <gd.h>

void gdgen_text(gdImagePtr im, pointf spf, pointf epf, int fontcolor,
                double fontsize, int fontdpi, double fontangle, char *fontname,
                char *str);
