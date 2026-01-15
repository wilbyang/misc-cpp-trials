#include <coroutine>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <exception>
#include <memory>
#include <chrono>

// Thread pool for async execution
class ThreadPool {
public:
    explicit ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers)
            worker.join();
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// Task<T> coroutine type
template<typename T>
struct Task {
    struct promise_type {
        T result;
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_value(T value) {
            result = std::move(value);
        }

        void unhandled_exception() {
            exception = std::current_exception();
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    Task(handle_type h) : coro(h) {}
    
    Task(Task&& t) noexcept : coro(t.coro) { t.coro = nullptr; }
    
    ~Task() { 
        if (coro) coro.destroy(); 
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // Make Task awaitable
    bool await_ready() const noexcept { 
        return false; 
    }

    void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        coro.promise().continuation = awaiting;
        coro.resume();
    }

    T await_resume() {
        if (coro.promise().exception) {
            std::rethrow_exception(coro.promise().exception);
        }
        return std::move(coro.promise().result);
    }

    T get() {
        coro.resume();
        if (coro.promise().exception) {
            std::rethrow_exception(coro.promise().exception);
        }
        return std::move(coro.promise().result);
    }
};

// Specialization for Task<void>
template<>
struct Task<void> {
    struct promise_type {
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_void() {}

        void unhandled_exception() {
            exception = std::current_exception();
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    Task(handle_type h) : coro(h) {}
    
    Task(Task&& t) noexcept : coro(t.coro) { t.coro = nullptr; }
    
    ~Task() { 
        if (coro) coro.destroy(); 
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    bool await_ready() const noexcept { 
        return false; 
    }

    void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        coro.promise().continuation = awaiting;
        coro.resume();
    }

    void await_resume() {
        if (coro.promise().exception) {
            std::rethrow_exception(coro.promise().exception);
        }
    }

    void get() {
        coro.resume();
        if (coro.promise().exception) {
            std::rethrow_exception(coro.promise().exception);
        }
    }
};

// Async file read operation
Task<std::string> async_read_file(const std::string& filename, ThreadPool& pool) {
    struct Awaiter {
        std::string filename;
        ThreadPool& pool;
        std::string result;
        std::exception_ptr exception;
        std::coroutine_handle<> awaiting_coroutine;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> h) {
            awaiting_coroutine = h;
            pool.enqueue([this]() {
                try {
                    std::ifstream file(filename);
                    if (!file.is_open()) {
                        throw std::runtime_error("Failed to open file: " + filename);
                    }
                    
                    std::string content((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
                    result = std::move(content);
                    
                    // Simulate some processing time
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                } catch (...) {
                    exception = std::current_exception();
                }
                
                // Resume the coroutine
                awaiting_coroutine.resume();
            });
        }

        std::string await_resume() {
            if (exception) {
                std::rethrow_exception(exception);
            }
            return std::move(result);
        }
    };

    co_return co_await Awaiter{filename, pool, "", nullptr, nullptr};
}

// Main coroutine coordinating multiple reads
Task<void> read_multiple_files(ThreadPool& pool) {
    std::cout << "Starting concurrent file reads...\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    // Read files concurrently using co_await
    auto content1 = co_await async_read_file("test_file1.txt", pool);
    std::cout << "File 1 content:\n" << content1 << "\n\n";

    auto content2 = co_await async_read_file("test_file2.txt", pool);
    std::cout << "File 2 content:\n" << content2 << "\n\n";

    auto content3 = co_await async_read_file("test_file3.txt", pool);
    std::cout << "File 3 content:\n" << content3 << "\n\n";

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "All files read successfully!\n";
    std::cout << "Total time: " << duration.count() << "ms\n";

    co_return;
}

// Helper to create test files
void create_test_files() {
    {
        std::ofstream f1("test_file1.txt");
        f1 << "This is test file 1\n";
        f1 << "It contains some sample text.\n";
        f1 << "Line 3 of file 1.";
    }
    {
        std::ofstream f2("test_file2.txt");
        f2 << "Test file 2 here!\n";
        f2 << "Second line of file 2.\n";
        f2 << "More content in file 2.";
    }
    {
        std::ofstream f3("test_file3.txt");
        f3 << "File 3 content:\n";
        f3 << "Different text in this file.\n";
        f3 << "Last line of file 3.";
    }
}

int main() {
    std::cout << "C++20 Coroutine Async File Reading Demo\n";
    std::cout << "========================================\n\n";

    // Create test files
    create_test_files();
    std::cout << "Created test files: test_file1.txt, test_file2.txt, test_file3.txt\n\n";

    // Create thread pool with 4 workers
    ThreadPool pool(4);

    try {
        // Run the coroutine
        auto task = read_multiple_files(pool);
        task.get();  // Start and wait for completion

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nDemo completed successfully!\n";
    return 0;
}
