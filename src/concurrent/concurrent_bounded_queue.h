#pragma once

#include <memory>
#include "futex_interface.h"
#include <chrono>

namespace gflow::concurrent {

#ifndef QUEUE_STATS
#define QUEUE_STATS false
#endif

#if QUEUE_STATS
#define INC_STATS(x) g_stat.x()
#define PRINT_STATS() g_stat.print()
#else 
#define INC_STATS(x)
#define PRINT_STATS() g_stat.do_nothing()
#endif

class QueueStat {
public:
    std::atomic<uint64_t> _wait{0};
    std::atomic<uint64_t> _wake{0};
    std::atomic<uint64_t> _futex_compacted{0};
    std::atomic<uint64_t> _futex_no_compacted{0};
    std::atomic<uint64_t> _loop{0};
public:
    void wait() { _wait++; }
    void wake() { _wake++; }
    void futex_compacted() { _futex_compacted++; }
    void futex_no_compacted() { _futex_no_compacted++; }
    void loop() { _loop++; }
    void do_nothing() {}
    void print() {
        std::cout << "QueueStat -> " 
                    << " wait:" << _wait.load()
                    << " wake:" << _wake.load()
                    << " futex_compacted:" << _futex_compacted.load()
                    << " futex_no_compacted:" << _futex_no_compacted.load()
                    << " loop:" << _loop.load()
                    << std::endl;
    }
};
static QueueStat g_stat;

using gflow::concurrent::FutexInterface;

constexpr size_t CACHELINE_SIZE = 64;

template <class T>
inline constexpr
typename std::enable_if_t<
  (std::is_integral_v<T> &&
   std::is_unsigned_v<T> &&
   sizeof(T) > sizeof(unsigned int) &&
   sizeof(T) <= sizeof(unsigned long)),
  unsigned int>
findLastSet(T x) {
    return x ? ((8 * sizeof(unsigned long) - 1) ^ __builtin_clzl(x)) + 1 : 0;
}

template <class T>
inline constexpr
typename std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, T>
nextPowTwo(T v) {
    return v ? (T(1) << findLastSet(v - 1)) : 1;
}

inline uint16_t get_raw_version(uint32_t tagged_version) noexcept {
    return (uint16_t)(tagged_version);
}

inline uint32_t make_tagged_version(uint32_t version) noexcept {
    return version + UINT16_MAX + 1;
}

inline bool is_tagged_version(uint32_t version) noexcept {
    return version > UINT16_MAX;
}

enum ConcurrentBoundedQueueOption {
    CACHELINE_ALIGNED = 0x1,
    DEFAULT = 0x0,
}; 

template <typename T, typename F = FutexInterface, uint32_t OPTION = ConcurrentBoundedQueueOption::DEFAULT>
class ConcurrentBoundedQueue {
public:
    class BaseSlotFutex {
    public:
        void wait_until_reach_expected_version(uint16_t expected_version) noexcept {
            uint32_t current_version = _futex.value().load(std::memory_order_relaxed);
            uint16_t current_raw_version = get_raw_version(current_version);
            if (current_raw_version == expected_version) {
                return;
            }

            // version不满足，需要wait
            while (current_raw_version != expected_version) {
                INC_STATS(loop);
                // wait之前先把version标记一下，表明现在有等待者，这样别人版本推进后才会唤醒
                if (!is_tagged_version(current_version)) {
                    uint32_t tagged_version = make_tagged_version(current_version);
                    // 只需要1个线程负责标记就可以了，用CAS操作
                    if (_futex.value().compare_exchange_weak(
                                                current_version, 
                                                tagged_version,
                                                std::memory_order_release,
                                                std::memory_order_acquire)) {
                        INC_STATS(wait);
                        _futex.wait(tagged_version, nullptr);
                    } 
                } else {
                    INC_STATS(wait);
                    _futex.wait(current_version, nullptr);
                }
                // 被唤醒了，看看最新的version是不是我想要的
                current_version = _futex.value().load(std::memory_order_relaxed);
                current_raw_version = get_raw_version(current_version); 
            }
        }

        bool is_ready(uint16_t expected_version) {
            uint32_t current_version = _futex.value().load(std::memory_order_relaxed);
            return (get_raw_version(current_version) == expected_version);
        }

