#pragma once

#include <string>
#include <vector>
#include <atomic>
#include "check.h"

namespace gflow {

class Graph;
class GraphData;
class GraphVertex;
class ClosureContext;

class GraphDependency {
   public:
    explicit GraphDependency(std::string name, Graph *graph,
                             GraphVertex *vertex);
    // 禁止拷贝和移动
    inline GraphDependency(GraphDependency &&) = delete;
    inline GraphDependency(const GraphDependency &) = delete;
    inline GraphDependency &operator=(GraphDependency &&) = delete;
    inline GraphDependency &operator=(const GraphDependency &) = delete;
    ~GraphDependency() {}

    //bool is_ready() { return _is_ready; }
    int32_t activate(std::vector<GraphVertex *> &vertexs,
                          ClosureContext *closure_context);
    void reset();
    GraphDependency *when(std::string condition);
    void execute_from_me();

    int get_condition_data_idx() {
        static int idx = 0;
        return idx++;
    }
    int32_t fire_condition(bool condition_value);
    int32_t fire_data();
    void fire();

    template<typename T>
    T *value();

    std::string get_name() { return _name; }
    GraphVertex *get_attached_vertex() { return _attached_vertex; }

   private:
    GraphData *_depend_data = nullptr;
    //bool _is_ready = false;

   protected:
    GraphData *_condition_data = nullptr;
    Graph *_graph = nullptr;
    GraphVertex *_attached_vertex = nullptr;
    std::string _name;
    std::string _condition_expr;
    std::atomic<int32_t> _expect_num{0};
    bool _condition_ready = false;
};

}  // namespace

#include "dependency.hpp"