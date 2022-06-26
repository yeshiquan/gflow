#pragma once

#include <map>
#include <string>
#include <any>
#include <memory>
#include <functional>

namespace gflow {

class GraphVertex;
class GraphData;
class GraphProcessor;

using ProcessorCreator = std::function<GraphProcessor*()>;
class ProcessorFactory {
private:
    std::unordered_map<std::string, ProcessorCreator> _creator_map;
public:
    void add_creator(const std::string name, ProcessorCreator creator) {
        _creator_map.emplace(name, creator);
    }

    GraphProcessor* create(const std::string name) {
        auto iter = _creator_map.find(name);
        if (iter == _creator_map.end()) {
            return nullptr;
        }
        ProcessorCreator creator = iter->second;
        return creator();
    }

    static ProcessorFactory& instance() {
        static ProcessorFactory _instance;
        return _instance;
    }
};

class ProcessorRegister {
public:
    ProcessorRegister(const std::string name, ProcessorCreator creator) {
        ProcessorFactory::instance().add_creator(name, creator);
    }
};

#define REGISTER_PROCESSOR(PROCESSOR) \
    using gflow::ProcessorRegister; \
    using gflow::GraphProcessor; \
    static ProcessorRegister PROCESSOR##ProcessorRegister(#PROCESSOR, []() -> GraphProcessor* { \
        return new PROCESSOR; \
    }); \

class GraphProcessor {
   public:
    GraphProcessor() = default;
    // 禁止拷贝和移动
    inline GraphProcessor(GraphProcessor &&) = delete;
    inline GraphProcessor(const GraphProcessor &) = delete;
    inline GraphProcessor &operator=(GraphProcessor &&) = delete;
    inline GraphProcessor &operator=(const GraphProcessor &) = delete;

    virtual ~GraphProcessor() {}
    virtual int process() = 0;
    virtual int setup() { return 0; }
    GraphData *get_data(std::string name);
    void set_vertex(GraphVertex *vertex) { _vertex = vertex; }
    GraphVertex& vertex() { return *_vertex; }

   protected:
    GraphVertex *_vertex = nullptr;
};

class GraphFunction : public GraphProcessor {
public:
    virtual int setup(std::any& option) = 0;
    virtual int operator()() = 0;
    virtual int setup() override;   
    virtual int process() override;
private:
    virtual int __auto_setup_data() = 0;
    virtual int __auto_init_data() = 0;    
    virtual void __auto_release_data() = 0;
};

}  // namespace
