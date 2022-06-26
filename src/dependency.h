#pragma once

#include <string>
#include <vector>
#include <atomic>

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
    void activate(std::vector<GraphVertex *> &vertexs,
                          ClosureContext *closure_context);
    void reset();
    GraphDependency *if_true(std::string condition);
    void execute_from_me();

    int get_condition_data_idx() {
        static int idx = 0;
        return idx++;
    }
    void fire_condition(bool condition_value);
    void fire();
    GraphData *get_data() { return _depend_data; }
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
    std::atomic<int32_t> _fire_num{0};
};

}  // namespace
