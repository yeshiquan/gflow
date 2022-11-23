#pragma once

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <any>
#include <memory>

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
#include <boost/preprocessor/stringize.hpp>

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
    explicit GraphVertex(Graph *graph, std::shared_ptr<GraphProcessor> processor, std::string name);
    // 禁止拷贝和移动
    inline GraphVertex(GraphVertex &&) = delete;
    inline GraphVertex(const GraphVertex &) = delete;
    inline GraphVertex &operator=(GraphVertex &&) = delete;
    inline GraphVertex &operator=(const GraphVertex &) = delete;
    ~GraphVertex();

    bool is_ready_before_run();
    bool is_ready();
    bool decr_waiting_num();
    void add_waiting_num();
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

    const std::string& name() { return _name; }

    template <typename T>
    T &get_context() {
        return std::any_cast<T &>(_any_ctx);
    }

    std::vector<GraphDependency *>& get_optional_dependencys() {
        return _optional_dependencys;
    }

    std::string get_binding_data_name(std::string var_name) {
        if (_data_binding_map.count(var_name) == 0) {
            throw std::runtime_error("can't find graph data for var name[" + var_name + "]");
        }
        return _data_binding_map[var_name];
    }

    // 建图相关
    GraphDependency* depend_and_bind(const std::string& data_name, const std::string& var_name);
    GraphData* emit_and_bind(const std::string& data_name, const std::string& var_name);
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
    std::shared_ptr<GraphProcessor> _processor;
    std::atomic<int64_t> _waiting_num = 0;
    GraphExecutor *_executor = nullptr;
    ClosureContext *_closure_context = nullptr;
    std::string _name;
    std::atomic<bool> _is_activated{false};
    // var name和graph data name的绑定，key是var name
    // 例如 var name为"response", data name为"RESPONSE"
    std::unordered_map<std::string, std::string> _data_binding_map;
};

}  // namespace

// ========================== 第1套接口，适用于非通用算子，简单易用 ====================
// 使用示例：
// GRAPH_DECLARE(
//     DEPEND(std::vector<std::string>, MERGE_RESULT, merge_mids)
//     EMIT(std::vector<std::string>, RESULT, response)
// );
#define DEPEND(type, data_name, var_name) ((0, type, #data_name, var_name))
#define EMIT(type, data_name, var_name) ((1, type, #data_name, var_name))

#define IS_EMIT(args) BOOST_PP_EQUAL(BOOST_PP_TUPLE_ELEM(0, args), 1)

// EMIT(std::vector<std::string>, RESULT, response)
// 相当于std::vector<std::string>
#define DATA_TYPE(args) BOOST_PP_TUPLE_ELEM(1, args) 

// EMIT(std::vector<std::string>, RESULT, response)
// 相当于RESULT, GraphData对应的名字
#define DATA_NAME(args) BOOST_PP_TUPLE_ELEM(2, args)

// EMIT(std::vector<std::string>, RESULT, response)
// 相当于response, 类型是std::vector<std::string>*
#define RAW_VAR(args) BOOST_PP_TUPLE_ELEM(3, args)

// DEPEND(std::vector<std::string>, RESULT, response)
// 相当于 response_graph_data__, 类型是GraphData*
#define GRAPH_DATA_VAR(args) BOOST_PP_CAT(RAW_VAR(args), _graph_data__)

// 用于手动release数据
#define RELEASE(raw_var) raw_var##_graph_data__->release();

#define DECLARE(r, data, args) \
    DATA_TYPE(args)* RAW_VAR(args) = nullptr;\
    GraphData* GRAPH_DATA_VAR(args) = nullptr;

#define SETUP(r, data, args) \
    GRAPH_DATA_VAR(args) = get_data(DATA_NAME(args));

#define INIT(r, data, args) \
    BOOST_PP_IF( \
        IS_EMIT(args), \
        RAW_VAR(args) = GRAPH_DATA_VAR(args)->make<DATA_TYPE(args)>()->pointer<DATA_TYPE(args)>();, \
        RAW_VAR(args) = GRAPH_DATA_VAR(args)->pointer<DATA_TYPE(args)>();  \
        GRAPH_DATA_VAR(args)->any_data().require_same_type<DATA_TYPE(args)>(BOOST_PP_STRINGIZE(DATA_NAME(args))); \
    )

#define DO_NOTHING() // do nothing
#define AUTO_RELEASE(r, data, args) \
    BOOST_PP_IF( \
        IS_EMIT(args), \
        GRAPH_DATA_VAR(args)->release();,\
        DO_NOTHING() \
    )

