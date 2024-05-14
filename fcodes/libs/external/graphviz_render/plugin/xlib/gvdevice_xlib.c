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

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif
#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <cgraph/agxbuf.h>
#include <cgraph/exit.h>
#include <gvc/gvplugin_device.h>

#include <cairo.h>
#ifdef CAIRO_HAS_XLIB_SURFACE
#include <cairo-xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

typedef struct window_xlib_s {
    Window win;
    uint64_t event_mask;
    Pixmap pix;
    GC gc;
    Visual *visual;
    Colormap cmap;
    int depth;
    Atom wm_delete_window_atom;
} window_t;

static void handle_configure_notify(GVJ_t * job, XConfigureEvent * cev)
{
/*FIXME - should allow for margins */
/*	- similar zoom_to_fit code exists in: */
/*	plugin/gtk/callbacks.c */
/*	plugin/xlib/gvdevice_xlib.c */
/*	lib/gvc/gvevent.c */

    assert(cev->width >= 0 && "Xlib returned an event with negative width");
    assert(cev->height >= 0 && "Xlib returned an event with negative height");

    job->zoom *= 1 + MIN(
	((double) cev->width - (double) job->width) / (double) job->width,
	((double) cev->height - (double) job->height) / (double) job->height);
    if ((unsigned)cev->width > job->width ||
        (unsigned)cev->height > job->height)
        job->has_grown = true;
    job->width = (unsigned)cev->width;
    job->height = (unsigned)cev->height;
    job->needs_refresh = true;
}

static void handle_expose(GVJ_t * job, XExposeEvent * eev)
{
    window_t *window;

    window = job->window;
    assert(eev->width >= 0 &&
           "Xlib returned an expose event with negative width");
    assert(eev->height >= 0 &&
           "Xlib returned an expose event with negative height");
    XCopyArea(eev->display, window->pix, eev->window, window->gc,
              eev->x, eev->y, (unsigned)eev->width, (unsigned)eev->height,
              eev->x, eev->y);
}

static void handle_client_message(GVJ_t * job, XClientMessageEvent * cmev)
{
    window_t *window;

    window = job->window;
    if (cmev->format == 32
        && (Atom) cmev->data.l[0] == window->wm_delete_window_atom)
        graphviz_exit(0);
}

static bool handle_keypress(GVJ_t *job, XKeyEvent *kev)
{
    
    int i;
    KeyCode *keycodes;

    keycodes = job->keycodes;
    for (i=0; i < job->numkeys; i++) {
	if (kev->keycode == keycodes[i])
	    return job->keybindings[i].callback(job) != 0;
    }
    return false;
}

static Visual *find_argb_visual(Display * dpy, int scr)
{
    XVisualInfo *xvi;
    XVisualInfo template;
    int nvi;
    int i;
    XRenderPictFormat *format;
    Visual *visual;

    template.screen = scr;
    template.depth = 32;
    template.class = TrueColor;
    xvi = XGetVisualInfo(dpy,
                         VisualScreenMask |
                         VisualDepthMask |
                         VisualClassMask, &template, &nvi);
    if (!xvi)
        return 0;
    visual = 0;
    for (i = 0; i < nvi; i++) {
        format = XRenderFindVisualFormat(dpy, xvi[i].visual);
        if (format->type == PictTypeDirect && format->direct.alphaMask) {
            visual = xvi[i].visual;
            break;
        }
    }

    XFree(xvi);
    return visual;
}

static void browser_show(GVJ_t *job)
{
#ifdef HAVE_SYS_TYPES_H
   char *exec_argv[3] = {BROWSER, NULL, NULL};
   pid_t pid;

   exec_argv[1] = job->selected_href;

   pid = fork();
   if (pid == -1) {
       fprintf(stderr,"fork failed: %s\n", strerror(errno));
   }
   else if (pid == 0) {
       execvp(exec_argv[0], exec_argv);
       fprintf(stderr,"error starting %s: %s\n", exec_argv[0], strerror(errno));
   }
#else
   fprintf(stdout,"browser_show: %s\n", job->selected_href);
#endif
}

