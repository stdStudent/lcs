//
// Created by Labour on 20.01.2024.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector> // for vector
#include <queue> // for queue
#include <mutex> // for mutex
#include <condition_variable> // for condition_variable
#include <sys/epoll.h> // for epoll
#include <memory> // for shared_ptr
#include <future> // for future
#include <functional> // for bind

class ThreadPool {
public:
    using CallbackFunction = std::function<void(int)>;
    void registerCallback(const int fd, const CallbackFunction &callback) {
        callbacks_[fd] = callback;
    }

public:
    explicit ThreadPool(const size_t num_threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < num_threads; ++i) {
            epfds_.push_back(epoll_create1(0));
            if (epfds_.back() == -1) {
                perror("epoll_create1");
                throw std::runtime_error("Failed to create epoll instance");
            }

            eventThreads_.emplace_back([this, i] {
                while (true) {
                    epoll_event ev{};
                    if (const int ret = epoll_wait(epfds_[i], &ev, 1, -1); ret == -1) {
                        perror("epoll_wait");
                        break;
                    }

                    if (ev.events & EPOLLIN) {
                        // Handle incoming data
                        auto it = callbacks_.find(ev.data.fd);
                        if (it != callbacks_.end()) {
                            it->second(ev.data.fd);
                        }
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& thread : eventThreads_) {
            thread.join();
        }
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<std::result_of_t<F(Args...)>> {
        using return_type = std::result_of_t<F(Args...)>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task]{ (*task)(); });
        }
        cv_.notify_one();
        return res;
    }

    // Add a new socket to the epoll instance
    void addSocket(const int fd) const {
        // Choose an epoll instance round-robin fashion
        const int epfd = epfds_[fd % epfds_.size()];

        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            perror("epoll_ctl");
            throw std::runtime_error("Failed to add socket to epoll instance");
        }
    }

private:
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_ = false;

    std::map<int, CallbackFunction> callbacks_;
    std::vector<int> epfds_;
    std::vector<std::thread> eventThreads_;
};

#endif //THREADPOOL_H
