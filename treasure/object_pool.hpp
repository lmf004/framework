#ifndef __TREASURE_OBJECT_POOL_HPP__
#define __TREASURE_OBJECT_POOL_HPP__

template<class T>
struct allocator{
  template<class... Args>
  boost::shared_ptr<T> allocate(Args &... args){
    boost::lock_guard<boost::mutex> guard(mtx_);
    return boost::shared_ptr<T>(allocator_.construct(args...), boost::bind(&allocator<T>::destroy, this, _1));
  }
  void destroy(T* p){ 
    boost::lock_guard<boost::mutex> guard(mtx_);
    BOOST_ASSERT(allocator_.is_from(p));
    allocator_.destroy(p); 
  }
  boost::object_pool<T> allocator_;
  boost::mutex mtx_;
};

#endif
