#ifndef __RINGBUFFER_HPP__
#define __RINGBUFFER_HPP__

template<class T>
struct RingBuffer
{
  explicit RingBuffer(int capacity)
    : _capacity(capacity), _nodes(capacity)
  {
  }
  void push(T const & t)
  {
    {
      std::unique_lock<std::mutex> lk(_mtx);
      _cv_full.wait(lk, [this]{ return _cnt < _capacity; });
      _head %= _capacity;
      _nodes[_head++] = t;
      _cnt++;
    }
    _cv_zero.notify_one();
  }

  T pop()
  {
    T t;
    {
      std::unique_lock<std::mutex> lk(_mtx);
      _cv_zero.wait(lk, [this]{ return _cnt > 0; });
      _tail %= _capacity;
      t = _nodes[_tail++];
      _cnt--;
    }
    _cv_full.notify_one();
    return t;
  }

  int capacity() { return _capacity; }
  int size() { return _cnt; }
  
private:
  int _head = 0;
  int _tail = 0;
  int _cnt = 0;
  std::vector<T> _nodes;
  int _capacity;
  std::mutex _mtx;
  std::condition_variable _cv_full;
  std::condition_variable _cv_zero;
};


template<class T>
void dot(const T & t)
{
  std::cout << t << std::flush;
}

template<class T>
struct RingBuffer2
{
  explicit RingBuffer2(int capacity = 32)
    : _capacity(capacity), _nodes(capacity)
  {
  }
  void push(T const & t)
  {
    {
      std::unique_lock<std::mutex> lk(_mtx);
      if(_cnt == _capacity)
	expand();
      
      _tail %= _capacity;
      _nodes[_tail++] = t;
      _cnt++;
    }
    _cv_zero.notify_one();
  }

  T pop()
  {
    std::unique_lock<std::mutex> lk(_mtx);
    if(_cnt <= _capacity/4)
      shrink();
    _cv_zero.wait(lk, [this]{ return _cnt > 0; });

    _head %= _capacity;
    auto t = _nodes[_head++];
    _cnt--;
    return t;
  }

  void dump()
  {
    std::cout << "ringbuffer dump: \n";
    std::cout << "  capacity: " << _capacity << "\n";
    std::cout << "  size:     " << _cnt << "\n";
  }
  
  int capacity() { return _capacity; }
  int size() { return _cnt; }
  
private:
  void expand()
  {
    std::cout << "ringbuffer expend " << _capacity << "\n";
    
    std::vector<T> nodes(_capacity*2);
    
    for(int i=0; i<_cnt; ++i)
      nodes[i] = _nodes[(_tail+i)%_capacity];

    _nodes = nodes;
    _capacity *= 2;
    _head = 0;
    _tail = _cnt;
  }

  void shrink()
  {
    if(_capacity <= 32)
      return;

    std::cout << "ringbuffer shrink " << _capacity << "\n";
    
    std::vector<T> nodes(_capacity/2);

    for(int i=0; i<_cnt; ++i)
      nodes[i] = _nodes[(_tail+i)%_capacity];

    _nodes = nodes;
    _capacity /= 2;
    _head = 0;
    _tail = _cnt;
  }
  
  int _head = 0;
  int _tail = 0;
  int _cnt = 0;
  std::vector<T> _nodes;
  int _capacity;
  std::mutex _mtx;
  std::condition_variable _cv_zero;
};



#endif
