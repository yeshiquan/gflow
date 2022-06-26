#include "vertex.h"
#include "dependency.h"
#include "processor.h"
#include "graph.h"
#include "executor.h"
#include "closure.h"

namespace gflow {

GraphVertex::GraphVertex(Graph *graph, GraphProcessor *processor)
    : _graph(graph), _processor(processor) {
    _processor->set_vertex(this);
}

GraphDependency *GraphVertex::depend(std::string name) {
    auto *dependency = new GraphDependency(name, _graph, this);
    _dependencys.emplace_back(dependency);
    return dependency;
}

GraphDependency *GraphVertex::optional_depend(std::string name) {
    auto *dependency = depend(name);
    _optional_dependencys.emplace_back(dependency);
    return dependency;
}

GraphData *GraphVertex::emit(std::string name) {
    GraphData *data = _graph->ensure_data(name);
    data->set_producer(this);
    _emits.emplace_back(data);
    return data;
}

int GraphVertex::run() {
    // 可能别的节点在执行过程出错了，然后图被标记结束了，那即便轮到我也不要执行了
    if (_closure_context->is_mark_finished()) {
        return 0;
    }
    int error_code = _processor->process();
    if (error_code != 0) {
        LOG(WARNING) << "vertex run error! mark_finish error_code:"
                    << error_code;
        _closure_context->mark_finish(error_code);
        return error_code;
    }
    LOG(NOTICE) << "GraphVertex run finish vertex:" << this << " closure_context:" << _closure_context;
    _closure_context->one_vertex_finished();
    return 0;
}

void GraphVertex::execute() { _executor->execute(this, _closure_context); }

bool GraphVertex::is_ready_before_run() { return is_ready(); }

bool GraphVertex::is_ready() {
    return _waiting_num.load(std::memory_order_acquire) == 0;
}

bool GraphVertex::decr_waiting_num() {
    // 有可能变为负数，如果在activate之前依赖的数据就已经ready了
    auto waiting_num = _waiting_num.fetch_sub(1, std::memory_order_release) - 1;
    LOG(TRACE) << "vertex decr waiting_num:" << _waiting_num.load();
    bool is_vertx_ready = (waiting_num == 0);
    return is_vertx_ready;
}

void GraphVertex::activate(std::vector<GraphVertex *> &vertexs,
                           ClosureContext *closure_context) {
    set_closure_context(closure_context);

    vertexs.emplace_back(this);
    int64_t waiting_num = 0;
    for (auto &dependency : _dependencys) {
        dependency->activate(vertexs, closure_context);
        // if (!dependency->is_ready()) {
        waiting_num++;
        //}
    }
    LOG(TRACE) << "GraphVertex activate, waiting_num:" << waiting_num
                << ", depend:" << _dependencys.size();
    _waiting_num.fetch_add(waiting_num, std::memory_order_relaxed);
}

void GraphVertex::reset() {
    for (auto dependency : _dependencys) {
        dependency->reset();
    }
    /*
    for (auto& emitter : _emits) {
        emitter->reset();
    }
    */
    _waiting_num.store(0, std::memory_order_relaxed);
}

GraphData *GraphVertex::get_data(std::string name) {
    return _graph->get_data(name);
}

void GraphVertex::build() { _processor->setup(); }

}  // namespace
