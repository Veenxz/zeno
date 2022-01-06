#pragma once
#include <memory>

#include <zeno/types/ListObject.h>
#include <zeno/types/PrimitiveObject.h>

namespace zeno {

ZENO_API std::shared_ptr<PrimitiveObject>
primitive_merge(std::shared_ptr<ListObject> list);

}