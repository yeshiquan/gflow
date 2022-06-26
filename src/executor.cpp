#include <tuple>
#include <bthread.h>
#include <bthread/mutex.h>
#include <future>
#include <mutex>
#include <assert.h>

#include "executor.h"
#include "closure.h"
#include "vertex.h"

namespace gflow {

inline void* run_vertex(void* args) {
    auto* params = reinterpret_cast<Params*>(args);
    GraphVertex* vertex = params->vertex;
    ClosureContext* closure_context = params->closure_context;
    assert(closure_context != nullptr);
    assert(closure_context == vertex->get_closure_context());
    assert(closure_context->is_delete.load() == false);
    vertex->run();
    delete params;
    return NULL;
}

ClosureContext* BthreadGraphExecutor::create_closure_context() {
    auto* p = new ClosureContext(1);
    return p;
    //return ClosureContext::create<bthread::Mutex>();
    //return std::move(p);
}

int32_t BthreadGraphExecutor::execute(GraphVertex* vertex,
                ClosureContext* closure_context) {
    auto* params = new Params(vertex, closure_context);
    bthread_t th;
    if (bthread_start_background(&th, NULL, run_vertex, params) != 0) {
        LOG(WARNING) << "bthread start to run vertex failed";
        delete params;
        return -1;
    }
    //LOG(NOTICE) << "bthread execute return from vertex:" << vertex;
    return 0;
}

ClosureContext* AsyncGraphExecutor::create_closure_context() {
    auto* p = new ClosureContext(2);
    return p;
    //return ClosureContext::create<std::mutex>();
    //return std::move(p);
}

int32_t AsyncGraphExecutor::execute(GraphVertex* vertex,
                ClosureContext* closure_context) {
    auto* params = new Params(vertex, closure_context);
    //std::async(std::launch::async, run_vertex, params);
    std::packaged_task<void*()> task([params]() -> void*{
        return run_vertex(params); 
    });
    task();
    //LOG(NOTICE) << "async execute return from vertex:" << vertex;
    return 0;
}

}  // namespace gflow
