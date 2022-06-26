#pragma once

namespace gflow {

/*
template<typename M>
inline std::unique_ptr<ClosureContext> ClosureContext::create() {
    return std::make_unique<ClosureContextImpl<M>>();
}
*/

template<typename M>
inline ClosureContext* ClosureContext::create() {
    ClosureContext* ctx = new ClosureContextImpl<M>();
    LOG(NOTICE) << "ClosureContext create ctx:" << ctx;
    return ctx;
}

}
