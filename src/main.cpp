#include "aboutdialog.h"
#include "headless.h"
#include "mainwindow.h"

#include "cubiomes/util.h"

#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QTimer>
#include "world.h"

extern "C"
int getStructureConfig_override(int stype, int mc, StructureConfig *sconf)
{
    if unlikely(mc == INT_MAX) // to check if override is enabled in cubiomes
        mc = 0;
    int ok = getStructureConfig_default(stype, mc, sconf);
    if (ok && g_extgen.saltOverride)
    {
        uint64_t salt = g_extgen.salts[stype];
        if (salt <= MASK48)
            sconf->salt = salt;
    }
    return ok;
}

int main(int argc, char *argv[])
{
    // the library (xpple's fork) asks this provider instead of a compile time override
    setStructureConfigProvider(getStructureConfig_override);
    initBiomeColors(g_biomeColors);
    initBiomeTypeColors(g_tempsColors);

    QCoreApplication::setApplicationName(APP_STRING);

    bool version = false;
    bool nogui = false;
    bool clear = false;
    bool reset = false;
    bool usage = false;
    QString sessionpath;
    QString resultspath;
    QString grabpath, grabwinpath, grabsize, grablayer;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
            version = true;
        else if (strcmp(argv[i], "--nogui") == 0)
            nogui = true;
        else if (strcmp(argv[i], "--reset") == 0)
            clear = true;
        else if (strcmp(argv[i], "--reset-all") == 0)
            reset = true;
        else if (strncmp(argv[i], "--session=", 10) == 0)
            sessionpath = argv[i] + 10;
        else if (strncmp(argv[i], "--session", 9) == 0 && i+1 < argc)
            sessionpath = argv[++i];
        else if (strncmp(argv[i], "--out=", 6) == 0)
            resultspath = argv[i] + 6;
        else if (strncmp(argv[i], "--out", 5) == 0 && i+1 < argc)
            resultspath = argv[++i];
        else if (strncmp(argv[i], "--grab=", 7) == 0)
            grabpath = argv[i] + 7;
        else if (strncmp(argv[i], "--grab-window=", 14) == 0)
            grabwinpath = argv[i] + 14;
        else if (strncmp(argv[i], "--grab-size=", 12) == 0)
            grabsize = argv[i] + 12;
        else if (strncmp(argv[i], "--layer=", 8) == 0)
            grablayer = argv[i] + 8;
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
            usage = true;
    }

    if (usage)
    {
        const char *msg =
                "Usage: cubiomes-viewer [options]\n"
                "Options:\n"
                "      --help                 Display this help and exit.\n"
                "      --version              Output version information and exit.\n"
                "      --nogui                Run in headless search mode.\n"
                "      --reset                Discard results and reset starting seed.\n"
                "      --reset-all            Clear settings and remove all session data.\n"
                "      --session=file         Open this session file.\n"
                "      --out=file             Write matching seeds to this file while searching.\n"
                "      --grab=file.png        Render the map of the restored settings to an image and exit\n"
                "                             (works with QT_QPA_PLATFORM=offscreen).\n"
                "      --grab-window=file.png Same for the whole window.\n"
                "      --grab-size=WxH        Window size for --grab (default 1600x1000).\n"
                "      --layer=mode,disp      Map layer for --grab, e.g. 0,0 biomes; 12,5 height layer with\n"
                "                             hypsometric tints and mountain outline.\n"
                "\n";
        printf("%s", msg);
        exit(0);
    }
    if (version)
    {
        printf("%s %s\n", APP_STRING, getVersStr().toLocal8Bit().data());
        exit(0);
    }

    if (reset)
    {
        QSettings settings(APP_STRING, APP_STRING);
        settings.clear();

        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir dir(path);
        if (dir.exists() && path.contains(APP_STRING))
        {
            dir.removeRecursively();
        }
    }

    if (sessionpath.isEmpty())
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir dir(path);
        if (!dir.exists())
            dir.mkpath(".");
        sessionpath = path + "/session.save";
    }

    if (nogui)
    {
        QCoreApplication app(argc, argv);
        Headless headless(sessionpath, resultspath, clear, &app);

        QObject::connect(&headless, SIGNAL(finished()), &app, SLOT(quit()));
        QTimer::singleShot(0, &headless, SLOT(run()));

        return app.exec();
    }
    else
    {
        QGuiApplication::setDesktopFileName("com.github.cubitect.cubiomes-viewer");
        QApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, false);

        QApplication app(argc, argv);

        MainWindow mw(sessionpath, resultspath);
        mw.show();

        if (!grabpath.isEmpty() || !grabwinpath.isEmpty())
        {   // render the current map to an image file once all tiles are done, then quit
            int w = 1600, h = 1000;
            if (sscanf(grabsize.toLocal8Bit().data(), "%dx%d", &w, &h) != 2 || w < 100 || h < 100)
                w = 1600, h = 1000;
            mw.showNormal();
            mw.resize(w, h);
            int mode = -1, disp = -1;
            if (sscanf(grablayer.toLocal8Bit().data(), "%d,%d", &mode, &disp) >= 1 && mode >= 0 && mode < LOPT_MAX)
                QMetaObject::invokeMethod(&mw, "onActionBiomeLayerSelect", Q_ARG(int, mode), Q_ARG(int, disp));
            QTimer *timer = new QTimer(&mw);
            QElapsedTimer *clock = new QElapsedTimer();
            clock->start();
            int *idle = new int(0);
            QObject::connect(timer, &QTimer::timeout, [&mw, &app, timer, clock, idle, grabpath, grabwinpath]() {
                MapView *mv = mw.getMapView();
                mv->update();
                *idle = (mv->world && !mv->world->isBusy() && clock->elapsed() > 1500) ? *idle + 1 : 0;
                if (*idle < 6 && clock->elapsed() < 600000)
                    return;
                timer->stop();
                bool ok = true;
                if (!grabpath.isEmpty())
                    ok &= mv->grab().save(grabpath);
                if (!grabwinpath.isEmpty())
                    ok &= mw.grab().save(grabwinpath);
                WorldInfo wi;
                mw.getSeed(&wi, false);
                printf("grab: %s seed=%" PRId64 " mc=%s tectonic=%d x=%.0f z=%.0f scale=%g map=%dx%d after %.1fs\n",
                    ok ? "saved" : "FAILED", (int64_t)wi.seed, mc2str(wi.mc), wi.tectonic,
                    mv->getX(), mv->getZ(), mv->getScale(), mv->width(), mv->height(), clock->elapsed() / 1000.0);
                fflush(stdout);
                mw.close(); // saves settings and session like a normal exit
                app.exit(ok ? 0 : 1);
            });
            timer->start(150);
        }
        return app.exec();
    }
}
