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

#include <memory>
#include <vector>

#include <windows.h>
#include <gdiplus.h>

typedef enum {
	FORMAT_NONE,
	FORMAT_METAFILE,
	FORMAT_BMP,
	FORMAT_EMF,
	FORMAT_EMFPLUS,
	FORMAT_GIF,
	FORMAT_JPEG,
	FORMAT_PNG,
	FORMAT_TIFF
} format_type;

/* RAII for GetDC/ReleaseDC */

struct DeviceContext
{
	HWND hwnd;
	HDC hdc;

	DeviceContext(HWND wnd = nullptr): hwnd(wnd), hdc(GetDC(wnd))
	{
	}

	~DeviceContext()
	{
		ReleaseDC(hwnd, hdc);
	}

};

/* textlayout etc. */

struct Layout
{
	std::unique_ptr<Gdiplus::Font> font;
	std::vector<WCHAR> text;

	Layout(char *fontname, double fontsize, char* string);
};

static const int BYTES_PER_PIXEL = 4;		/* bytes per pixel */

void gdiplus_free_layout(void *layout);

void UseGdiplus();
const Gdiplus::StringFormat* GetGenericTypographic();
void SaveBitmapToStream(Gdiplus::Bitmap &bitmap, IStream *stream, int format);
