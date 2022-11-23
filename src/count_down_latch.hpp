
namespace gflow {

template<typename M, typename C>
inline CountDownLatch<M, C>::CountDownLatch(uint32_t count) {
   this->count = count; 
}

template<typename M, typename C>
inline void CountDownLatch<M, C>::await() {
    std::unique_lock<M> lck(lock);
    if (0 == count){
        return;
    }
    cv.wait(lck);  
}

template<typename M, typename C>
inline uint32_t CountDownLatch<M,C>::get_count() {
    std::unique_lock<M> lck(lock);
    return count;
}

template<typename M, typename C>
inline void CountDownLatch<M,C>::notify_all() {
    std::unique_lock<M> lck(lock);
    cv.notify_all();
}

template<typename M, typename C>
inline void CountDownLatch<M,C>::add_count(uint32_t count) {
    std::unique_lock<M> lck(lock);
    this->count += count;
}

template<typename M, typename C>
inline void CountDownLatch<M, C>::count_down() {
    std::unique_lock<M> lck(lock);
    if (0 == count) {
        return;
    } 
    --count;
    if (0 == count) {
        cv.notify_all();
    }
}

} // namespace