        void advance_version(uint16_t new_version) noexcept {
            _futex.value().exchange((uint32_t)new_version, std::memory_order_relaxed);
        }

        void wakeup_waiters() noexcept {
            uint32_t current_version = _futex.value().load(std::memory_order_relaxed);
            if (!is_tagged_version(current_version)) {
                // 没有等待者就不调用futex
                return;
            } else {
                uint16_t raw_version = get_raw_version(current_version);
                // 只有version被标记了才会唤醒, 被标记了说明有别的线程在等待
                INC_STATS(wake);
                // 把标记了的version改回未标记的, 表明没有人在等了
                _futex.value().compare_exchange_strong(current_version, (uint32_t)raw_version, std::memory_order_relaxed);
                _futex.wake_all();
            }
        }

        void advance_version_and_wakeup_waiters(uint16_t new_version) noexcept {
            // 推进版本
            uint32_t old_version = _futex.value().exchange(new_version, std::memory_order_release);
            if (!is_tagged_version(old_version)) {
                // 没有等待者就不调用futex
                return;
            } else {
                // 只有version被标记了才会唤醒, 被标记了说明有别的线程在等待
                INC_STATS(wake);
                _futex.wake_all();
            }
        }

        void reset() noexcept {
            _futex.value().store(0, std::memory_order_relaxed);
        }
    private:
        Futex<F> _futex {0};
    };
    struct alignas(CACHELINE_SIZE) AlignedSlotFutex : public BaseSlotFutex {
    };
    struct BaseSlot {
        T value;
    };
    struct alignas(CACHELINE_SIZE) CompactedSlot : public BaseSlot {
        BaseSlotFutex futex;
    };
    struct alignas(CACHELINE_SIZE) AlignedSlot : public BaseSlot {
    };
    static constexpr bool CACHELINE_ALIGNED = OPTION & ConcurrentBoundedQueueOption::CACHELINE_ALIGNED;
    // COMPACTED表示是否要利用CacheLine剩余的碎片来存储Futex
    static constexpr bool COMPACTED = CACHELINE_ALIGNED && CACHELINE_SIZE >=  
                                    ((sizeof(T) % CACHELINE_SIZE) + sizeof(Futex<F>));
    using Slot = typename std::conditional_t<COMPACTED, 
                                            CompactedSlot, 
                                            typename std::conditional_t<CACHELINE_ALIGNED, 
                                                                        AlignedSlot, 
                                                                        BaseSlot>>;
    using SlotFutex = typename std::conditional_t<CACHELINE_ALIGNED, AlignedSlotFutex, BaseSlotFutex>;  

    template<bool C>
    class SimpleIterator: public std::iterator<::std::random_access_iterator_tag, T, ssize_t, T*, T&>{
    public:
        SimpleIterator(Slot* slot) : _slot(slot) {}
        ~SimpleIterator() {}

        SimpleIterator operator+(ssize_t offset) const noexcept {
            return SimpleIterator(_slot + offset);
        }
        SimpleIterator operator-(ssize_t offset) const noexcept {
            return SimpleIterator(_slot - offset);
        }
        bool operator<(SimpleIterator other) const noexcept {
            return _slot < other._slot;
        }
        bool operator<=(SimpleIterator other) const noexcept {
            return _slot <= other._slot;
        }
        bool operator>(SimpleIterator other) const noexcept {
            return _slot > other._slot;
        }
        bool operator>=(SimpleIterator other) const noexcept {
            return _slot >= other._slot;
        }
        ssize_t operator-(SimpleIterator other) const noexcept {
            return _slot - other._slot;
        }
        const T& operator*() const { return _slot->value; }
        const T* operator->() const { return &_slot->value; }

        template<bool Z = C, typename = std::enable_if_t<(!Z)>>
        T& operator*() noexcept { return _slot->value; }

        template<bool Z = C, typename = std::enable_if_t<(Z)>>
        const T& operator*() const noexcept { return _slot->value; }

        template<bool Z = C, typename = std::enable_if_t<(!Z)>>
        T* operator->() noexcept { return &_slot->value; }

