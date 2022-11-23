#include <iostream>
#include <functional>
#include <type_traits>
#include <future>

namespace gflow::concurrent {

// ::std::function要求包装目标必须可以拷贝构造
// 而典型的被包装对象bind和lambda，都可能闭包携带不可拷贝资源
// 为了能支持这部分对象的统一包装，提供一种只能移动的function类型
template <typename>
class MoveOnlyFunction;

template <typename R, typename... Args>
class MoveOnlyFunction<R(Args...)> {
public:
    inline MoveOnlyFunction() noexcept = default;
    inline MoveOnlyFunction(MoveOnlyFunction&&) = default;
    inline MoveOnlyFunction(const MoveOnlyFunction&) = delete;
    inline MoveOnlyFunction& operator=(MoveOnlyFunction&&) = default;
    inline MoveOnlyFunction& operator=(const MoveOnlyFunction&) = delete;

    template <typename C, typename = typename std::enable_if_t<
        // 满足调用签名
        std::is_invocable_v<C, Args...>
        // 支持移动构造
        && std::is_move_constructible_v<C>
        // 非引用类型，以及函数类型
        && (!std::is_reference_v<C>
            || std::is_function_v<typename std::remove_reference_t<C>>)>>
    inline MoveOnlyFunction(C&& callable);

    // 是否可调用，即持有函数
    inline operator bool() const noexcept;

    // 实际调用
    inline R operator()(Args... args) const;

private:
    ::std::function<R(Args&&...)> _function;
};

template <typename C, typename R, typename... Args>
class MoveOnlyCallable {
public:
    // 只支持移动，补充无效的拷贝构造实现，用来支持存入std::function
    inline MoveOnlyCallable() = delete;
    inline MoveOnlyCallable(MoveOnlyCallable&&) = default;
    inline MoveOnlyCallable(const MoveOnlyCallable&);
    inline MoveOnlyCallable& operator=(MoveOnlyCallable&&) = default;
    inline MoveOnlyCallable& operator=(const MoveOnlyCallable&);
    // 从Callable移动构造
    inline MoveOnlyCallable(C&& callable);
    // 代理Callable的调用
    inline R operator() (Args&&... args);

private:
    C _callable;
};

template <typename C, typename R, typename... Args>
inline MoveOnlyCallable<C, R, Args...>::MoveOnlyCallable(const MoveOnlyCallable& other) :
        _callable(::std::move(const_cast<MoveOnlyCallable&>(other))._callable) {
	LOG(FATAL) << "Can't Call Copy Constructor MoveOnlyCallable";
    ::abort();
}

template <typename C, typename R, typename... Args>
inline MoveOnlyCallable<C, R, Args...>& MoveOnlyCallable<C, R, Args...>::operator=(const MoveOnlyCallable&) {
	LOG(FATAL) << "Can't Call Copy Assign MoveOnlyCallable";
    ::abort();
}

template <typename C, typename R, typename... Args>
inline MoveOnlyCallable<C, R, Args...>::MoveOnlyCallable(C&& callable) :
    _callable(::std::move(callable)) {}

template <typename C, typename R, typename... Args>
inline R MoveOnlyCallable<C, R, Args...>::operator() (Args&&... args) {
    return static_cast<R>(_callable(::std::forward<Args>(args)...));
}


template <typename R, typename... Args>
template <typename C, typename>
inline MoveOnlyFunction<R(Args...)>::MoveOnlyFunction(C&& callable) :
    _function(MoveOnlyCallable<C, R, Args...>(::std::move(callable))) {}

template <typename R, typename... Args>
inline MoveOnlyFunction<R(Args...)>::operator bool() const noexcept {
    return static_cast<bool>(_function);
}

template <typename R, typename... Args>
inline R MoveOnlyFunction<R(Args...)>::operator()(Args... args) const {
    return _function(::std::forward<Args>(args)...);
}

} // namespace