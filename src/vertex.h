#pragma once

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <any>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/comparison/less.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>
#include <boost/preprocessor/tuple/push_front.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/facilities/empty.hpp>

namespace gflow {

class Graph;
class GraphDependency;
class GraphProcessor;
class GraphData;
class GraphExecutor;

class GraphVertex;
class ClosureContext;

class GraphVertex {
   public:
    explicit GraphVertex(Graph *graph, GraphProcessor *processor);
    // 禁止拷贝和移动
    inline GraphVertex(GraphVertex &&) = delete;
    inline GraphVertex(const GraphVertex &) = delete;
    inline GraphVertex &operator=(GraphVertex &&) = delete;
    inline GraphVertex &operator=(const GraphVertex &) = delete;
    ~GraphVertex() {}

    bool is_ready_before_run();
    bool is_ready();
    bool decr_waiting_num();
    GraphData *get_data(std::string name);
    void activate(std::vector<GraphVertex *> &vertexs,
                  ClosureContext *closure_context);
    void reset();
    int run();
    void execute();
    void set_closure_context(ClosureContext *closure_context) {
        _closure_context = closure_context;
    }
    ClosureContext *get_closure_context() { return _closure_context; }
    void set_executor(GraphExecutor *executor) { _executor = executor; }
    
    template<typename T>
    const T& get_option() {
        return std::any_cast<const T&>(_option);
    }
    std::any& get_any_option() {
        return _option;
    }

    template<typename T>
    void set_option(T&& option) {
        _option = std::move(option);
    }
    
    template <typename T>
    T &make_context() {
        _any_ctx = std::make_any<T>();
        return get_context<T>();
    }

    template <typename T>
    T &get_context() {
        return std::any_cast<T &>(_any_ctx);
    }

    std::vector<GraphDependency *>& get_optional_dependencys() {
        return _optional_dependencys;
    }

    // 建图相关
    GraphDependency *depend(std::string name);
    GraphDependency *optional_depend(std::string name);
    GraphData *emit(std::string name);
    void build();

   private:
    std::vector<GraphDependency *> _dependencys;
    std::vector<GraphDependency *> _optional_dependencys;
    std::vector<GraphData *> _emits;
    Graph *_graph = nullptr;
    std::any _option;
    std::any _any_ctx;
    GraphProcessor *_processor = nullptr;
    std::atomic<int64_t> _waiting_num = 0;
    GraphExecutor *_executor = nullptr;
    ClosureContext *_closure_context = nullptr;
};

}  // namespace


// GRAPH_DECLARE(
//     DEPEND(std::vector<std::string>, MERGE_RESULT, merge_mids)
//     EMIT(std::vector<std::string>, RESULT, response)
// );
#define DEPEND(type, data_name, var_name) ((0, type, #data_name, var_name))
#define EMIT(type, data_name, var_name) ((1, type, #data_name, var_name))

#define IS_EMIT(args) BOOST_PP_EQUAL(BOOST_PP_TUPLE_ELEM(0, args), 1)
#define DATA_TYPE(args) BOOST_PP_TUPLE_ELEM(1, args)
#define DATA_NAME(args) BOOST_PP_TUPLE_ELEM(2, args)
#define RAW_VAR(args) BOOST_PP_TUPLE_ELEM(3, args)
#define DATA_VAR(args) BOOST_PP_CAT(RAW_VAR(args), _data)

#define DECLARE(r, data, args) \
    DATA_TYPE(args)* RAW_VAR(args) = nullptr;\
    GraphData* DATA_VAR(args) = nullptr;

#define SETUP(r, data, args) \
    BOOST_PP_IF( \
        IS_EMIT(args), \
        DATA_VAR(args) = get_data(DATA_NAME(args))->make<DATA_TYPE(args)>();, \
        DATA_VAR(args) = get_data(DATA_NAME(args)); \
    )

#define INIT(r, data, args) \
    RAW_VAR(args) = &(DATA_VAR(args)->raw<DATA_TYPE(args)>());

#define DO_NOTHING() // do nothing
#define RELEASE(r, data, args) \
    BOOST_PP_IF( \
        IS_EMIT(args), \
        DATA_VAR(args)->release();,\
        DO_NOTHING() \
    )

#define GRAPH_DECLARE(seq) \
    int __auto_setup_data() { \
        BOOST_PP_SEQ_FOR_EACH(SETUP, _, seq) \
        return 0; \
    } \
    int __auto_init_data() {\
        BOOST_PP_SEQ_FOR_EACH(INIT, _, seq) \
        return 0; \
    } \
    void __auto_release_data() { \
        BOOST_PP_SEQ_FOR_EACH(RELEASE, _, seq) \
    } \
    BOOST_PP_SEQ_FOR_EACH(DECLARE, _, seq)