        template<bool Z = C, typename = std::enable_if_t<(Z)>>
        const T* operator->() const noexcept { return &_slot->value; }
        // 前置操作重载
        const SimpleIterator& operator++() {
            ++_slot;
            return *this;
        }
        // 后置操作重载
        SimpleIterator operator++(int) {
            auto prev = *this;
            ++*this;
            return prev;
        }
        bool operator==(const SimpleIterator& o) const { return _slot == o._slot; }
        bool operator!=(const SimpleIterator& o) const { return !(*this == o); }
        SimpleIterator& operator=(const SimpleIterator& o) {
            SimpleIterator tmp(o);
            swap(tmp);
            return *this;
        }
        SimpleIterator(const SimpleIterator& o) { _slot = o._slot; }
        SimpleIterator(SimpleIterator&& o) { _slot = o._slot; }
        void swap(SimpleIterator& o) { std::swap(_slot, o._slot); }
    private:
        Slot* _slot = {nullptr};
    };
    using Iterator = SimpleIterator<false>;
    using ConstIterator = SimpleIterator<true>;    
private:
    struct SlotVector {
    public:
        SlotVector() = default;

        size_t size() noexcept { return _size; }

        void init(size_t capacity) {
            _size = capacity;
            _slots = reinterpret_cast<Slot*>(::aligned_alloc(alignof(Slot), sizeof(Slot) * _size));
            if constexpr (!COMPACTED) {
                _futexes = reinterpret_cast<SlotFutex*>(::aligned_alloc(alignof(SlotFutex), sizeof(SlotFutex) * _size));
            }
            for (size_t i = 0; i < _size; ++i) {
                new (&_slots[i]) Slot();
                if constexpr (!COMPACTED) {
                    new (&_futexes[i]) SlotFutex();
                }
            }
        }

        void resize(size_t capacity) noexcept {
            clear();
            init(capacity);
        }

        ~SlotVector() noexcept { clear(); }
        Iterator at(size_t i) noexcept { return Iterator(_slots + i); }
        Iterator begin() noexcept { return Iterator(_slots); }
        Iterator end() noexcept { return Iterator(_slots + _size); }
        ConstIterator cbegin() noexcept { return ConstIterator(_slots); }
        ConstIterator cend() noexcept { return ConstIterator(_slots + _size); }

        void clear() noexcept {
            for (size_t i = 0; i < _size; ++i) {
                _slots[i].~Slot();
                if constexpr (!COMPACTED) {
                    _futexes[i].~SlotFutex();
                }
            }
            if (_slots) {
                free(_slots);
                _slots = nullptr;
            }
            if constexpr (!COMPACTED) {
                if (_futexes) {
                    free(_futexes);
                    _futexes = nullptr;
                }
            }
            _size = 0;        
        }

        BaseSlotFutex& get_futex(size_t slot_id) noexcept {
            if constexpr (COMPACTED) {
                INC_STATS(futex_compacted);
                return _slots[slot_id].futex;
            } else {
                INC_STATS(futex_no_compacted);
                return _futexes[slot_id];
            }
        }

        Slot& get_slot(size_t i) noexcept { return _slots[i]; }
    private:
        Slot* _slots = nullptr;
        SlotFutex* _futexes = nullptr;
        size_t _size = 0;
    };                                                                        
public:
    ConcurrentBoundedQueue() noexcept {}
    explicit ConcurrentBoundedQueue(size_t capacity) noexcept {
        reserve_and_clear(capacity);
    }
    // 禁止拷贝和移动
    ConcurrentBoundedQueue(ConcurrentBoundedQueue&&) = delete;
    ConcurrentBoundedQueue(const ConcurrentBoundedQueue&) = delete;
    ConcurrentBoundedQueue& operator=(ConcurrentBoundedQueue&&) = delete;
    ConcurrentBoundedQueue& operator=(const ConcurrentBoundedQueue&) = delete;

