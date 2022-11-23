namespace gflow {

template <typename T>
GraphData *GraphData::make() {
    //LOG(TRACE) << "GraphData make " << _name;
    if (unlikely(_any_data.get<T>() == nullptr)) {
        if constexpr (std::is_copy_constructible_v<T>) {
            _any_data = Any(T());
        } else {
            _any_data = Any(std::make_unique<T>());
        }
    }
    return this;
}

template <typename T>
T &GraphData::raw() {
    //LOG(TRACE) << "raw:" << _name;
    return *pointer<T>();
    //return std::any_cast<T &>(_any_data);
}

template <typename T>
T GraphData::as() {
    return _any_data.as<T>();
}

template <typename T>
T* GraphData::pointer() {
    T* p = _any_data.get<T>();
    //LOG(TRACE) << "pointer:" << _name << " ptr:" << p;
    return p;
}

// template <typename T>
// const T* GraphData::pointer() {
//     return _any_data.get<T>();
// }

template <typename T>
void GraphData::set_value(T&& value) {
    // LOG(TRACE) << "raw:" << _name;
    //_any_data = std::move(value);
    _any_data = Any(std::forward<T>(value));
}

template <typename T>
Commiter<T> GraphData::commiter() {
    return Commiter<T>(this);
}
template <typename T>
void GraphData::emit_value(T&& value) {
    set_value(std::forward<T>(value));
    release();
}

template <typename T>
Commiter<T>::Commiter(Commiter &&o) {
    this._data = o._data;
    o._data = nullptr;
}

template <typename T>
Commiter<T>& Commiter<T>::operator=(Commiter &&o) {
    ::std::swap(_data, o._data);
    o.release();
    return *this;
}

template <typename T>
Commiter<T>::Commiter(GraphData* data) : _data(data) {
    _data->make<T>();
}

template <typename T>
Commiter<T>::~Commiter() {
    commit();
    _data = nullptr;
}

template <typename T>
T& Commiter<T>::operator*() noexcept {
    DCHECK(_data);
    return _data->raw<T>();
}

template <typename T>
T* Commiter<T>::operator->() noexcept {
    DCHECK(_data);
    return _data->pointer<T>();
}

template <typename T>
void Commiter<T>::set_value(T&& value) noexcept {
    DCHECK(_data);
    _data->set_value(std::forward<T>(value));
}

template <typename T>
void Commiter<T>::commit() noexcept {
    if (_data && _is_valid) { 
        LOG(TRACE) << "Commiter commit data:" << _data->get_name();
        _data->release();
        _is_valid = false;
    }
}


}  // namespace
