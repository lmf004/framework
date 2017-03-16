#ifndef __TREASURE_THREAD_HPP__
#define __TREASURE_THREAD_HPP__

template<typename Derived>
struct Thread
{
  template<typename... Args>
  std::future<void> start(std::shared_ptr<bool> running, Args&&... args){
    _running = running;
    Derived * d = static_cast<Derived *>(this);
    return std::async(std::launch::async, &Derived::run, d, running, args...);
  }
  void stop(){ if(_running) *_running = false; } 
  shared_ptr<bool> _running;
};

//-------------------------------------------------------------------------------------------------------------
struct read_write_lock
{
  read_write_lock(){ g_rw_lock_init (&_lock); }
  ~read_write_lock(){ g_rw_lock_clear (&_lock); }
  GRWLock _lock;
};

struct read_lock_guard
{
  read_lock_guard(read_write_lock & lock) 
    : _lock(lock) { g_rw_lock_reader_lock (&(_lock._lock)); }
  ~read_lock_guard(){ g_rw_lock_reader_unlock (&(_lock._lock)); }
  read_write_lock & _lock;
};

struct write_lock_guard
{
  write_lock_guard(read_write_lock & lock) 
    : _lock(lock) { g_rw_lock_writer_lock (&(_lock._lock)); }
  ~write_lock_guard(){ g_rw_lock_writer_unlock (&(_lock._lock)); }
  read_write_lock & _lock;
};

//-------------------------------------------------------------------------------------------------------------


#endif
