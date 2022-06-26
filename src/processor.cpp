#include "vertex.h"
#include "dependency.h"
#include "data.h"
#include "processor.h"

namespace gflow {

GraphData *GraphProcessor::get_data(std::string name) {
    return _vertex->get_data(name);
}

int GraphFunction::setup() {
    if (__auto_setup_data() != 0) {
        return -1;
    }
    return setup(_vertex->get_any_option());
}     

int GraphFunction::process() {
    if (__auto_init_data() != 0) {
        return -1;
    }
    auto ret = (*this)(); // 调用operator()
    __auto_release_data();
    return ret;
}

}  // namespace
