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
#include	<stddef.h>

/*	Dealing with $ argument addressing stuffs.
**
**	Written by Kiem-Phong Vo.
*/

static char *sffmtint(const char *str, int *v)
{
    for (*v = 0; isdigit((int)*str); ++str)
	*v = *v * 10 + (*str - '0');
    *v -= 1;
    return (char *) str;
}

/* table for floating point and integer conversions */
Sftab_t _Sftable = {
    .sf_pos10 = {1e1, 1e2, 1e4, 1e8, 1e16, 1e32},
    .sf_neg10 = {1e-1, 1e-2, 1e-4, 1e-8, 1e-16, 1e-32},

    .sf_dec =
    {'0', '0', '0', '1', '0', '2', '0', '3', '0', '4',
     '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
     '1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
     '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
     '2', '0', '2', '1', '2', '2', '2', '3', '2', '4',
     '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
     '3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
     '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
     '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
     '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
     '5', '0', '5', '1', '5', '2', '5', '3', '5', '4',
     '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
     '6', '0', '6', '1', '6', '2', '6', '3', '6', '4',
     '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
     '7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
     '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
     '8', '0', '8', '1', '8', '2', '8', '3', '8', '4',
     '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
     '9', '0', '9', '1', '9', '2', '9', '3', '9', '4',
     '9', '5', '9', '6', '9', '7', '9', '8', '9', '9',
     },

    .sf_digits = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_",

    .sf_fmtintf = sffmtint,
    .sf_cv36 =
        {
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            0,        1,        2,        3,        4,        5,
            6,        7,        8,        9,        SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, 62,       10,
            11,       12,       13,       14,       15,       16,
            17,       18,       19,       20,       21,       22,
            23,       24,       25,       26,       27,       28,
            29,       30,       31,       32,       33,       34,
            35,       SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, 63,
            SF_RADIX, 10,       11,       12,       13,       14,
            15,       16,       17,       18,       19,       20,
            21,       22,       23,       24,       25,       26,
            27,       28,       29,       30,       31,       32,
            33,       34,       35,       SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
        },
    .sf_cv64 =
        {
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            0,        1,        2,        3,        4,        5,
            6,        7,        8,        9,        SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, 62,       36,
            37,       38,       39,       40,       41,       42,
            43,       44,       45,       46,       47,       48,
            49,       50,       51,       52,       53,       54,
            55,       56,       57,       58,       59,       60,
            61,       SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, 63,
            SF_RADIX, 10,       11,       12,       13,       14,
            15,       16,       17,       18,       19,       20,
            21,       22,       23,       24,       25,       26,
            27,       28,       29,       30,       31,       32,
            33,       34,       35,       SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
            SF_RADIX, SF_RADIX, SF_RADIX, SF_RADIX,
        },
    .sf_type = {
        ['d'] = SFFMT_INT,
        ['i'] = SFFMT_INT,
        ['u'] = SFFMT_UINT,
        ['o'] = SFFMT_UINT,
        ['x'] = SFFMT_UINT,
        ['X'] = SFFMT_UINT,
        ['e'] = SFFMT_FLOAT,
        ['E'] = SFFMT_FLOAT,
        ['g'] = SFFMT_FLOAT,
        ['G'] = SFFMT_FLOAT,
        ['f'] = SFFMT_FLOAT,
        ['s'] = SFFMT_POINTER,
        ['n'] = SFFMT_POINTER,
        ['p'] = SFFMT_POINTER,
        ['!'] = SFFMT_POINTER,
        ['c'] = SFFMT_BYTE,
        ['['] = SFFMT_CLASS,
    }
};
