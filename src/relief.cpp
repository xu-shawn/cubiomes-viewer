#include "relief.h"
#include "world.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

std::atomic<qint64> g_tile_ns(0), g_tile_count(0), g_tile_samples(0), g_relief_ns(0), g_relief_count(0);

int reliefAutoInterval(float blocksPerPixel)
{
    // Two to four image pixels of horizontal distance per contour on a 1:1 slope,
    // i.e. lines every 4-8 pixels on ordinary hillsides at every zoom.
    int iv = 2;
    while (iv < 64 && iv < 2 * blocksPerPixel)
        iv *= 2;
    return iv;
}

int reliefImageScale(qreal pixPerSample)
{
    // image pixels per height sample, so that an image pixel covers 0.5-1 screen pixels
    if (pixPerSample <= 1.0)
        return 1;
    if (pixPerSample <= 2.0)
        return 2;
    return 4;
}

// 1.18+ noise biomes: the climate sample that selects the biome also carries the depth, from which
// mapApproxHeight() derives the surface. One sample per pixel yields both.
// The biome lookup takes a search hint (the previous result), which speeds it up without making
// the result depend on the order of the lookups, so neighbouring tiles always agree.
static inline int sampleNoiseBiomeHeight(const BiomeNoise *bn, float *height, int x4, int y4, int z4, uint64_t *hint)
{
    int64_t np[6];
    sampleBiomeNoise(bn, np, x4, y4, z4, NULL, SAMPLE_NO_BIOME);
    // mapApproxHeight() uses the depth at y = 0; depth falls by 1/32 per 1:4 cell of height
    *height = (np[NP_DEPTH] + 312.5f * y4) / 76.0f;
    return hint ? climateToBiomeHint(bn->mc, (const uint64_t*) np, hint) : -1;
}

// Beyond 1:4 the plain biome map takes two shortcuts (no local distortion of the sampling position,
// and the biome search continues from the previous result); the relief map takes the same ones,
// so both show the same biomes at the same cost.
static inline int sampleNoiseBiomeHeightFar(const BiomeNoise *bn, float *height, int x4, int y4, int z4, uint64_t *dat)
{
    int64_t np[6];
    int id = sampleBiomeNoise(bn, np, x4, y4, z4, dat, dat ? SAMPLE_NO_SHIFT : SAMPLE_NO_SHIFT | SAMPLE_NO_BIOME);
    *height = (np[NP_DEPTH] + 312.5f * y4) / 76.0f;
    return id;
}

bool reliefSampleTile(const Generator *g, const SurfaceNoise *sn, Range r,
        int *biomes, float *heights, const std::atomic_bool *abort)
{
    const int B = RELIEF_BORDER;
    const int w = r.sx, h = r.sz, sw = w + 2*B, sh = h + 2*B;
    const int S = r.scale;
    const bool tectonic = g->dim == DIM_OVERWORLD && (g->flags & TECTONIC_ANY);
    const bool noise = g->dim == DIM_OVERWORLD && g->mc >= MC_1_18 && g->bn.nptype < 0;

    if ((tectonic || noise) && S >= 4)
    {   // biome and height of every pixel from a single climate evaluation
        const int f = S / 4, mid = f / 2;
        for (int j = -B; j < h + B; j++)
        {
            if (abort && *abort) return false;
            uint64_t hint = 0; // search hint for the biome lookup: the previous result along the row
            int z4 = (r.z + j) * f + mid;
            for (int i = -B; i < w + B; i++)
            {
                int x4 = (r.x + i) * f + mid;
                float *hp = &heights[(j + B) * sw + (i + B)];
                bool inside = (i >= 0 && i < w && j >= 0 && j < h);
                if (tectonic)
                {
                    if (inside)
                        biomes[j*w + i] = sampleTectonicBiomeHeight(&g->tn, NULL, hp, x4, r.y, z4, &hint);
                    else
                        *hp = getTectonicHeight(&g->tn, x4 * 4, z4 * 4);
                }
                else if (S > 4)
                {
                    int id = sampleNoiseBiomeHeightFar(&g->bn, hp, x4, r.y, z4, inside ? &hint : NULL);
                    if (inside)
                        biomes[j*w + i] = id;
                }
                else
                {
                    int id = sampleNoiseBiomeHeight(&g->bn, hp, x4, r.y, z4, inside ? &hint : NULL);
                    if (inside)
                        biomes[j*w + i] = id;
                }
            }
        }
        return true;
    }

    // otherwise the biomes come from the regular generator ...
    if (genBiomes(g, biomes, r))
        return false;
    if (abort && *abort) return false;

    // ... and the heights are sampled separately
    if (tectonic)
    {   // S == 1: the base surface is exact at every block
        for (int j = 0; j < sh; j++)
        {
            if (abort && *abort) return false;
            for (int i = 0; i < sw; i++)
                heights[j*sw + i] = getTectonicHeight(&g->tn, r.x + i - B, r.z + j - B);
        }
        return true;
    }
    if (noise)
    {   // S == 1: approximate heights exist per 1:4 cell (at block 4*cell), interpolate between them
        int cx0 = (r.x - B) >> 2, cz0 = (r.z - B) >> 2;
        int cw = ((r.x + w + B - 1) >> 2) - cx0 + 2, ch = ((r.z + h + B - 1) >> 2) - cz0 + 2;
        std::vector<float> buf((size_t)cw * ch);
        for (int j = 0; j < ch; j++)
        {
            if (abort && *abort) return false;
            for (int i = 0; i < cw; i++)
                sampleNoiseBiomeHeight(&g->bn, &buf[j*cw + i], cx0 + i, 0, cz0 + j, NULL);
        }
        for (int j = 0; j < sh; j++)
        {
            int bz = r.z + j - B, cj = (bz >> 2) - cz0;
            float dj = (bz & 3) * 0.25f;
            for (int i = 0; i < sw; i++)
            {
                int bx = r.x + i - B, ci = (bx >> 2) - cx0;
                float di = (bx & 3) * 0.25f;
                const float *p = &buf[cj*cw + ci];
                float v0 = p[0] + di * (p[1] - p[0]);
                float v1 = p[cw] + di * (p[cw+1] - p[cw]);
                heights[j*sw + i] = v0 + dj * (v1 - v0);
            }
        }
        return true;
    }

    // older versions: the height estimate of the height layer
    std::vector<float> hv;
    if (!sampleSurfaceHeights(hv, B, r, g, sn, S > 16 ? 1 : 0, false, abort))
        return false;
    memcpy(heights, hv.data(), sizeof(float) * sw * sh);
    return true;
}