static int handle_xlib_events (GVJ_t *firstjob, Display *dpy)
{
    GVJ_t *job;
    window_t *window;
    XEvent xev;
    pointf pointer;
    int rc = 0;

    while (XPending(dpy)) {
        XNextEvent(dpy, &xev);

        for (job = firstjob; job; job = job->next_active) {
	    window = job->window;
	    if (xev.xany.window == window->win) {
                switch (xev.xany.type) {
                case ButtonPress:
		    pointer.x = (double)xev.xbutton.x;
		    pointer.y = (double)xev.xbutton.y;
                    assert(xev.xbutton.button <= (unsigned)INT_MAX &&
                           "Xlib returned invalid button event");
                    job->callbacks->button_press(job, (int)xev.xbutton.button,
                                                 pointer);
		    rc++;
                    break;
                case MotionNotify:
		    if (job->button) { /* only interested while a button is pressed */
		        pointer.x = (double)xev.xbutton.x;
		        pointer.y = (double)xev.xbutton.y;
                        job->callbacks->motion(job, pointer);
		        rc++;
		    }
                    break;
                case ButtonRelease:
		    pointer.x = (double)xev.xbutton.x;
		    pointer.y = (double)xev.xbutton.y;
                    assert(xev.xbutton.button <= (unsigned)INT_MAX &&
                           "Xlib returned invalid button event");
                    job->callbacks->button_release(job, (int)xev.xbutton.button,
                                                   pointer);
		    if (job->selected_href && job->selected_href[0] && xev.xbutton.button == 1)
		        browser_show(job);
		    rc++;
                    break;
                case KeyPress:
		    if (handle_keypress(job, &xev.xkey))
			return -1;  /* exit code */
		    rc++;
                    break;
                case ConfigureNotify:
                    handle_configure_notify(job, &xev.xconfigure);
		    rc++;
                    break;
                case Expose:
                    handle_expose(job, &xev.xexpose);
		    rc++;
                    break;
                case ClientMessage:
                    handle_client_message(job, &xev.xclient);
		    rc++;
                    break;
                default:
                    break;
                }
	        break;
	    }
	}
    }
    return rc; 
}

static void update_display(GVJ_t *job, Display *dpy)
{
    window_t *window;
    cairo_surface_t *surface;

    window = job->window;

    // window geometry is set to fixed values
    assert(job->width <= (unsigned)INT_MAX && "out of range width");
    assert(job->height <= (unsigned)INT_MAX && "out of range height");

    if (job->has_grown) {
	XFreePixmap(dpy, window->pix);
	assert(window->depth >= 0 && "Xlib returned invalid window depth");
	window->pix = XCreatePixmap(dpy, window->win,
	                            job->width, job->height,
	                            (unsigned)window->depth);
	job->has_grown = false;
	job->needs_refresh = true;
    }
    if (job->needs_refresh) {
	XFillRectangle(dpy, window->pix, window->gc, 0, 0,
                	job->width, job->height);
	surface = cairo_xlib_surface_create(dpy,
			window->pix, window->visual,
			(int)job->width, (int)job->height);
    	job->context = cairo_create(surface);
	job->external_context = true;
        job->callbacks->refresh(job);
	cairo_surface_destroy(surface);
	XCopyArea(dpy, window->pix, window->win, window->gc,
			0, 0, job->width, job->height, 0, 0);
        job->needs_refresh = false;
    }
}

static void init_window(GVJ_t *job, Display *dpy, int scr)
{
    int argb = 0;
    const char *base = "";
    XGCValues gcv;
    XSetWindowAttributes attributes;
    XWMHints *wmhints;
    XSizeHints *normalhints;
    XClassHint *classhint;
    uint64_t attributemask = 0;
    window_t *window;
    double zoom_to_fit;

    window = malloc(sizeof(window_t));
    if (window == NULL) {
	fprintf(stderr, "Failed to malloc window_t\n");
	return;
    }

    unsigned w = 480;    /* FIXME - w,h should be set by a --geometry commandline option */
    unsigned h = 325;
    
    zoom_to_fit = MIN((double) w / (double) job->width,
			(double) h / (double) job->height);
    if (zoom_to_fit < 1.0) /* don't make bigger */
	job->zoom *= zoom_to_fit;

    job->width  = w;    /* use window geometry */
    job->height = h;

    job->window = window;
    job->fit_mode = false;
    job->needs_refresh = true;

    if (argb && (window->visual = find_argb_visual(dpy, scr))) {
        window->cmap = XCreateColormap(dpy, RootWindow(dpy, scr),
                                    window->visual, AllocNone);
        attributes.override_redirect = False;
        attributes.background_pixel = 0;
        attributes.border_pixel = 0;
        attributes.colormap = window->cmap;
        attributemask = ( CWBackPixel
                          | CWBorderPixel
			  | CWOverrideRedirect
			  | CWColormap );
        window->depth = 32;
    } else {
        window->cmap = DefaultColormap(dpy, scr);
        window->visual = DefaultVisual(dpy, scr);
        attributes.background_pixel = WhitePixel(dpy, scr);
        attributes.border_pixel = BlackPixel(dpy, scr);
        attributemask = (CWBackPixel | CWBorderPixel);
        window->depth = DefaultDepth(dpy, scr);
    }

    window->win = XCreateWindow(dpy, RootWindow(dpy, scr),
                             0, 0, job->width, job->height, 0, window->depth,
                             InputOutput, window->visual,
                             attributemask, &attributes);

    agxbuf name = {0};
    agxbprint(&name, "graphviz: %s", base);

    normalhints = XAllocSizeHints();
    normalhints->flags = 0;
    normalhints->x = 0;
    normalhints->y = 0;
    assert(job->width <= (unsigned)INT_MAX && "out of range width");
    normalhints->width = (int)job->width;
    assert(job->height <= (unsigned)INT_MAX && "out of range height");
    normalhints->height = (int)job->height;

    classhint = XAllocClassHint();
    classhint->res_name = "graphviz";
    classhint->res_class = "Graphviz";

    wmhints = XAllocWMHints();
    wmhints->flags = InputHint;
    wmhints->input = True;

    Xutf8SetWMProperties(dpy, window->win, agxbuse(&name), base, 0, 0,
                         normalhints, wmhints, classhint);
    XFree(wmhints);
    XFree(classhint);
    XFree(normalhints);
    agxbfree(&name);

    assert(window->depth >= 0 && "Xlib returned invalid window depth");
    window->pix = XCreatePixmap(dpy, window->win, job->width, job->height,
                                (unsigned)window->depth);
    if (argb)
        gcv.foreground = 0;
    else
        gcv.foreground = WhitePixel(dpy, scr);
    window->gc = XCreateGC(dpy, window->pix, GCForeground, &gcv);
    update_display(job, dpy);

    window->event_mask = (
          ButtonPressMask
        | ButtonReleaseMask
        | PointerMotionMask
        | KeyPressMask
        | StructureNotifyMask
        | ExposureMask);
    XSelectInput(dpy, window->win, (long)window->event_mask);
    window->wm_delete_window_atom =
        XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, window->win, &window->wm_delete_window_atom, 1);
    XMapWindow(dpy, window->win);
}

