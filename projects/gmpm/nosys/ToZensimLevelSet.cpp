#include "../ZensimGeometry.h"
// #include "zensim/geometry/GeometrySampler.h"
#include "zensim/geometry/VdbLevelSet.h"
// #include "zensim/geometry/VdbSampler.h"
// #include "zensim/io/ParticleIO.hpp"
#include "zensim/tpls/fmt/color.h"
#include "zensim/tpls/fmt/format.h"
#include <zeno/VDBGrid.h>

namespace zen {

struct ToZensimLevelSet : zen::INode {
  void apply() override {
    fmt::print(fg(fmt::color::green), "begin executing ToZensimLevelSet\n");
    auto ls = zen::IObject::make<ZenoSparseLevelSet>();

    // auto path = std::get<std::string>(get_param("path"));
    // pass in FloatGrid::Ptr
    zs::OpenVDBStruct gridPtr =
        get_input("VDBFloatGrid")->as<VDBFloatGrid>()->m_grid;

    ls->get() = zs::convertFloatGridToSparseLevelSet(
        gridPtr, zs::MemoryHandle{zs::memsrc_e::um, 0});
    fmt::print(fg(fmt::color::cyan), "done executing ToZensimLevelSet\n");
    set_output("ZSLevelSet", ls);
  }
};

static int defToZensimLevelSet = zen::defNodeClass<ToZensimLevelSet>(
    "ToZensimLevelSet", {/* inputs: */ {"VDBFloatGrid"},
                         /* outputs: */ {"ZSLevelSet"},
                         /* params: */ {},
                         /* category: */ {"GPUMPM"}});

} // namespace zen