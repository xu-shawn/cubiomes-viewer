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

// Fills biomes[r.sx*r.sz] and heights[(r.sx+2B)*(r.sz+2B)] (B = RELIEF_BORDER) for an Overworld
// map tile. Returns false if the generator has no surface height here or on abort.
bool reliefSampleTile(const Generator *g, const SurfaceNoise *sn, Range r,
        int *biomes, float *heights, const std::atomic_bool *abort);

// Renders the (w*k) x (h*k) RGB888 image of a tile.
void reliefRender(unsigned char *rgb, const int *biomes, const float *heights,
        int w, int h, int k, int scale, const ReliefOpt& opt, int dim,
        const unsigned char colors[256][3]);

// tile statistics (for --grab): generation time, tiles, sampled pixels; relief image renders
extern std::atomic<qint64> g_tile_ns, g_tile_count, g_tile_samples, g_relief_ns, g_relief_count;

#endif // RELIEF_H