static inline int floordiv5(int a)
{
    return a >= 0 ? a / 5 : -((4 - a) / 5);
}

void reliefRender(unsigned char *rgb, const int *biomes, const float *heights,
        int w, int h, int k, int scale, const ReliefOpt& opt, int dim,
        const unsigned char colors[256][3])
{
    const int B = RELIEF_BORDER, b = 2;
    const int W = w * k, H = h * k;
    const int sw = w + 2*B;
    const int iw = W + 2*b, ih = H + 2*b;

    // heights at image resolution, with a border of two image pixels
    std::vector<float> hi((size_t)iw * ih);
    if (k == 1)
    {
        memcpy(hi.data(), heights, sizeof(float) * iw * ih); // b == B
    }
    else
    {   // bilinear between the samples; image pixel centres lie at (I + 0.5) / k - 0.5 samples
        std::vector<int> ci(std::max(iw, ih));
        std::vector<float> cf(std::max(iw, ih));
        for (int I = 0; I < std::max(iw, ih); I++)
        {
            float u = (I - b + 0.5f) / k - 0.5f;
            int c = (int) floorf(u);
            ci[I] = c + B;
            cf[I] = u - c;
        }
        for (int J = 0; J < ih; J++)
        {
            const float *r0 = heights + (size_t)ci[J] * sw;
            const float *r1 = r0 + sw;
            float fj = cf[J];
            float *o = &hi[(size_t)J * iw];
            for (int I = 0; I < iw; I++)
            {
                int c = ci[I];
                float fi = cf[I];
                float v0 = r0[c] + fi * (r0[c+1] - r0[c]);
                float v1 = r1[c] + fi * (r1[c+1] - r1[c]);
                o[I] = v0 + fj * (v1 - v0);
            }
        }
    }

    const float bpp = (float) scale / k; // blocks per image pixel
    const int iv = opt.interval > 0 ? opt.interval : reliefAutoInterval(bpp);
    const bool hassea = (dim == DIM_OVERWORLD);
    const float base = hassea ? 63.f : 0.f; // contour levels are counted from sea level
    const float inv = 1.0f / iv;
    const float str = opt.strength / 100.0f;
    // slopes are in blocks per block; beyond 16 blocks per pixel neighbouring samples are far apart
    // and the relief is exaggerated instead of fading out
    const float islope = 1.0f / (bpp < 16 ? bpp : 16);

    std::vector<int> band((size_t)iw * ih);
    for (size_t i = 0, n = band.size(); i < n; i++)
        band[i] = (int) floorf((hi[i] - base) * inv);

    for (int J = 0; J < H; J++)
    {
        const float *p = &hi[(size_t)(J + b) * iw + b];
        const int *q = &band[(size_t)(J + b) * iw + b];
        const int *brow = biomes + (size_t)(J / k) * w;
        unsigned char *out = rgb + (size_t)3 * J * W;

        for (int I = 0; I < W; I++, p++, q++, out += 3)
        {
            int id = brow[I / k];
            float c[3];
            if (id >= 0 && id < 256)
            {
                c[0] = colors[id][0]; c[1] = colors[id][1]; c[2] = colors[id][2];
            }
            else
            {
                c[0] = c[1] = c[2] = 0;
            }

            const float h0 = p[0];
            const float gx = (p[1] - p[-1]) * 0.5f;   // height change per image pixel, east
            const float gz = (p[iw] - p[-iw]) * 0.5f; // and south
            const float sp = sqrtf(gx*gx + gz*gz);
            // +1 where the slope faces the light in the north-west, -1 where it faces away
            const float aspect = sp > 1e-6f ? (gx + gz) / (1.41421356f * sp) : 0.f;
            const bool water = hassea && h0 < base;

            const int b0 = q[0];
            int bmin = std::min(std::min(q[-1], q[1]), std::min(q[-iw], q[iw]));
            const int bmin2 = std::min(bmin, std::min(std::min(q[-2], q[2]), std::min(q[-2*iw], q[2*iw])));
            // a line is drawn on the upper side of every level that is crossed between neighbours
            const bool line = b0 > bmin;
            const bool index = floordiv5(b0) > floordiv5(bmin2); // every fifth level, two pixels wide
            // sea level; far out every pond would get an outline, so it is left to the biome colours there
            const bool coast = hassea && bpp <= 8 && b0 >= 0 && bmin < 0;
            // contour levels crossed per pixel: above 0.4 the lines would merge
            const float dens = sp * inv;

            if (water)
            {   // depth contours, no light
                float f = 1.0f;
                if (dens > 0.4f)
                    f -= 0.30f * str * (dens < 1.0f ? dens : 1.0f);
                else if (index && dens <= 2.0f)
                    f -= 0.55f * str;
                else if (line)
                    f -= 0.30f * str;
                c[0] *= f; c[1] *= f; c[2] *= f;
            }
            else
            {
                if (opt.shading)
                {   // soft hillshade under the lines, from the slope over five pixels so that
                    // small bumps do not turn into texture
                    float sx = (p[2] - p[-2]) * 0.25f * islope, sz = (p[2*iw] - p[-2*iw]) * 0.25f * islope;
                    float l = 1.0f + 0.40f * (sx + sz) / sqrtf(1.0f + 2.0f * (sx*sx + sz*sz));
                    l = l < 0.78f ? 0.78f : (l > 1.18f ? 1.18f : l);
                    c[0] *= l; c[1] *= l; c[2] *= l;
                }
                // illuminated contours: light on slopes facing the light, dark on slopes facing away
                float shift = aspect < 0 ? -0.20f + 0.60f * aspect : -0.20f + 0.95f * aspect;
                float amount = 0;
                if (dens > 0.4f)
                    amount = 0.9f * (dens < 1.0f ? dens : 1.0f); // too dense for lines: shade the slope instead
                else if (line)
                    amount = 1.0f;
                amount *= shift * str;
                if (amount < 0)
                {
                    float f = 1.0f + amount;
                    c[0] *= f; c[1] *= f; c[2] *= f;
                }
                else if (amount > 0)
                {
                    if (c[0] + c[1] + c[2] > 600.f)
                    {   // snow and other bright ground cannot get lighter: a faint cool line instead
                        amount *= 0.6f;
                        c[0] += (160.f - c[0]) * amount;
                        c[1] += (178.f - c[1]) * amount;
                        c[2] += (212.f - c[2]) * amount;
                    }
                    else
                    {
                        c[0] += (255.f - c[0]) * amount;
                        c[1] += (255.f - c[1]) * amount;
                        c[2] += (255.f - c[2]) * amount;
                    }
                }
                if (index && dens <= 2.0f && !coast)
                {   // index contours stay dark on both sides, a little lighter towards the light
                    float f = 1.0f - str * (0.85f - 0.30f * (aspect > 0 ? aspect : 0));
                    f = f < 0.08f ? 0.08f : f;
                    c[0] *= f; c[1] *= f; c[2] *= f;
                }
                if (coast)
                {   // sea level
                    float t = 0.55f + 0.45f * str;
                    c[0] += (12.f - c[0]) * t;
                    c[1] += (32.f - c[1]) * t;
                    c[2] += (72.f - c[2]) * t;
                }
            }
            out[0] = (unsigned char)(c[0] <= 0 ? 0 : (c[0] >= 255.f ? 255 : c[0]));
            out[1] = (unsigned char)(c[1] <= 0 ? 0 : (c[1] >= 255.f ? 255 : c[1]));
            out[2] = (unsigned char)(c[2] <= 0 ? 0 : (c[2] >= 255.f ? 255 : c[2]));
        }
    }
}
