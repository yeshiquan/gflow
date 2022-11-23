#include "vertex.h"
#include "dependency.h"
#include "processor.h"
#include "graph.h"
#include "graph_executor.h"
#include "closure.h"

namespace gflow {

GraphVertex::GraphVertex(Graph *graph, std::shared_ptr<GraphProcessor> processor, std::string name)
    : _graph(graph), _processor(processor), _name(std::move(name)) {
    _processor->set_vertex(this);
}

GraphVertex::~GraphVertex() {
    for (auto& dep : _dependencys) {
        delete dep;
    }
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
    GraphData *data = _graph->create_data(name);
    data->set_producer(this);
    _emits.emplace_back(data);
    return data;
}

GraphDependency* GraphVertex::depend_and_bind(const std::string& data_name, const std::string& var_name) {
    if (_data_binding_map.count(var_name)) {
        LOG(FATAL) << "depend duplicated var name:" << var_name;
        return nullptr;
    }
    _data_binding_map.emplace(var_name, data_name);
    auto *dependency = depend(data_name);
    return dependency;
}

GraphData* GraphVertex::emit_and_bind(const std::string& data_name, const std::string& var_name) {
    if (_data_binding_map.count(var_name)) {
        LOG(FATAL) << "emit duplicated var name:" << var_name;
        return nullptr;
    }
    _data_binding_map.emplace(var_name, data_name);
    GraphData *data = emit(data_name);
    return data;
}

int GraphVertex::run() {
    //LOG(TRACE) << "GraghVertex::run start -> " << name();
    // 可能别的节点在执行过程出错了，然后图被标记结束了，那即便轮到我也不要执行了
    if (_closure_context->is_mark_finished()) {
        return 0;
    }
    int error_code = _processor->process();
    if (error_code != 0) {
        LOG(TRACE) << "GraphVertex[" << name() << "] run error! mark_finish error_code:"
                    << error_code;
        _closure_context->mark_finish(error_code);
        return error_code;
    }
    LOG(NOTICE) << "GraphVertex[" << name() << "] is finished";
    _closure_context->one_vertex_finished();
    return 0;
}

void GraphVertex::execute() { 
    _executor->execute(this, _closure_context); 
}

bool GraphVertex::is_ready_before_run() { return is_ready(); }

bool GraphVertex::is_ready() {
    return _waiting_num.load(std::memory_order_acquire) == 0;
}

bool GraphVertex::decr_waiting_num() {
    // 有可能变为负数，如果在activate之前依赖的数据就已经ready了
    auto waiting_num = _waiting_num.fetch_sub(1, std::memory_order_acq_rel) - 1;
    LOG(TRACE) << "GraphVertex[" << name() << "] decr waiting_num:" << _waiting_num.load();
    bool is_vertx_ready = (waiting_num == 0);
    return is_vertx_ready;
}

void GraphVertex::add_waiting_num() {
    _waiting_num.fetch_add(1, std::memory_order_release);
}

void GraphVertex::activate(std::vector<GraphVertex *> &vertexs,
                           ClosureContext *closure_context) {
    set_closure_context(closure_context);

    // 有可能多个线程同时激活1个vertex，不用原子变量的cas操作会有data race问题
    bool expected = false;
    if (!_is_activated.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
        return;
    }
    
    vertexs.emplace_back(this);

    int64_t waiting_num = 0;
    for (auto &dependency : _dependencys) {
        dependency->activate(vertexs, closure_context);
        // if (!dependency->is_ready()) {
        waiting_num++;
        //}
    }
    LOG(TRACE) << "GraphVertex[" << name() << "] activate, waiting_num:" << waiting_num
                << ", depend:" << _dependencys.size();
    _waiting_num.fetch_add(waiting_num, std::memory_order_release);
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
    _waiting_num.store(0, std::memory_order_release);
    _is_activated.store(false, std::memory_order_relaxed);
}

GraphData *GraphVertex::get_data(std::string name) {
    return _graph->get_data(name);
}

void GraphVertex::build() { _processor->setup(); }

}  // namespace