static int handle_stdin_events(GVJ_t *job)
{
    int rc=0;

    if (feof(stdin))
	return -1;
    job->callbacks->read(job, job->input_filename, job->layout_type);
    
    rc++;
    return rc;
}

#ifdef HAVE_SYS_INOTIFY_H
static int handle_file_events(GVJ_t *job, int inotify_fd)
{
    int avail, ret, len, rc = 0;
    char *bf, *p;
    struct inotify_event *event;

    ret = ioctl(inotify_fd, FIONREAD, &avail);
    if (ret < 0) {
	fprintf(stderr,"ioctl() failed\n");
	return -1;;
    }

    if (avail) {
        assert(avail > 0 && "invalid value from FIONREAD");
        void *buf = malloc((size_t)avail);
        if (!buf) {
            fprintf(stderr, "out of memory (could not allocate %d bytes)\n",
                    avail);
            return -1;
        }
        len = (int)read(inotify_fd, buf, (size_t)avail);
        if (len != avail) {
            fprintf(stderr,"avail = %d, len = %d\n", avail, len);
            free(buf);
            return -1;
        }
        bf = buf;
        while (len > 0) {
    	    event = (struct inotify_event *)bf;
	    if (event->mask == IN_MODIFY) {
		p = strrchr(job->input_filename, '/');
		if (p)
		    p++;
		else 
		    p = job->input_filename;
		if (strcmp(event->name, p) == 0) {
		    job->callbacks->read(job, job->input_filename, job->layout_type);
		    rc++;
		}
    	    }
	    size_t ln = event->len + sizeof(struct inotify_event);
            assert(ln <= (size_t)len);
            bf += ln;
            len -= (int)ln;
        }
        free(buf);
        if (len != 0) {
            fprintf(stderr,"length miscalculation, len = %d\n", len);
            return -1;
        }
    }
    return rc;
}
#endif

static bool initialized;

static void xlib_initialize(GVJ_t *firstjob)
{
    Display *dpy;
    KeySym keysym;
    KeyCode *keycodes;
    const char *display_name = NULL;
    int i, scr;

    dpy = XOpenDisplay(display_name);
    if (dpy == NULL) {
	fprintf(stderr, "Failed to open XLIB display: %s\n",
		XDisplayName(NULL));
	return;
    }
    scr = DefaultScreen(dpy);

    firstjob->display = dpy;
    firstjob->screen = scr;

    assert(firstjob->numkeys >= 0);
    keycodes = malloc((size_t)firstjob->numkeys * sizeof(KeyCode));
    if (keycodes == NULL) {
        fprintf(stderr, "Failed to malloc %d*KeyCode\n", firstjob->numkeys);
        return;
    }
    for (i = 0; i < firstjob->numkeys; i++) {
        keysym = XStringToKeysym(firstjob->keybindings[i].keystring);
        if (keysym == NoSymbol)
            fprintf(stderr, "ERROR: No keysym for \"%s\"\n",
		firstjob->keybindings[i].keystring);
        else
            keycodes[i] = XKeysymToKeycode(dpy, keysym);
    }
    firstjob->keycodes = keycodes;

    firstjob->device_dpi.x = DisplayWidth(dpy, scr) * 25.4 / DisplayWidthMM(dpy, scr);
    firstjob->device_dpi.y = DisplayHeight(dpy, scr) * 25.4 / DisplayHeightMM(dpy, scr);
    firstjob->device_sets_dpi = true;

    initialized = true;
}

