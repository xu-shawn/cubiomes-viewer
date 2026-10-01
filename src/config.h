#ifndef CONFIG_H
#define CONFIG_H

#include "cubiomes/finders.h"
#include "cubiomes/features/end_city.h"
#include "cubiomes/features/fortress.h"
#include "cubiomes/tectonic.h"

// structure configuration with the user's salt overrides applied (main.cpp);
// it is registered with the library through setStructureConfigProvider()
extern "C" int getStructureConfig_override(int stype, int mc, StructureConfig *sconf);

#include <QSettings>
#include <QString>
#include <QFont>
#include <QTextStream>

#include <vector>


// This build keeps its settings and sessions apart from a regular Cubiomes Viewer installation
// (~/.config/cubiomes-viewer-tectonic instead of ~/.config/cubiomes-viewer).
#define APP_STRING "cubiomes-viewer-tectonic"

#define PRECOMPUTE48_BUFSIZ ((int64_t)1 << 30)

enum { MC_DEFAULT = MC_1_21_3 };

struct ExtGenConfig
{
    bool experimentalVers;
    bool estimateTerrain;
    bool saltOverride;
    uint64_t salts[FEATURE_NUM];

    ExtGenConfig() { reset(); }

    void reset();
    void load(QSettings& settings);
    void save(QSettings& settings);
    void load() { QSettings s(APP_STRING, APP_STRING); load(s); }
    void save() { QSettings s(APP_STRING, APP_STRING); save(s); }
};

extern unsigned char g_biomeColors[256][3];
extern unsigned char g_tempsColors[256][3];

extern qreal g_fontscale;
extern qreal g_iconscale;

// Keep the extended generator settings in global scope.
extern ExtGenConfig g_extgen;


// Overworld terrain of the Tectonic mod instead of vanilla (cubiomes/tectonic.h); needs MC 26.3
enum { TECT_OFF = 0, TECT_MOD = 1, TECT_DATAPACK = 2 };

struct WorldInfo
{
    int mc;
    bool large;
    int tectonic;
    uint64_t seed;
    int y;

    // flags for setupGenerator()
    uint32_t genFlags() const
    {
        uint32_t flags = large ? LARGE_BIOMES : 0;
        if (tectonic == TECT_MOD && mc == MC_26_3) flags |= TECTONIC_MOD;
        if (tectonic == TECT_DATAPACK && mc == MC_26_3) flags |= TECTONIC_DATAPACK;
        return flags;
    }
    bool isTectonic() const { return genFlags() & TECTONIC_ANY; }

    WorldInfo() { reset(); }

    bool equals(const WorldInfo& wi) const;

    void reset();
    void load(QSettings& settings);
    void save(QSettings& settings);
    void load() { QSettings s(APP_STRING, APP_STRING); load(s); }
    void save() { QSettings s(APP_STRING, APP_STRING); save(s); }
    bool read(const QString& line);
    void write(QTextStream& stream);
};


enum {
    LOPT_BIOMES,
    LOPT_NOISE_PARA,
    LOPT_NOISE_T_4 = LOPT_NOISE_PARA,
    LOPT_NOISE_H_4,
    LOPT_NOISE_C_4,
    LOPT_NOISE_E_4,
    LOPT_NOISE_D_4,
    LOPT_NOISE_W_4,
    LOPT_RIVER_4,
    LOPT_OCEAN_256,
    LOPT_NOOCEAN_1,
    LOPT_BETA_T_1,
    LOPT_BETA_H_1,
    LOPT_HEIGHT,
    LOPT_STRUCTS,
    LOPT_MAX,
};

struct LayerOpt
{
    int8_t mode;
    int8_t disp[LOPT_MAX];

    LayerOpt() { reset(); }

    void reset();

    int activeDisp() const;
    bool activeDifference(const LayerOpt& l) const;
    bool isClimate(int mc) const;
};

