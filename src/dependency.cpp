#include "dependency.h"
#include "graph.h"
#include "vertex.h"
#include "expr_processor.h"

namespace gflow {

GraphDependency::GraphDependency(std::string name, Graph *graph,
                                 GraphVertex *vertex)
    : _name(name),
      _depend_data(graph->create_data(name)),
      _graph(graph),
      _attached_vertex(vertex) {
    _depend_data->add_downstream(this);
}

void GraphDependency::reset() {
    _condition_ready = false;
    _expect_num.store(0, std::memory_order_release);
}

GraphDependency *GraphDependency::when(std::string condition) {
    LOG(TRACE) << "GraphDependency::when() " << condition;
    _condition_expr = std::move(condition);

    auto condition_idx = get_condition_data_idx();
    std::string result_name = "CONDITION_DATA_" + std::to_string(condition_idx);
    std::string vertex_name = "ExpressionProcessor_" + std::to_string(condition_idx);

    auto processor = std::make_shared<ExpressionProcessor>();
    GraphVertex *expr_vertex =
        _graph->add_vertex(static_cast<std::shared_ptr<GraphProcessor>>(processor), vertex_name);
    _condition_data = expr_vertex->emit(result_name);
    _condition_data->add_downstream(this);
    _condition_data->set_is_condition(true);
    processor->init(_condition_expr, result_name);

    return this;
}

void GraphDependency::execute_from_me() {
    std::vector<GraphVertex *> actived_vertexs;
    ClosureContext *closure_context = _attached_vertex->get_closure_context();
    DCHECK(_depend_data);
    DCHECK(closure_context);
    _depend_data->activate(actived_vertexs, closure_context);
    closure_context->add_wait_vertex_num(actived_vertexs.size());

    LOG(TRACE) << ">>> GraphDependency execute_from_me activated vertexs size:"
                << actived_vertexs.size();

    for (GraphVertex *vertex : actived_vertexs) {
        if (vertex->is_ready_before_run()) {
            LOG(TRACE) << "vertex[" << vertex->name() << "] is ready before run";
            vertex->execute();
        }
    }
}

int32_t GraphDependency::fire_condition(bool condition_value) {
    _condition_ready = true;

    int32_t current_expect_num = _expect_num.fetch_sub(1, std::memory_order_acq_rel) - 1;    
    LOG(TRACE) << "GraphDependency fire_condition value:" << condition_value << " current_expect_num: " << (current_expect_num + 1);
    if (current_expect_num == 0) {
        fire();
        LOG(TRACE) << "GraphDependency::fire_condition case1";
        return current_expect_num;
    }
    if (condition_value == false) {
        current_expect_num = _expect_num.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (current_expect_num == 0) {
            fire();
            LOG(TRACE) << "GraphDependency::fire_condition case2";
            return current_expect_num;
        }        
    } else if (current_expect_num == 1) {
        // 还差data没有出结果，从data那里继续激活和执行
        LOG(TRACE) << "GraphDependency::fire_condition case3";
        execute_from_me();
    }
    return current_expect_num;
}

int32_t GraphDependency::fire_data() {
    int32_t current_expect_num = _expect_num.fetch_sub(1, std::memory_order_acq_rel) - 1;
    LOG(TRACE) << "GraphDependency::fire_data expect_num:" << current_expect_num;
    if (current_expect_num == 0) {
        fire();
    }
    return current_expect_num;
}

void GraphDependency::fire() {
    bool is_vertex_ready = _attached_vertex->decr_waiting_num();

    LOG(TRACE) << "GraphDependency fire(). attached_vertex[" << _attached_vertex->name() << "] " << noflush;

    if (is_vertex_ready) {
        LOG(TRACE) << "is ready";
        _attached_vertex->execute();
    } else {
        LOG(TRACE) << "is not ready";
    }
}

int32_t GraphDependency::activate(std::vector<GraphVertex *> &vertexs,
                               ClosureContext *closure_context) {
    int32_t num = (_condition_data != nullptr ? 2 : 1);
    int32_t current_expect_num = _expect_num.fetch_add(num, std::memory_order_acq_rel) + num;
    LOG(TRACE) << "GraphDependency::activate expect_num:" << current_expect_num << " " << _attached_vertex->name();

    if (current_expect_num == -1) {
        fire();
        LOG(TRACE) << "GraphDependency::activate branch 1 " << _attached_vertex->name();
    } else if (current_expect_num == 0) {
        fire();
        LOG(TRACE) << "GraphDependency::activate branch 2 " << _attached_vertex->name();
    } else if (current_expect_num == 1) {
        if (_condition_ready || _condition_data == nullptr) {
            _depend_data->activate(vertexs, closure_context);
            LOG(TRACE) << "GraphDependency::activate branch 3 " << _attached_vertex->name();
        } else if (_condition_data != nullptr) {
            _condition_data->activate(vertexs, closure_context);
            LOG(TRACE) << "GraphDependency::activate branch 4 " << _attached_vertex->name();
        }
    } else if (current_expect_num == 2) {
        _condition_data->activate(vertexs, closure_context);
        LOG(TRACE) << "GraphDependency::activate branch 5 " << _attached_vertex->name();
    } else {
        LOG(WARNING) << "GraphDependency::activate invalid expect_num:" << current_expect_num;
    }

    return current_expect_num;
}

}  // namespace
