#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace corewatch::core {

/// Fixed-capacity bounded ring buffer for time-series telemetry samples.
/// Allocates capacity once and overwrites the oldest element when full.
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity)
        : buffer_(capacity)
        , capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("RingBuffer capacity must be greater than zero");
        }
    }

    void push(const T& item) {
        buffer_[head_] = item;
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_) {
            ++size_;
        }
    }

    void push(T&& item) {
        buffer_[head_] = std::move(item);
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_) {
            ++size_;
        }
    }

    void clear() noexcept {
        head_ = 0;
        size_ = 0;
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] bool full() const noexcept { return size_ == capacity_; }

    /// Returns elements in chronological order (oldest -> newest).
    [[nodiscard]] std::vector<T> to_vector() const {
        std::vector<T> out;
        out.reserve(size_);
        const std::size_t start = (size_ == capacity_) ? head_ : 0;
        for (std::size_t i = 0; i < size_; ++i) {
            out.push_back(buffer_[(start + i) % capacity_]);
        }
        return out;
    }

    /// Returns at most the last `max_count` elements in chronological order.
    [[nodiscard]] std::vector<T> tail(std::size_t max_count) const {
        const std::size_t count = std::min(max_count, size_);
        std::vector<T> out;
        out.reserve(count);
        const std::size_t start = (head_ + capacity_ - count) % capacity_;
        for (std::size_t i = 0; i < count; ++i) {
            out.push_back(buffer_[(start + i) % capacity_]);
        }
        return out;
    }

private:
    std::vector<T> buffer_;
    std::size_t capacity_{0};
    std::size_t head_{0};
    std::size_t size_{0};
};

} // namespace corewatch::core
