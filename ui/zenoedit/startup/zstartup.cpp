#include <zeno/extra/EventCallbacks.h>
#include <zeno/extra/assetDir.h>
#include <zeno/core/Session.h>
#include <zeno/utils/log.h>
#include <zeno/types/UserData.h>
#include <zeno/types/GenericObject.h>
#include "zstartup.h"
#include <QApplication>
#include <QSettings>
#include <algorithm>
#include "settings/zsettings.h"

void startUp()
{
    zeno::setExecutableDir(QCoreApplication::applicationDirPath().toStdString());
    zeno::setConfigVariable("EXECFILE", QCoreApplication::applicationFilePath().toStdString());

    QSettings settings(zsCompanyName, zsEditor);
    QVariant nas_loc_v = settings.value("nas_loc");
    if (!nas_loc_v.isNull()) {
        zeno::setConfigVariable("NASLOC", nas_loc_v.toString().toStdString());
    }

#if 0
    QVariant scalefac_v = settings.value("scale_factor");
    if (!scalefac_v.isNull()) {
        float scalefac = scalefac_v.toFloat();
        if (scalefac >= 1.0f)
            qputenv("QT_SCALE_FACTOR", QString::number(scalefac).toUtf8());
    }
#endif

    static int calledOnce = ([]{
        zeno::getSession().eventCallbacks->triggerEvent("init");
    }(), 0);
}

std::string getZenoVersion() {
    const char *date = __DATE__;
    const char *table[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    int month = std::find(table, table + 12, std::string(date, 3)) - table + 1;
    int day = std::stoi(std::string(date + 4, 2));
    int year = std::stoi(std::string(date + 7, 4));
    return zeno::format("{:04d}.{:02d}.{:02d}", year, month, day);
}

void verifyVersion()
{
    auto ver = getZenoVersion();
    const char *plat =
#if defined(Q_OS_WIN)
                   "windows"
#elif defined(Q_OS_LINUX)
                   "linux"
#else
                   "unknown"
#endif
#if defined(NDEBUG)
                   " release"
#else
                   " debug"
#endif
                   ;
    const char *feat =
#if defined(__INTEL_COMPILER)
                   "icc"
#elif defined(__clang__)
                   "clang"
#elif defined(__GNUC__)
                   "gcc"
#elif defined(_MSC_VER)
                   "msvc"
#else
                   "unknown"
#endif
#if defined(ZENO_MULTIPROCESS)
#if defined(ZENO_IPC_USE_TCP)
                   "+tcp"
#else
                   "+pipe"
#endif
#endif
#if defined(_OPENMP)
                   "+omp"
#endif
#if defined(ZENO_PARALLEL_STL)
                   "+pstl"
#endif
#if defined(ZENO_ENABLE_BACKWARD)
                   "+bt"
#endif
#if defined(ZENO_BENCHMARKING)
                   "+tm"
#endif
                   ;
    // TODO: luzh, may check the internet latest version and compare, if not latest hint the user to update..
    zeno::log_info("zeno {} {} {} {}", plat, ver, __TIME__, feat);
}

int invoke_main(int argc, char *argv[]);
int invoke_main(int argc, char *argv[]) {
    if (argc < 1) {
        zeno::log_error("no enough arguments to -invoke");
        return -1;
    }
    std::string prog = argv[0];
    std::vector<char *> newArgv(argv, argv + argc);
    std::string newProg = QCoreApplication::applicationFilePath().toStdString() + "\0";
    newArgv[0] = newProg.data();
    auto &ud = zeno::getSession().userData();
    if (!ud.has("subprogram_" + prog)) {
        zeno::log_error("no such sub-program named [{}]", prog);
        return -1;
    }
    zeno::log_info("launching sub-program [{}]", prog);
    auto alterMain = ud.get<zeno::GenericObject<int(*)(int, char **)>>("subprogram_" + prog)->get();
    alterMain(newArgv.size(), newArgv.data());
    return 0;
}
