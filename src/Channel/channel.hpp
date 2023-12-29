#pragma once

#include <tuple>
#include <deque>
#include <mutex>
#include <memory>
#include <optional>
#include <condition_variable>

namespace mpsc {

template <typename T>
struct Inner {
    std::deque<T> queue;
    std::mutex mutex;
    std::condition_variable available;
    unsigned senders = 1;
};

template <typename T>
class Sender {
public:
    Sender(std::shared_ptr<Inner<T>> inner) : inner_{std::move(inner)} {}

    Sender& operator=(const Sender& other) {
        std::lock_guard lock(other.inner_->mutex);
        other.inner_->senders++;
        this->inner_ = other.inner_;
        return *this;
    }

    Sender(const Sender& other) {
        std::lock_guard lock(other.inner_->mutex);
        other.inner_->senders++;
        this->inner_ = other.inner_;
    }

    ~Sender() {
        if (this->inner_) {
            std::lock_guard lock(this->inner_->mutex);

            this->inner_->senders--;

            if (this->inner_->senders == 0) {
                this->inner_->available.notify_one();
            }
        }
    }

    Sender& operator=(Sender&&) = default;
    Sender(Sender&&) = default;

    void send(const T& data) {
        {
            std::lock_guard lock(inner_->mutex);
            inner_->queue.push_front(data);
        }
        inner_->available.notify_one();
    }


private:
    std::shared_ptr<Inner<T>> inner_;
};

template <typename T>
class Receiver { 
public:
    Receiver(std::shared_ptr<Inner<T>> inner) : inner_{inner} {}

    Receiver& operator=(const Receiver&) = delete;
    Receiver(const Receiver&) = delete;

    Receiver& operator=(Receiver&&) = default;
    Receiver(Receiver&&) = default;

    ~Receiver() = default;

    std::optional<T> recv() {
        std::unique_lock lock(inner_->mutex);

        while(true) {
            if (!inner_->queue.empty()) {
                auto data = inner_->queue.front();
                inner_->queue.pop_front();
                return data;
            } else if (inner_->senders == 0) {
                return std::nullopt;
            } else {
                inner_->available.wait(lock);
            }              
        }
    }

    std::optional<T> try_recv() {
        std::lock_guard lock(inner_->mutex);
        
        if (!inner_->queue.empty()) {
            auto data = inner_->queue.front();
            inner_->queue.pop_front();
            return data;
        }

        return std::nullopt;
    }


private:
    std::shared_ptr<Inner<T>> inner_;
};

template <typename T>
std::tuple<Sender<T>, Receiver<T>> make_channel() {
    auto inner = std::make_shared<Inner<T>>();
    return std::make_tuple(Sender<T>(inner), Receiver<T>(inner));
}

}  // namespace mpsc
 
