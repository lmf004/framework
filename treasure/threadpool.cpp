#include "headers.hpp"

int max_thread_count = 10;
std::atomic<int> ThreadPool::thread_count{0};
std::atomic<int> ThreadPool::free_thread_count{0};
QueueThreadSafe<ThreadPool*> ThreadPool::free_threads;

