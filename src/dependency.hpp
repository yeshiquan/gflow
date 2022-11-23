#include "data.h"

namespace gflow {

template<typename T>
T* GraphDependency::value() {
    DCHECK(_depend_data);
    return _depend_data->pointer<T>();
}

} // namespace