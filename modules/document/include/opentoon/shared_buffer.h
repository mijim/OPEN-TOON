#pragma once
#include <compare>
#include <initializer_list>
#include <memory>
#include <vector>
namespace opentoon {
// Immutable value semantics let document snapshots share unchanged media buffers.
template <typename T> class SharedBuffer {
  public:
    SharedBuffer() : data_(std::make_shared<const std::vector<T>>()) {}
    SharedBuffer(std::vector<T> data) : data_(std::make_shared<const std::vector<T>>(std::move(data))) {}
    SharedBuffer(std::initializer_list<T> data) : SharedBuffer(std::vector<T>(data)) {}
    [[nodiscard]] const T* data() const { return data_->data(); }
    [[nodiscard]] std::size_t size() const { return data_->size(); }
    [[nodiscard]] bool empty() const { return data_->empty(); }
    [[nodiscard]] auto begin() const { return data_->begin(); }
    [[nodiscard]] auto end() const { return data_->end(); }
    [[nodiscard]] const T& operator[](std::size_t index) const { return (*data_)[index]; }
    [[nodiscard]] const std::vector<T>& values() const { return *data_; }
    template <typename Iterator> void assign(Iterator first, Iterator last) {
        data_ = std::make_shared<const std::vector<T>>(first, last);
    }
    bool operator==(const SharedBuffer& other) const {
        return data_ == other.data_ || *data_ == *other.data_;
    }
    auto operator<=>(const SharedBuffer& other) const { return *data_ <=> *other.data_; }

  private:
    std::shared_ptr<const std::vector<T>> data_;
};
} // namespace opentoon
