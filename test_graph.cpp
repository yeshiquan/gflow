#include <baidu/graph/engine/builder.h>

using baidu::feed::graph::GraphBuilder;
using baidu::feed::graph::GraphProcessor;
using baidu::feed::graph::GraphFunction;
using baidu::feed::graph::Closure;
using baidu::feed::mlarch::babylon::ApplicationContext;
#include <chrono>
#include <thread>

// 一个简单的算子，做加法，继承函数基类
class PlusFunction : public GraphFunction {
    // 实现计算功能
    virtual int32_t operator()() noexcept override {
        std::cout << "PlusFunction start..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        *z.emit() = *x + *y;
        std::cout << "PlusFunction end..." << std::endl;
        return 0;
    }
    // 定义函数接口输入x，y两个整数，输出z
    GRAPH_FUNCTION_INTERFACE(GRAPH_FUNCTION_DEPEND_DATA(int32_t, x)
                                 GRAPH_FUNCTION_DEPEND_DATA(int32_t, y)
                                     GRAPH_FUNCTION_EMIT_DATA(int32_t, z));
};
// 注册到ApplicationContext用于组装
BABYLON_REGISTER_FACTORY_COMPONENT_WITH_TYPE_NAME(PlusFunction, GraphProcessor,
                                                  PlusFunction);

class MultiplyFunction : public GraphFunction {
    // 实现计算功能
    virtual int32_t operator()() noexcept override {
        std::cout << "MultiplyFunction start..." << std::endl;
        *r.emit() = *z * 99;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        std::cout << "MultiplyFunction end..." << std::endl;
        return 0;
    }
    // 定义函数接口输入x，y两个整数，输出z
    GRAPH_FUNCTION_INTERFACE(GRAPH_FUNCTION_DEPEND_DATA(int32_t, z)
                                 GRAPH_FUNCTION_EMIT_DATA(int32_t, r));
};
// 注册到ApplicationContext用于组装
BABYLON_REGISTER_FACTORY_COMPONENT_WITH_TYPE_NAME(MultiplyFunction,
                                                  GraphProcessor,
                                                  MultiplyFunction);

void task() {
    ApplicationContext::instance().initialize(false);
    GraphBuilder builder;
    // 创建图节点
    {
        // 使用ApplicationContext寻找名为PlusFunction的函数用于计算
        auto& v = builder.add_vertex("PlusFunction");
        // 依赖A和B，输出C
        // 绑定函数形参x, y和z
        v.named_depend("x").to("A");
        v.named_depend("y").to("B");
        v.named_emit("z").to("C");
        auto& v2 = builder.add_vertex("MultiplyFunction");
        v2.named_depend("z").to("C");
        v2.named_emit("r").to("D");
    }
    // 结束构建
    builder.finish();
    // 获得一个可以运行的图实例
    auto graph = builder.build();
    // 找到图数据，用于外部操作
    auto* a = graph->find_data("A");
    auto* b = graph->find_data("B");
    auto* c = graph->find_data("C");
    auto* d = graph->find_data("D");
    // 初始数据赋值
    *(a->emit<int32_t>()) = 1;
    *(b->emit<int32_t>()) = 2;
    // 针对c数据进行求解，会使用A, B的数据为依赖，运行节点来得到C
    auto closure = graph->run(d);

    // closure.wait();

    std::cout << "trace 1--------------" << std::endl;
    auto ret = closure.finished();
    std::cout << "trace 2--------------finished:" << ret << std::endl;

    // closure.on_finish([graph = std::move(graph)] (Closure&& finished_closure)
    // {
    // 	std::cout << "on_finish trace 1..." << std::endl;
    //     finished_closure.wait();
    //     finished_closure.get();
    // 	std::cout << "on_finish trace 2..." << std::endl;
    // });

    closure.wait();
    std::cout << "trace4..." << std::endl;
    int32_t v = *d->cvalue<int32_t>();
    std::cout << "trace5..." << std::endl;
    std::cout << "value -> " << v << std::endl;
}

int32_t main(int32_t, char**) {
    std::thread thd(task);
    thd.join();
    return 0;
}