    void reserve_and_clear(size_t min_capacity) noexcept {
        auto new_capacity = nextPowTwo(min_capacity);
        if (new_capacity != capacity()) {
            _round_bits = __builtin_popcount(new_capacity - 1) ;
            _capacity = new_capacity;
            _slot_vec.resize(new_capacity);
            _next_tail_index.store(0, std::memory_order_relaxed);
            _next_head_index.store(0, std::memory_order_relaxed);
            for (size_t i = 0; i < new_capacity; ++i) {
                _slot_vec.get_futex(i).reset();
            }
        } else {
            clear();
        }
    }

    size_t capacity() noexcept { return _capacity; }

    void clear() noexcept {
        while (size() > 0) {
            pop([](T& v){});
        }
    }

    size_t size() noexcept {
        auto head_index = _next_head_index.load(std::memory_order_relaxed);
        auto tail_index = _next_tail_index.load(std::memory_order_relaxed);
        return tail_index > head_index ? tail_index - head_index : 0;
    }

    uint16_t push_version_for_index(size_t index) noexcept {
        return (index >> _round_bits) << 1;
    }

    uint16_t pop_version_for_index(size_t index) noexcept {
        return push_version_for_index(index) + 1;
    }

    // 处理单个元素
    template<bool IS_PUSH, typename C>
    void process(size_t index, C&& callback) noexcept {
        uint16_t expected_version = 0;
        if constexpr (IS_PUSH) {
            expected_version = push_version_for_index(index);
        } else {
            expected_version = pop_version_for_index(index);
        }
        uint32_t slot_id = index & (_capacity - 1); // index % _capacity
        Slot& slot = _slot_vec.get_slot(slot_id);
        BaseSlotFutex& slot_futex = _slot_vec.get_futex(slot_id);

        slot_futex.wait_until_reach_expected_version(expected_version);

        std::atomic_thread_fence(::std::memory_order_acquire);
        callback(slot.value);

        slot_futex.advance_version_and_wakeup_waiters(expected_version + 1);
    }

    void push(const T& value) noexcept {
        push([&](T& v) __attribute__((always_inline)) {
            v = value;
        });
    }

    void push(T&& value) noexcept {
        push([&](T& v) __attribute__((always_inline)) {
            v = std::move(value);
        });
    }

    template<typename C, typename = std::enable_if_t<std::is_invocable_v<C, T&>>>
    void push(C&& callback) noexcept {
        size_t index = _next_tail_index.fetch_add(1, std::memory_order_relaxed);
        process<true>(index, std::forward<C>(callback));
    }

    // 批量处理n个元素, 生成一个index在区间位于[index, index+num]对应的起始、结束迭代器，然后回调这个区间
    // 调用者需要确保这n个元素都在相同的轮次，相当于[index, index+num]区间内的轮次都一样
    // 这样expected_version都是一样的
    template<bool IS_PUSH, typename C>
    void process_n(C&& callback, size_t index, size_t num) {
        uint16_t expected_version = 0;
        if constexpr (IS_PUSH) {
            expected_version = push_version_for_index(index);
        } else {
            expected_version = pop_version_for_index(index);
        }

        uint32_t slot_id = index & (_capacity - 1); // index % _capacity
        for (size_t i = 0; i < num; ++i) {
            BaseSlotFutex& slot_futex = _slot_vec.get_futex(slot_id+i);
            slot_futex.wait_until_reach_expected_version(expected_version);
        }

        std::atomic_thread_fence(::std::memory_order_acquire);
        callback(_slot_vec.at(slot_id), _slot_vec.at(slot_id + num));
        std::atomic_thread_fence(::std::memory_order_release);

        for (size_t i = 0; i < num; ++i) {
            BaseSlotFutex& slot_futex = _slot_vec.get_futex(slot_id+i);
            slot_futex.advance_version(expected_version+1);
        }

        std::atomic_thread_fence(::std::memory_order_seq_cst);

        for (size_t i = 0; i < num; ++i) {
            BaseSlotFutex& slot_futex = _slot_vec.get_futex(slot_id+i);
            slot_futex.wakeup_waiters();
        }
    }

