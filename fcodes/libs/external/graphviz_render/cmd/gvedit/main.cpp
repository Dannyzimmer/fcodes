/**
 * @file
 * @brief gvedit - simple graph editor and viewer
 */

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

#ifdef _WIN32
#include "windows.h"
#endif
#include <stdio.h>
#include <QApplication>
#include <QFile>
#include "mainwindow.h"

#include <getopt.h>
#include <gvc/gvc.h>
#include <common/globals.h>
#include <cgraph/exit.h>


#ifdef _MSC_VER
#pragma comment( lib, "cgraph.lib" )
#pragma comment( lib, "gvc.lib" )
#pragma comment( lib, "ingraphs.lib" )

#endif

QTextStream errout(stderr, QIODevice::WriteOnly);

static char useString[] = "Usage: gvedit [-v?] <files>\n\
  -s    - Scale input by 72\n\
  -v    - verbose\n\
  -?    - print usage\n";

static void usage(int v)
{
    printf("%s",useString);
    graphviz_exit(v);
}

static char **parseArgs(int argc, char *argv[])
{
    int c;

    const char *cmd = argv[0];
    while ((c = getopt(argc, argv, ":sv?")) != -1) {
	switch (c) {
	case 's':
	    PSinputscale = POINTS_PER_INCH;
	    break;
	case 'v':
	    Verbose = 1;
	    break;
	case '?':
	    if (optopt == '\0' || optopt == '?')
		usage(0);
	    else {
		errout << cmd << " : option -" << ((char) optopt) <<
		    " unrecognized\n";
		errout.flush();
		usage(1);
	    }
	    break;
	}
    }

    argv += optind;
    argc -= optind;

    if (argc)
	return argv;
    else
	return nullptr;
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(mdi);
    int ret;

    char **files = parseArgs(argc, argv);
    QApplication app(argc, argv);
    CMainWindow mainWin(files);
    mainWin.show();
    ret = app.exec();
    graphviz_exit(ret);
}

/**
 * @dir .
 * @brief simple graph editor and viewer
 */
