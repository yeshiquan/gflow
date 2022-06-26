#include "dependency.h"
#include "graph.h"
#include "vertex.h"
#include "expr_processor.h"

namespace gflow {

GraphDependency::GraphDependency(std::string name, Graph *graph,
                                 GraphVertex *vertex)
    : _name(name),
      _depend_data(graph->ensure_data(name)),
      _graph(graph),
      _attached_vertex(vertex) {
    _depend_data->add_downstream(this);
    //_fire_num.fetch_add(1, std::memory_order_relaxed);
}

void GraphDependency::reset() {
    //_is_ready = false;
    //_fire_num.store(0, std::memory_order_relaxed);
}

GraphDependency *GraphDependency::if_true(std::string condition) {
    LOG(TRACE) << "GraphDependency::if_true() " << condition;
    _condition_expr = std::move(condition);
    std::string result_name =
        "CONDITION_DATA_" + std::to_string(get_condition_data_idx());
    ExpressionProcessor *processor = new ExpressionProcessor;
    GraphVertex *expr_vertex =
        _graph->add_vertex(static_cast<GraphProcessor *>(processor));
    _condition_data = expr_vertex->emit(result_name);
    _condition_data->add_downstream(this);
    _condition_data->set_is_condition(true);
    //_fire_num.fetch_add(1, std::memory_order_relaxed);
    processor->init(_condition_expr, result_name);

    return this;
}

void GraphDependency::fire_condition(bool condition_value) {
    LOG(TRACE) << "GraphDependency fire_condition value:" << condition_value;
    if (condition_value) {
        // 条件成立，从这里开始继续执行图
        //_fire_num.fetch_sub(1, std::memory_order_relaxed);
        execute_from_me();
    } else {
        // 条件不成立，直接触发dependency
        //_fire_num.fetch_sub(2, std::memory_order_relaxed);
        fire();
    }
}

void GraphDependency::execute_from_me() {
    std::vector<GraphVertex *> actived_vertexs;
    ClosureContext *closure_context = _attached_vertex->get_closure_context();
    _depend_data->activate(actived_vertexs, closure_context);
    closure_context->add_wait_vertex_num(actived_vertexs.size());

    LOG(TRACE) << ">>> GraphDependency execute_from_me activated vertexs size:"
                << actived_vertexs.size();

    for (GraphVertex *vertex : actived_vertexs) {
        if (vertex->is_ready_before_run()) {
            LOG(TRACE) << "vertex is ready before run!! add vertex:" << vertex;
            vertex->execute();
        }
    }
}

void GraphDependency::fire() {
    bool is_vertex_ready = _attached_vertex->decr_waiting_num();

    LOG(TRACE) << "GraphDependency fire check attached_vertex -> " << noflush;

    if (is_vertex_ready) {
        LOG(TRACE) << "ready!! add vertex:" << _attached_vertex;
        _attached_vertex->execute();
    } else {
        LOG(TRACE) << "not ready vertex:" << _attached_vertex;
    }
}

void GraphDependency::activate(std::vector<GraphVertex *> &vertexs,
                               ClosureContext *closure_context) {
    if (_condition_data) {
        //LOG(TRACE) << "GraphDependency activate condition data";
        _condition_data->activate(vertexs, closure_context);
    } else {
        //LOG(TRACE) << "GraphDependency activate target data";
        // condition没出结果之前先不激活data
        _depend_data->activate(vertexs, closure_context);
    }
}

}  // namespace