#define GRAPH_DECLARE(seq) \
    int __auto_setup_data() { \
        BOOST_PP_SEQ_FOR_EACH(SETUP, _, seq) \
        return 0; \
    } \
    int __auto_init_data() { \
        BOOST_PP_SEQ_FOR_EACH(INIT, _, seq) \
        return 0; \
    } \
    void __auto_release_data() { \
        BOOST_PP_SEQ_FOR_EACH(AUTO_RELEASE, _, seq) \
    } \
    BOOST_PP_SEQ_FOR_EACH(DECLARE, _, seq)


// ========================== 第2套接口，适用于通用算子，稍微复杂一点 ====================
// 使用示例：
// VAR_DECLARE(
//     DEPEND_VAR(std::vector<std::string>, merge_mids)
//     EMIT_VAR(std::vector<std::string>, response)
// );
#define DEPEND_VAR(type, var_name) ((0, type, var_name))
#define EMIT_VAR(type, var_name) ((1, type, var_name))

#define IS_EMIT_V2(args) BOOST_PP_EQUAL(BOOST_PP_TUPLE_ELEM(0, args), 1)

// EMIT(std::vector<std::string>, response)
// 相当于std::vector<std::string>
#define DATA_TYPE_V2(args) BOOST_PP_TUPLE_ELEM(1, args) 

// EMIT(std::vector<std::string>, response)
// 相当于response, 类型是std::vector<std::string>*
#define RAW_VAR_V2(args) BOOST_PP_TUPLE_ELEM(2, args)

// DEPEND(std::vector<std::string>, response)
// 相当于 response_graph_data__, 类型是GraphData*
#define GRAPH_DATA_VAR_V2(args) BOOST_PP_CAT(RAW_VAR_V2(args), _graph_data__)

// DEPEND(std::vector<std::string>, response)，生成如下代码：
// std::vector<std::string>* response = nullptr;
// GraphData* response_graph_data__ = nullptr;
#define DECLARE_V2(r, data, args) \
    DATA_TYPE_V2(args)* RAW_VAR_V2(args) = nullptr;\
    GraphData* GRAPH_DATA_VAR_V2(args) = nullptr;

// response_graph_data__ = get_data(_vertex->get_binding_data_name("response"));
#define SETUP_V2(r, data, args) \
    GRAPH_DATA_VAR_V2(args) = get_data(_vertex->get_binding_data_name(BOOST_PP_STRINGIZE(RAW_VAR_V2(args))));

// 如果是EMIT，生成如下代码：
// response = response_graph_data__->make<std::vector<std::string>>()->pointer<std::vector<std::string>>();
// 如果是DEPEND，生成如下代码: 
// response = response_graph_data__->pointer<std::vector<std::string>>();
// response_graph_data__->any_data().require_same_type<std::vector<std::string>>("RESPONSE");
#define INIT_V2(r, data, args) \
    BOOST_PP_IF( \
        IS_EMIT_V2(args), \
        RAW_VAR_V2(args) = GRAPH_DATA_VAR_V2(args)->make<DATA_TYPE_V2(args)>()->pointer<DATA_TYPE_V2(args)>();, \
        RAW_VAR_V2(args) = GRAPH_DATA_VAR_V2(args)->pointer<DATA_TYPE_V2(args)>();  \
        GRAPH_DATA_VAR_V2(args)->any_data().require_same_type<DATA_TYPE_V2(args)>( \
                        _vertex->get_binding_data_name(BOOST_PP_STRINGIZE(RAW_VAR_V2(args))) \
        ); \
    )

#define AUTO_RELEASE_V2(r, data, args) \
    BOOST_PP_IF( \
        IS_EMIT_V2(args), \
        GRAPH_DATA_VAR_V2(args)->release();,\
        DO_NOTHING() \
    )

#define VAR_DECLARE(seq) \
    int __auto_setup_data() { \
        BOOST_PP_SEQ_FOR_EACH(SETUP_V2, _, seq) \
        return 0; \
    } \
    int __auto_init_data() { \
        BOOST_PP_SEQ_FOR_EACH(INIT_V2, _, seq) \
        return 0; \
    } \
    void __auto_release_data() { \
        BOOST_PP_SEQ_FOR_EACH(AUTO_RELEASE_V2, _, seq) \
    } \
    BOOST_PP_SEQ_FOR_EACH(DECLARE_V2, _, seq)
