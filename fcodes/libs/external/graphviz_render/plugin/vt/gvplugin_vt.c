/// \file
/// \brief Device that renders using ANSI terminal colors

#include <assert.h>
#include <gvc/gvplugin.h>
#include <gvc/gvplugin_device.h>
#include <limits.h>
#include <stddef.h>

#include <gvc/gvio.h>

/// an ANSI color
typedef struct {
  unsigned value;
  unsigned red;
  unsigned green;
  unsigned blue;

} color_t;

/// ANSI 3-bit colors
static const color_t COLORS[] = {
    {0, 0x00, 0x00, 0x00}, ///< black
    {1, 0xff, 0x00, 0x00}, ///< red
    {2, 0x00, 0xff, 0x00}, ///< green
    {3, 0xff, 0xff, 0x00}, ///< yellow
    {4, 0x00, 0x00, 0xff}, ///< blue
    {5, 0xff, 0x00, 0xff}, ///< magenta
    {6, 0x00, 0xff, 0xff}, ///< cyan
    {7, 0xff, 0xff, 0xff}, ///< white
};

/// a metric of “closeness” to a given color
static unsigned distance(const color_t base, unsigned red, unsigned green,
                         unsigned blue) {
  unsigned diff = 0;
  diff += red > base.red ? red - base.red : base.red - red;
  diff += green > base.green ? green - base.green : base.green - green;
  diff += blue > base.blue ? blue - base.blue : base.blue - blue;
  return diff;
}

/// find closest ANSI color
static unsigned get_color(unsigned red, unsigned green, unsigned blue) {
  unsigned winner = 0;
  unsigned diff = UINT_MAX;
  for (size_t i = 0; i < sizeof(COLORS) / sizeof(COLORS[0]); ++i) {
    unsigned d = distance(COLORS[i], red, green, blue);
    if (d < diff) {
      diff = d;
      winner = COLORS[i].value;
    }
  }
  return winner;
}

static void process(GVJ_t *job, int color_depth) {

  unsigned char *data = (unsigned char *)job->imagedata;

  assert(color_depth == 3 || color_depth == 24);

  for (unsigned y = 0; y < job->height; y += 2) {
    for (unsigned x = 0; x < job->width; ++x) {

      // number of bytes per pixel
      const unsigned BPP = 4;

      {
        // extract the upper pixel
        unsigned offset = y * job->width * BPP + x * BPP;
        unsigned red = data[offset + 2];
        unsigned green = data[offset + 1];
        unsigned blue = data[offset];

        // use this to select a foreground color
        if (color_depth == 3) {
          unsigned fg = get_color(red, green, blue);
          gvprintf(job, "\033[3%um", fg);
        } else {
          assert(color_depth == 24);
          gvprintf(job, "\033[38;2;%u;%u;%um", red, green, blue);
        }
      }

      {
        // extract the lower pixel
        unsigned red = 0;
        unsigned green = 0;
        unsigned blue = 0;
        if (y + 1 < job->height) {
          unsigned offset = (y + 1) * job->width * BPP + x * BPP;
          red = data[offset + 2];
          green = data[offset + 1];
          blue = data[offset];
        }

        // use this to select a background color
        if (color_depth == 3) {
          unsigned bg = get_color(red, green, blue);
          gvprintf(job, "\033[4%um", bg);
        } else {
          assert(color_depth == 24);
          gvprintf(job, "\033[48;2;%u;%u;%um", red, green, blue);
        }
      }

      // print unicode “upper half block” to effectively do two rows of
      // pixels per one terminal row
      gvprintf(job, "▀\033[0m");
    }
    gvprintf(job, "\n");
  }
}

static void process3(GVJ_t *job) { process(job, 3); }

static void process24(GVJ_t *job) { process(job, 24); }

static gvdevice_engine_t engine3 = {
    .format = process3,
};

static gvdevice_engine_t engine24 = {
    .format = process24,
};

static gvdevice_features_t device_features = {
    .default_dpi = {96, 96},
};

static gvplugin_installed_t device_types[] = {
    {8, "vt:cairo", 0, &engine3, &device_features},
    {1 << 24, "vt-24bit:cairo", 0, &engine24, &device_features},
    {0},
};

static gvplugin_api_t apis[] = {
    {API_device, device_types},
    {(api_t)0, 0},
};

gvplugin_library_t gvplugin_vt_LTX_library = {"vt", apis};
