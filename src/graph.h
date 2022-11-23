#pragma once

#include "vertex.h"
#include "data.h"
#include "processor.h"
#include "closure.h"
#include "graph_executor.h"

#include <map>
#include <vector>
#include <string>
#include <future>
#include <mutex>

namespace gflow {

constexpr static int32_t BthreadExecutorType = 1;
constexpr static int32_t AsyncExecutorType = 2;

class Graph {
   public:
    // 禁止拷贝和移动
    inline Graph(Graph &&) = delete;
    inline Graph(const Graph &) = delete;
    inline Graph &operator=(Graph &&) = delete;
    inline Graph &operator=(const Graph &) = delete;

    inline Graph() {
        init();
    }

    inline void init() {
#ifdef USE_BTHREAD
        _executor = BthreadGraphExecutor::instance();
#else
        _executor = AsyncGraphExecutor::instance();
#endif
    }

    GraphVertex *add_vertex(std::string processor_name) {
        std::shared_ptr<GraphProcessor> processor = ProcessorFactory::instance().create(processor_name);
        if (!processor) {
            LOG(ERROR) << "create GraphProcessor failed!! invalid processor name -> " << processor_name;
            return nullptr;
        }
        GraphVertex* vertex = add_vertex(processor, processor_name);
        LOG(TRACE) << "GraphVertex add_vertex create processor:" << processor_name << " ptr:" << processor << " vertex:" << vertex;
        return vertex;
    }

    GraphVertex *add_vertex(std::shared_ptr<GraphProcessor> processor, std::string processor_name) {
        auto *v = new GraphVertex(this, processor, processor_name);
#ifdef USE_BTHREAD
        v->set_executor(BthreadGraphExecutor::instance());
#else
        v->set_executor(AsyncGraphExecutor::instance());
#endif
        _vertixes.emplace_back(v);
        return v;
    }

    GraphData *get_data(std::string data_name) {
        auto iter = _global_data.find(data_name);
        if (iter == _global_data.end()) {
            LOG(WARNING) << "GraphData get_data is null name:" << data_name;
            return nullptr;
        }
        return iter->second;
    }

    GraphData *create_data(std::string data_name) {
        auto iter = _global_data.find(data_name);
        if (iter == _global_data.end()) {
            auto *graph_data = new GraphData(data_name);
            _global_data.emplace(data_name, graph_data);
            return graph_data;
        }
        return iter->second;
    }

    void build() {
        for (auto vertex : _vertixes) {
            vertex->build();
        }
    }

    ClosureContext* run(GraphData *data) {
        //auto closure_context = std::make_unique<ClosureContext>();
        auto* closure_context = _executor->create_closure_context();
        // 让那些没有依赖的vertex先执行，因为只有是data的上游vertex才需要执行，
        // 所以不能全局遍历所有vertex，需要先递归遍历一遍把data的上游vertex标记出来。
        std::vector<GraphVertex *> actived_vertexs;
        //data->activate(actived_vertexs, closure_context.get());
        data->activate(actived_vertexs, closure_context);
        closure_context->add_wait_vertex_num(actived_vertexs.size());
        LOG(TRACE) << "--------------------- activate done begin execute -------------";    
            
        LOG(TRACE) << ">>> Graph run activated vertexs size:"
                    << actived_vertexs.size() << " vertexes:[" << noflush;
        for (GraphVertex *vertex : actived_vertexs) {
            LOG(TRACE) << vertex->name() << "," << noflush;
        }
        LOG(TRACE) << "]";

        for (GraphVertex *vertex : actived_vertexs) {
            if (vertex->is_ready_before_run()) {
                LOG(TRACE)
                    << "vertex[" << vertex->name() << "] is ready before run";
                vertex->execute();
            }
        }
        //return std::move(closure_context);
        return closure_context;
    }

    void reset() {
        for (auto vertex : _vertixes) {
            vertex->reset();
        }
        for (auto& [name, data] : _global_data) {
            data->reset();
        }
    }

    ~Graph() {
        for (auto vertex : _vertixes) {
            vertex->reset();
            delete vertex;
        }
        for (auto& [name, data] : _global_data) {
            data->reset();
            delete data;
        }        
    }

   private:
    std::vector<GraphVertex *> _vertixes;
    std::unordered_map<std::string, GraphData *> _global_data;
    GraphExecutor* _executor = nullptr;
    std::mutex _mutex;
};

}  // namespace
