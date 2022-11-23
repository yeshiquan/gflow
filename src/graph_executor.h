#pragma once

#include <tuple>
#include <future>
#include <mutex>

// #include <bthread.h>
// #include <bthread/mutex.h>

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

    ClosureContext* create_closure_context();

    virtual int32_t execute(GraphVertex* vertex,
                            ClosureContext* closure_context) = 0;
};

class BthreadGraphExecutor : public GraphExecutor {
   public:
    static GraphExecutor* instance() {
        static BthreadGraphExecutor ins;
        return &ins;
    }
    int32_t execute(GraphVertex* vertex,
                    ClosureContext* closure_context) override;
};

class AsyncGraphExecutor : public GraphExecutor {
   public:
    static GraphExecutor* instance() {
        static AsyncGraphExecutor ins;
        return &ins;
    }
    int32_t execute(GraphVertex* vertex,
                    ClosureContext* closure_context) override;
};

}  // namespace gflow
