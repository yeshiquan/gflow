#include "data.h"
#include "vertex.h"
#include "dependency.h"

#include <future>

namespace gflow {

void GraphData::release() {
    _is_released = true;
    LOG(TRACE) << "GraphData[" << _name << "] is released."
                << " downstream_num:" << _down_streams.size()
                << " is_condition:" << _is_condition;
    if (_is_condition) {
        int condition_value = raw<int>();
        for (GraphDependency *downstream : _down_streams) {
            DCHECK(downstream);
            downstream->fire_condition(condition_value);
        }
    } else {
        for (GraphDependency *downstream : _down_streams) {
            DCHECK(downstream);
            downstream->fire_data();
        }
    }
}

void GraphData::activate(std::vector<GraphVertex *> &vertexs,
                         ClosureContext *closure_context) {
    if (_is_released) {
        return;
    }
    if (_producer) {
        LOG(TRACE) << "GraphData[" << _name << "] activate start";
        _producer->activate(vertexs, closure_context);
    } else {
        // LOG(TRACE) << "GraphData not activate without producer activate
        // name:" << _name;
    }
}

void GraphData::add_downstream(GraphDependency *down_stream) {
    _down_streams.emplace_back(down_stream);
}

void GraphData::set_producer(GraphVertex *producer) { _producer = producer; }

}  // namespace
