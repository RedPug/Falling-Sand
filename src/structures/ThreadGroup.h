#ifndef THREADGROUP_H
#define THREADGROUP_H

#include <thread>
#include <vector>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <future>

template<typename TaskData>
class ThreadGroup {
private:
    std::vector<std::thread> threads;
    std::vector<TaskData> thread_data;
    std::vector<std::atomic<bool>> thread_has_task;
    
    std::function<void(TaskData&, int thread_id)> current_function;
    std::atomic<bool> should_terminate{false};
    std::atomic<int> active_threads{0};
    std::atomic<int> completed_threads{0};
    
    int num_threads;
    bool initialized = false;

    void workerLoop(int thread_id);

public:
    ThreadGroup() = default;
    
    ~ThreadGroup() {
        terminate();
    }

    // Initialize with specified number of threads
    void initializeThreads(int thread_count);

    // Set the function that all threads will execute
    void setFunction(std::function<void(TaskData&, int thread_id)> func);

    // Give each thread their specific task data
    void setThreadData(int thread_id, const TaskData& data);

    // Set data for all threads at once
    void setAllThreadData(const std::vector<TaskData>& data);

    // Start execution on all threads (replaces existing tasks if not complete)
    void executeThreads();

    // Wait for all threads to complete their current tasks
    void waitForCompletion();

    // Execute and wait (convenience function)
    void executeAndWait();

    // Check if all threads have completed
    bool isComplete() const;

    // Get number of active threads
    int getActiveThreadCount() const;

    // Get number of completed threads
    int getCompletedThreadCount() const;

    // Access thread data (read-only while threads are running)
    const TaskData& getThreadData(int thread_id) const;

    // Terminate all threads
    void terminate();

    // Get number of threads
    int getThreadCount() const;
};

template<typename TaskData>
void ThreadGroup<TaskData>::workerLoop(int thread_id) {
    while (!should_terminate) {
        while (!thread_has_task[thread_id].load() && !should_terminate.load()) {
            std::this_thread::yield();  // Give CPU to other threads
        }
        
        if (should_terminate) break;
        
        if (thread_has_task[thread_id].exchange(false)) {
            current_function(thread_data[thread_id], thread_id);
            completed_threads++;
        }
    }
}


// Initialize with specified number of threads
template<typename TaskData>
void ThreadGroup<TaskData>::initializeThreads(int thread_count) {
    if (initialized) {
        terminate(); // Clean up existing threads
    }
    
    num_threads = thread_count;
    threads.reserve(num_threads);
    thread_has_task = std::vector<std::atomic<bool>>(num_threads);
    thread_data.resize(num_threads);
    
    should_terminate = false;
    active_threads = 0;
    
    // Start worker threads
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(&ThreadGroup::workerLoop, this, i);
    }
    
    initialized = true;
}

// Set the function that all threads will execute
template<typename TaskData>
void ThreadGroup<TaskData>::setFunction(std::function<void(TaskData&, int thread_id)> func) {
    current_function = func;
}

// Give each thread their specific task data
template<typename TaskData>
void ThreadGroup<TaskData>::setThreadData(int thread_id, const TaskData& data) {
    if (thread_id >= 0 && thread_id < num_threads) {
        thread_data[thread_id] = data;
    }
}

// Set data for all threads at once
template<typename TaskData>
void ThreadGroup<TaskData>::setAllThreadData(const std::vector<TaskData>& data) {
    for (int i = 0; i < std::min((int)data.size(), num_threads); i++) {
        setThreadData(i, data[i]);
    }
}

// Start execution on all threads (replaces existing tasks if not complete)
template<typename TaskData>
void ThreadGroup<TaskData>::executeThreads() {
    completed_threads = 0;

    // Set all tasks
    for (int i = 0; i < num_threads; i++) {
        thread_has_task[i] = true;
    }
}

// Wait for all threads to complete their current tasks
template<typename TaskData>
void ThreadGroup<TaskData>::waitForCompletion() {
    while (completed_threads.load() < num_threads) {
        std::this_thread::yield();
    }
}

// Execute and wait (convenience function)
template<typename TaskData>
void ThreadGroup<TaskData>::executeAndWait() {
    executeThreads();
    waitForCompletion();
}

// Check if all threads have completed
template<typename TaskData>
bool ThreadGroup<TaskData>::isComplete() const {
    return completed_threads.load() == num_threads;
}

// Get number of active threads
template<typename TaskData>
int ThreadGroup<TaskData>::getActiveThreadCount() const {
    return active_threads;
}

// Get number of completed threads
template<typename TaskData>
int ThreadGroup<TaskData>::getCompletedThreadCount() const {
    return completed_threads.load();
}

// Access thread data (read-only while threads are running)
template<typename TaskData>
const TaskData& ThreadGroup<TaskData>::getThreadData(int thread_id) const {
    return thread_data[thread_id];
}

// Terminate all threads
template<typename TaskData>
void ThreadGroup<TaskData>::terminate() {
    if (!initialized) return;
    
    should_terminate = true;
    
    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    threads.clear();
    thread_has_task.clear();
    thread_data.clear();
    
    initialized = false;
}

// Get number of threads
template<typename TaskData>
int ThreadGroup<TaskData>::getThreadCount() const {
    return num_threads;
}

#endif // THREADGROUP_H