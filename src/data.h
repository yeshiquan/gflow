#pragma once

#include "any.h"
#include <vector>
#include <iostream>
//#include <base/logging.h>
#include "check.h"

namespace gflow {

class GraphDependency;
class GraphVertex;
class ClosureContext;
class GraphData;

template<typename T>
class Commiter {
public:
    // 禁止拷贝
    Commiter(const Commiter &) = delete;
    Commiter &operator=(const Commiter &) = delete;
    // 可以移动
    Commiter(Commiter &&);
    Commiter &operator=(Commiter &&);
    Commiter(GraphData* data);
    ~Commiter();
    T& operator*() noexcept;
    T* operator->() noexcept;
    void set_value(const T& value) noexcept;
    void commit() noexcept;
    void set_value(T&& value) noexcept;
private:
    GraphData* _data = nullptr;
    bool _is_valid = true;
};

class GraphData {
   public:
    inline GraphData(std::string name) : _name(name){};
    // 禁止拷贝和移动
    inline GraphData(GraphData &&) = delete;
    inline GraphData(const GraphData &) = delete;
    inline GraphData &operator=(GraphData &&) = delete;
    inline GraphData &operator=(const GraphData &) = delete;
    const std::string &get_name() { return _name; }
    void add_downstream(GraphDependency *down_stream);
    void set_producer(GraphVertex *producer);

    template <typename T>
    void set_value(T&& value);

    template <typename T>
    Commiter<T> commiter();

    template <typename T>
    void emit_value(T&& value);
    
    template <typename T>
    GraphData *make();

    template <typename T>
    T &raw();

    template <typename T>
    T as();    

    template <typename T>
    T* pointer();    

    Any& any_data() {
        return _any_data;
    }

    // template <typename T>
    // const T* pointer();      

    void release();
    bool is_released() { return _is_released; }
    void activate(std::vector<GraphVertex *> &vertexs,
                  ClosureContext *closure_context);
    bool is_condition() { return _is_condition; }
    void set_is_condition(bool value) { _is_condition = value; }
    void reset() {
        _is_released = false;
        // 保留之前分配的空间，不要重置any容器的值，否则会访问未分配存储的数据
        //_any_data.clear();
    }

    ~GraphData() {
        _any_data.clear();
    }

   private:
    Any _any_data;
    GraphVertex *_producer = nullptr;
    std::vector<GraphDependency *> _down_streams;
    bool _is_released = false;
    bool _is_condition = false;
    std::string _name;
};

}  // namespace

#include "data.hpp"