    template<bool IS_PUSH, typename C>
    void try_process_n(C&& callback, size_t index, size_t num) {
        uint16_t expected_version = 0;
        if constexpr (IS_PUSH) {
            expected_version = push_version_for_index(index);
        } else {
            expected_version = pop_version_for_index(index);
        }

        uint32_t slot_id = index & (_capacity - 1); // index % _capacity
        // 版本一样说明条件ready，不一样说明没有ready，找到第1个没有ready，下标就是ready了的数量
        for (size_t i = 0; i < num; ++i) {
            BaseSlotFutex& slot_futex = _slot_vec.get_futex(slot_id + i);
            if (!slot_futex.is_ready(expected_version)) {
                num = i;
                break;
            }
        }
        if (num == 0) {
            return;
        }

        if constexpr (IS_PUSH) {
            if(!_next_tail_index.compare_exchange_strong(index, 
                                                         index + num, 
                                                         std::memory_order_relaxed)) {
                // 抢占位置失败，直接返回，这次操作失败，让调用者负责重试
                return;
            }
        } else {
            if(!_next_head_index.compare_exchange_strong(index, 
                                                        index + num, 
                                                        std::memory_order_relaxed)) {
                // 抢占位置失败，直接返回，这次操作失败，让调用者负责重试
                return;
            }
        }


        std::atomic_thread_fence(::std::memory_order_acquire);
        callback(_slot_vec.at(slot_id), _slot_vec.at(slot_id + num));
        std::atomic_thread_fence(::std::memory_order_release);

        for (size_t i = 0; i < num; ++i) {
            BaseSlotFutex& slot_futex = _slot_vec.get_futex(slot_id + i);
            slot_futex.advance_version(expected_version+1);
        }

        std::atomic_thread_fence(::std::memory_order_seq_cst);

        for (size_t i = 0; i < num; ++i) {
            BaseSlotFutex& slot_futex = _slot_vec.get_futex(slot_id + i);
            slot_futex.wakeup_waiters();
        }
    }
      

    // 把迭代器[begin, end)之间的元素push到队列
    // 注意begin和end之间的元素数量有可能超出队列的capacity
     // 如果超过只拷贝前面一部分元素
    template<typename IT>
    void push_n(IT begin, IT end) {
        auto num_of_push = std::distance(begin, end);
        auto num = num_of_push > _capacity ? _capacity : num_of_push;

        auto callback = [&](Iterator dest_begin, Iterator dest_end) {
            auto dest_num = std::distance(dest_begin, dest_end);
            std::copy(begin, begin + dest_num, dest_begin);
            begin += dest_num;
        };

        process_n_by_round<true>(callback, num);
    }

    template<typename IT>
    void try_push_n(IT begin, IT end) {
        auto num_of_push = std::distance(begin, end);
        auto num = num_of_push > _capacity ? _capacity : num_of_push;

        auto callback = [&](Iterator dest_begin, Iterator dest_end) {
            auto dest_num = std::distance(dest_begin, dest_end);
            std::copy(begin, begin + dest_num, dest_begin);
            begin += dest_num;
        };

        try_process_n_by_round<true>(callback, num);
    }    

    template<typename C>
    void pop_n(C&& callback, size_t num) {
        if (num > _capacity) {
            num = _capacity;
        }
        process_n_by_round<false>(callback, num);
    }

    template<typename C>
    void try_pop_n(C&& callback, size_t num) {
        if (num > _capacity) {
            num = _capacity;
        }
        try_process_n_by_round<false>(callback, num);
    }    

    // 分轮次批量处理元素并回调
    // 调用者保证待插入的元素数量不超过队列容量
    template<bool IS_PUSH, typename C>
    void process_n_by_round(C&& callback, size_t num) {
        //DCHECK(num <= _capacity);
        size_t index = 0;
        if constexpr(IS_PUSH) {
            index = _next_tail_index.fetch_add(num, std::memory_order_relaxed);
        } else {
            index = _next_head_index.fetch_add(num, std::memory_order_relaxed);
        }

        // 算法：(round+1)*capacity = (index/capacity+1)*capaticy
        size_t next_round_begin_index = ((index >> _round_bits) + 1) << _round_bits;

        if ((index + num) < next_round_begin_index) {
            std::cout << "process_n no need split, index:" << index
                      << " num:" << num
                      << " next_round_begin_index:" << next_round_begin_index
                      << std::endl;
            process_n<IS_PUSH>(std::forward<C>(callback), index, num);
        } else {
            auto first_size = next_round_begin_index - index;
            auto second_size = num - first_size;
            std::cout << "process_n need split, index:" << index
                      << " num:" << num
                      << " next_round_begin_index:" << next_round_begin_index
                      << " first_size:" << first_size
                      << " second_size:" << second_size
                      << std::endl;            
            process_n<IS_PUSH>(std::forward<C>(callback), index, first_size);
            process_n<IS_PUSH>(std::forward<C>(callback), next_round_begin_index, second_size);
        }
    }

