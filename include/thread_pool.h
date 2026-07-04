#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>
#include <stdexcept>

class ThreadPool {
private:
    std::vector<std::thread> workers;        // Worker threads
    std::queue<std::function<void()>> tasks; // Task queue
    std::mutex queue_mutex;                   // Protect task queue
    std::condition_variable condition;       // Notify workers
    std::atomic<bool> stop;                  // Shutdown flag
    
public:
    // Constructor: Create worker threads
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    
    // Destructor: Clean up
    ~ThreadPool();
    
    // Add a task to the queue
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type>;
    
    // Get number of worker threads
    size_t worker_count() const { return workers.size(); }
    
    // Check if pool is still running
    bool is_running() const { return !stop.load(); }
};

// Template implementation - must be in header
template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result<F, Args...>::type> 
{
    using return_type = typename std::invoke_result<F, Args...>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop.load()) {
            throw std::runtime_error("ThreadPool stopped");
        }
        tasks.emplace([task]() { (*task)(); });
    }
    
    condition.notify_one();
    return result;
}

#endif // THREAD_POOL_H