enum {
    D_NONE = -1,
    // generics
    D_GRID,
    D_SLIME,
    // structures
    D_DESERT,
    D_JUNGLE,
    D_IGLOO,
    D_HUT,
    D_VILLAGE,
    D_MANSION,
    D_MONUMENT,
    D_OCEANRUIN,
    D_SHIPWRECK,
    D_TREASURE,
    D_MINESHAFT,
    D_WELL,
    D_GEODE,
    D_OUTPOST,
    D_ANCIENTCITY,
    D_TRAILRUINS,
    D_CHAMBERS,
    D_ABANDONEDCAMP,
    D_PORTAL,
    D_PORTALN,
    D_FORTESS,
    D_BASTION,
    D_ENDCITY,
    D_GATEWAY,
    // non-recurring structures
    D_SPAWN,
    D_STRONGHOLD,
    D_STRUCT_NUM
};

QString mapopt2display(int opt);
const char *mapopt2str(int opt);
int str2mapopt(const char *s);
int mapopt2stype(int opt);

struct MapConfig
{
    struct Opt
    {
        double scale;
        bool enabled;
        Opt() : scale(),enabled() {}
    };
    Opt opts[D_STRUCT_NUM];
    bool zoomEnabled;

    MapConfig(bool init = true);

    double scale(int opt) const { return opts[opt].scale; }
    bool enabled(int opt) const { return opts[opt].enabled; }
    bool hasScale(int opt) const;
    int getDim(int opt) const;
    int getTileSize(int opt) const;

    bool sameMapOpts(const MapConfig& a) const;

    void reset();
    void load(QSettings& settings);
    void save(QSettings& settings);
    void load() { QSettings s(APP_STRING, APP_STRING); load(s); }
    void save() { QSettings s(APP_STRING, APP_STRING); save(s); }
};


enum { STYLE_SYSTEM, STYLE_DARK };

struct Config
{
    bool smoothMotion;
    bool showBBoxes;
    bool restoreSession;
    bool restoreWindow;
    bool checkForUpdates;
    int autosaveCycle;
    int uistyle;
    int maxMatching;
    int gridSpacing;
    int gridMultiplier;
    int mapCacheSize;
    int mapThreads;
    QString lang;
    QString biomeColorPath;
    QString separator;
    QString quote;
    QFont fontNorm;
    QFont fontMono;
    qreal iconScale;

    Config() { reset(); }

    void reset();
    void load(QSettings& settings);
    void save(QSettings& settings);
    void load() { QSettings s(APP_STRING, APP_STRING); load(s); }
    void save() { QSettings s(APP_STRING, APP_STRING); save(s); }
};

enum { GEN48_AUTO, GEN48_QH, GEN48_QM, GEN48_LIST, GEN48_NONE };
enum { IDEAL, CLASSIC, NORMAL, BARELY, IDEAL_SALTED };

struct Gen48Config
{
    int mode;
    QString slist48path;
    uint64_t salt;
    uint64_t listsalt;
    int qual;
    int qmarea;
    bool manualarea;
    int x1, z1, x2, z2;

    Gen48Config() { reset(); }

    void reset();
    bool read(const QString& line);
    void write(QTextStream& stream);

    uint64_t estimateSeedCnt(uint64_t slist48len);
};

// search type options from combobox
enum { SEARCH_INC = 0, SEARCH_BLOCKS = 1, SEARCH_LIST = 2, SEARCH_48ONLY = 3 };

struct SearchConfig
{
    int searchtype;
    QString slist64path;
    int threads;
    uint64_t startseed;
    bool stoponres;
    uint64_t smin;
    uint64_t smax;

    SearchConfig() { reset(); }

    void reset();
    bool read(const QString& line);
    void write(QTextStream& stream);
};


Q_DECLARE_METATYPE(int64_t)
Q_DECLARE_METATYPE(uint64_t)
Q_DECLARE_METATYPE(Pos)
Q_DECLARE_METATYPE(Config)


#endif // CONFIG_H
