#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
public:
    void Push(const T& new_data)
    {
        auto data_ptr = std::make_shared<T>(new_data);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(data_ptr);
        m_cond_var.notify_one();
    }

    std::shared_ptr<T> WaitAndPop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_var.wait(lock, [this]() { return !m_queue.empty(); });
        auto data_ptr = m_queue.front();
        m_queue.pop();

        return data_ptr;
    }

    bool IsEmpty() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.is_empty();
    }

private:
    std::queue<std::shared_ptr<T>> m_queue {};
    mutable std::mutex m_mutex;
    std::condition_variable m_cond_var;
};
