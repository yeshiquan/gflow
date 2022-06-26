#pragma once

#include <tuple>
#include <bthread.h>
#include <bthread/mutex.h>
#include <future>
#include <mutex>

namespace gflow {

class GraphDependency;
class GraphVertex;
class ClosureContext;

struct Params {
    Params(GraphVertex* vertex_, ClosureContext* closure_context_) : 
                    vertex(vertex_), 
                    closure_context(closure_context_) {}
    GraphVertex* vertex = nullptr;
    ClosureContext* closure_context = nullptr;
};

inline void* run_vertex(void* args);

class GraphExecutor {
   public:
    GraphExecutor() = default;
    GraphExecutor(GraphExecutor const&) = delete;             // Copy construct
    GraphExecutor(GraphExecutor&&) = delete;                  // Move construct
    GraphExecutor& operator=(GraphExecutor const&) = delete;  // Copy assign
    GraphExecutor& operator=(GraphExecutor&&) = delete;       // Move assign

    virtual ClosureContext* create_closure_context() = 0;

    virtual int32_t execute(GraphVertex* vertex,
                            ClosureContext* closure_context) = 0;
};

class BthreadGraphExecutor : public GraphExecutor {
   public:
    static GraphExecutor* instance() {
        static BthreadGraphExecutor ins;
        return &ins;
    }
    ClosureContext* create_closure_context() override;
    int32_t execute(GraphVertex* vertex,
                    ClosureContext* closure_context) override;
};

class AsyncGraphExecutor : public GraphExecutor {
   public:
    static GraphExecutor* instance() {
        static AsyncGraphExecutor ins;
        return &ins;
    }
    ClosureContext* create_closure_context() override;
    int32_t execute(GraphVertex* vertex,
                    ClosureContext* closure_context) override;
};

}  // namespace gflow