static void xlib_finalize(GVJ_t *firstjob)
{
    GVJ_t *job;
    Display *dpy = firstjob->display;
    int scr = firstjob->screen;
    KeyCode *keycodes= firstjob->keycodes;
    int numfds, stdin_fd=0, xlib_fd, ret, events;
    fd_set rfds;
    bool watching_stdin_p = false;
#ifdef HAVE_SYS_INOTIFY_H
    int wd=0;
    int inotify_fd=0;
    bool watching_file_p = false;
    char *p, *cwd = NULL;

    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
	fprintf(stderr,"inotify_init() failed\n");
	return;
    }
#endif

    /* skip if initialization previously failed */
    if (!initialized) {
        return;
    }

    numfds = xlib_fd = XConnectionNumber(dpy);

    if (firstjob->input_filename) {
        if (firstjob->graph_index == 0) {
#ifdef HAVE_SYS_INOTIFY_H
	    watching_file_p = true;

	    agxbuf dir = {0};
	    if (firstjob->input_filename[0] != '/') {
    	        cwd = getcwd(NULL, 0);
	        agxbprint(&dir, "%s/%s", cwd, firstjob->input_filename);
	        free(cwd);
	    }
	    else {
	        agxbput(&dir, firstjob->input_filename);
	    }
	    char *dirstr = agxbuse(&dir);
	    p = strrchr(dirstr,'/');
	    *p = '\0';
    
    	    wd = inotify_add_watch(inotify_fd, dirstr, IN_MODIFY);
	    agxbfree(&dir);

            numfds = MAX(inotify_fd, numfds);
#endif
	}
    }
    else {
	watching_stdin_p = true;
	stdin_fd = fcntl(STDIN_FILENO, F_DUPFD, 0);
	numfds = MAX(stdin_fd, numfds);
    }

    for (job = firstjob; job; job = job->next_active)
	init_window(job, dpy, scr);

    /* This is the event loop */
    FD_ZERO(&rfds);
    while (1) {
	events = 0;

#ifdef HAVE_SYS_INOTIFY_H
	if (watching_file_p) {
	    if (FD_ISSET(inotify_fd, &rfds)) {
                ret = handle_file_events(firstjob, inotify_fd);
	        if (ret < 0)
	            break;
	        events += ret;
	    }
            FD_SET(inotify_fd, &rfds);
	}
#endif

	if (watching_stdin_p) {
	    if (FD_ISSET(stdin_fd, &rfds)) {
                ret = handle_stdin_events(firstjob);
	        if (ret < 0) {
	            watching_stdin_p = false;
                    FD_CLR(stdin_fd, &rfds);
                }
	        events += ret;
	    }
	    if (watching_stdin_p) 
	        FD_SET(stdin_fd, &rfds);
	}

	ret = handle_xlib_events(firstjob, dpy);
	if (ret < 0)
	    break;
	events += ret;
        FD_SET(xlib_fd, &rfds);

	if (events) {
            for (job = firstjob; job; job = job->next_active)
	        update_display(job, dpy);
	    XFlush(dpy);
	}

	ret = select(numfds+1, &rfds, NULL, NULL, NULL);
	if (ret < 0) {
	    fprintf(stderr,"select() failed\n");
	    break;
	}
    }

#ifdef HAVE_SYS_INOTIFY_H
    if (watching_file_p)
	ret = inotify_rm_watch(inotify_fd, wd);
#endif

    XCloseDisplay(dpy);
    free(keycodes);
    firstjob->keycodes = NULL;
}

static gvdevice_features_t device_features_xlib = {
    GVDEVICE_DOES_TRUECOLOR
	| GVDEVICE_EVENTS,      /* flags */
    {0.,0.},                    /* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},                  /* dpi */
};

static gvdevice_engine_t device_engine_xlib = {
    xlib_initialize,
    NULL,		/* xlib_format */
    xlib_finalize,
};
#endif

gvplugin_installed_t gvdevice_types_xlib[] = {
#ifdef CAIRO_HAS_XLIB_SURFACE
    {0, "xlib:cairo", 0, &device_engine_xlib, &device_features_xlib},
    {0, "x11:cairo", 0, &device_engine_xlib, &device_features_xlib},
#endif
    {0, NULL, 0, NULL, NULL}
};
