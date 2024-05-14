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

#include <gvc/gvplugin_device.h>
#include <gvc/gvio.h>

#ifdef HAVE_WEBP
#include "webp/encode.h"

static const char* const kErrorMessages[] = {
    "OK",
    "OUT_OF_MEMORY: Out of memory allocating objects",
    "BITSTREAM_OUT_OF_MEMORY: Out of memory re-allocating byte buffer",
    "NULL_PARAMETER: NULL parameter passed to function",
    "INVALID_CONFIGURATION: configuration is invalid",
    "BAD_DIMENSION: Bad picture dimension. Maximum width and height "
	"allowed is 16383 pixels.",
    "PARTITION0_OVERFLOW: Partition #0 is too big to fit 512k.\n"
        "To reduce the size of this partition, try using less segments "
        "with the -segments option, and eventually reduce the number of "
        "header bits using -partition_limit. More details are available "
        "in the manual (`man cwebp`)",
    "PARTITION_OVERFLOW: Partition is too big to fit 16M",
    "BAD_WRITE: Picture writer returned an I/O error",
    "FILE_TOO_BIG: File would be too big to fit in 4G",
    "USER_ABORT: encoding abort requested by user"
};

typedef enum {
    FORMAT_WEBP,
} format_type;

static int writer(const uint8_t* data, size_t data_size, const WebPPicture* const pic) {
    return gvwrite(pic->custom_ptr, (const char *)data, data_size) == data_size ? 1 : 0;
}

static void webp_format(GVJ_t * job)
{
    WebPPicture picture;
    WebPPreset preset;
    WebPConfig config;
    int stride;

    if (!WebPPictureInit(&picture) || !WebPConfigInit(&config)) {
	fprintf(stderr, "Error! Version mismatch!\n");
	goto Error;
    }

    // if either dimension exceeds the WebP API, map this to one of its errors
    if ((unsigned)INT_MAX / 4 < job->width || job->height > (unsigned)INT_MAX) {
	int error = VP8_ENC_ERROR_BAD_DIMENSION;
	fprintf(stderr, "Error! Cannot encode picture as WebP\n");
	fprintf(stderr, "Error code: %d (%s)\n", error, kErrorMessages[error]);
	goto Error;
    }

    picture.width = (int)job->width;
    picture.height = (int)job->height;
    stride = 4 * (int)job->width;

    picture.writer = writer;
    picture.custom_ptr = job;

    preset = WEBP_PRESET_DRAWING;

    if (!WebPConfigPreset(&config, preset, config.quality)) {
	fprintf(stderr, "Error! Could initialize configuration with preset.\n");
	goto Error;
    }

    if (!WebPValidateConfig(&config)) {
	fprintf(stderr, "Error! Invalid configuration.\n");
	goto Error;
    }

    if (!WebPPictureAlloc(&picture)) {
	fprintf(stderr, "Error! Cannot allocate memory\n");
	return;
    }

    if (!WebPPictureImportBGRA(&picture,
		(const uint8_t * const)job->imagedata, stride)) {
	fprintf(stderr, "Error! Cannot import picture\n");
	goto Error;
    }

    if (!WebPEncode(&config, &picture)) {
	fprintf(stderr, "Error! Cannot encode picture as WebP\n");
	fprintf(stderr, "Error code: %d (%s)\n",
	    picture.error_code, kErrorMessages[picture.error_code]);
	goto Error;
     }

Error:
     WebPPictureFree(&picture);
}

static gvdevice_engine_t webp_engine = {
    NULL,		/* webp_initialize */
    webp_format,
    NULL,		/* webp_finalize */
};

static gvdevice_features_t device_features_webp = {
	GVDEVICE_BINARY_FORMAT        
          | GVDEVICE_NO_WRITER
          | GVDEVICE_DOES_TRUECOLOR,/* flags */
	{0.,0.},                    /* default margin - points */
	{0.,0.},                    /* default page width, height - points */
	{96.,96.},                  /* 96 dpi */
};
#endif

gvplugin_installed_t gvdevice_webp_types[] = {
#ifdef HAVE_WEBP
    {FORMAT_WEBP, "webp:cairo", 1, &webp_engine, &device_features_webp},
#endif
    {0, NULL, 0, NULL, NULL}
};