    // 分轮次批量处理元素并回调，非阻塞版本
    // 调用者保证待插入的元素数量不超过队列容量
    template<bool IS_PUSH, typename C>
    void try_process_n_by_round(C&& callback, size_t num) {
        //DCHECK(num <= _capacity);
        size_t index = 0;
        if constexpr(IS_PUSH) {
            index = _next_tail_index.load(std::memory_order_relaxed);
        } else {
            index = _next_head_index.load(std::memory_order_relaxed);
        }

        // 算法：(round+1)*capacity = (index/capacity+1)*capaticy
        size_t next_round_begin_index = ((index >> _round_bits) + 1) << _round_bits;

        if ((index + num) < next_round_begin_index) {
            std::cout << "try_process_n no need split, index:" << index
                      << " num:" << num
                      << " next_round_begin_index:" << next_round_begin_index
                      << std::endl;
            try_process_n<IS_PUSH>(std::forward<C>(callback), index, num);
        } else {
            auto first_size = next_round_begin_index - index;
            auto second_size = num - first_size;
            std::cout << "try_process_n need split, index:" << index
                      << " num:" << num
                      << " next_round_begin_index:" << next_round_begin_index
                      << " first_size:" << first_size
                      << " second_size:" << second_size
                      << std::endl;            
            try_process_n<IS_PUSH>(std::forward<C>(callback), index, first_size);
            try_process_n<IS_PUSH>(std::forward<C>(callback), next_round_begin_index, second_size);
        }
    }

    void pop(T& value) noexcept {
        pop([&](T& v) __attribute__((always_inline)) {
            value = std::move(v);
        });
    }

    void pop(T* value) noexcept {
        pop([&](T& v) __attribute__((always_inline)) {
            value = &v;
        });
    }

    void close() {

    }

    template<typename C, typename = std::enable_if_t<std::is_invocable_v<C, T&>>>
    void pop(C&& callback) noexcept {
        size_t index = _next_head_index.fetch_add(1, std::memory_order_relaxed);
        process<false>(index, std::forward<C>(callback));
    }

    void print(std::string label = "") {
        uint32_t tail_slot_id = _next_tail_index.load() & (_capacity - 1);
        uint32_t head_slot_id = _next_head_index.load() & (_capacity - 1);
        Slot& pop_slot = _slot_vec.get_slot(head_slot_id);
        Slot& push_slot = _slot_vec.get_slot(tail_slot_id);
        std::cout << "---------------- print queue " << label << "--------------------\n";
        std::cout << "_next_head_index:" << _next_head_index.load()
                   << " _next_tail_index:" << _next_tail_index.load()
                   << " head_expected_version:" << pop_version_for_index(_next_head_index.load())
                   << " tail_expected_version:" << push_version_for_index(_next_tail_index.load())
                   << " head_slot_id:" << head_slot_id
                   << " tail_slot_id:" << tail_slot_id
                   << " head_value:" << pop_slot.value
                   << " tail_value:" << push_slot.value
                   << std::endl;
        for (int slot_id = 0; slot_id < _capacity; ++slot_id) {
            std::cout << "slot_id:" << slot_id
                      << " slot_value:" << _slot_vec.get_slot(slot_id).value
                      << std::endl;
        }
        std::cout << "------------------------------------\n\n";
    } 
private:
    SlotVector _slot_vec; // 物理槽位
    size_t _capacity = 0;
    size_t _round_bits = 0;
    alignas(CACHELINE_SIZE) std::atomic<size_t> _next_tail_index = ATOMIC_VAR_INIT(0);
    alignas(CACHELINE_SIZE) std::atomic<size_t> _next_head_index = ATOMIC_VAR_INIT(0);
};

} // namespace
