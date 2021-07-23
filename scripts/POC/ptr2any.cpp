#include "any.h"
#include <cstdio>

using namespace zeno;

struct IObject {
    using poly_base = IObject;

    virtual void hello() const {
        printf("IObject\n");
    }
};

struct DemoObject : IObject {
    virtual void hello() const {
        printf("DemoObject\n");
    }
};

int main() {
    auto x = make_shared<DemoObject>();
    auto p = shared_cast<IObject>(x);
    p->hello();
    return 0;
}

