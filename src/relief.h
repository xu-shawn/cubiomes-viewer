#ifndef RELIEF_H
#define RELIEF_H

#include "config.h"

#include <atomic>

// Relief view: the biome map with illuminated contour lines and a soft hillshade.
//
// A tile samples the biome and the surface height of every pixel once (reliefSampleTile) and keeps
// both. The image is rendered from that data (reliefRender) at 1, 2 or 4 image pixels per sample,
// so that lines stay about one screen pixel wide at every zoom without sampling the terrain again.

enum { RELIEF_BORDER = 2 }; // height samples around a tile, needed for slopes and interpolation

// Contour interval in blocks for a given image resolution (power of two, 2..64).
int reliefAutoInterval(float blocksPerPixel);

// Image pixels per height sample (1, 2 or 4) for the given number of screen pixels per sample.
int reliefImageScale(qreal pixPerSample);

// Zoomed out, the contour lines give way to shaded relief. The transition runs from about 3 to 7
// blocks per screen pixel in quarter-octave steps of the zoom; an image is rendered for one step and has
// to be rendered again when the step changes (the steps are constant outside the transition).
int reliefZoomStep(const ReliefOpt& opt, qreal blocksPerPixel);
// Share of the shaded relief for a step: 0 = contour lines only, 1 = shaded relief only.
float reliefFarBlend(int step);

// Largest number of screen pixels a height sample may cover at the given zoom (the level of detail
// switches to a four times finer grid above it): 4 for fast, 2 for fine. Balanced is fast while
// contour lines are drawn (bilinear heights carry lines well) and fine from 8 blocks per pixel on,
// where the shaded relief needs the detail; both rules pick the same grid at exactly 8.
qreal reliefQuality(const ReliefOpt& opt, qreal blocksPerPixel);

// Fills biomes[r.sx*r.sz] and heights[(r.sx+2B)*(r.sz+2B)] (B = RELIEF_BORDER) for an Overworld
// map tile. Returns false if the generator has no surface height here or on abort.
bool reliefSampleTile(const Generator *g, const SurfaceNoise *sn, Range r,
        int *biomes, float *heights, const std::atomic_bool *abort);

// Renders the (w*k) x (h*k) RGB888 image of a tile for a zoom step.
void reliefRender(unsigned char *rgb, const int *biomes, const float *heights,
        int w, int h, int k, int scale, int step, const ReliefOpt& opt, int dim,
        const unsigned char colors[256][3]);

// tile statistics (for --grab): generation time, tiles, sampled pixels; relief image renders
extern std::atomic<qint64> g_tile_ns, g_tile_count, g_tile_samples, g_relief_ns, g_relief_count;

#endif // RELIEF_H
