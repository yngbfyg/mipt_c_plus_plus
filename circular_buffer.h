#include <cstddef>
#include <stdexcept>
#include <iterator>
#include <type_traits>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <limits>
#include <new>
#include <memory>
#include <cstddef>

constexpr std::size_t DYNAMIC_CAPACITY = std::numeric_limits<std::size_t>::max();

template <std::size_t Cap>
struct CapBaseTrue {
  std::size_t cap_;

  CapBaseTrue() = default;
  explicit CapBaseTrue(std::size_t c) noexcept
          : cap_{c}
  {}
  CapBaseTrue(const CapBaseTrue&) noexcept = default;
  CapBaseTrue& operator=(CapBaseTrue const& o) noexcept {
    cap_ = o.cap_;
    return *this;
  }
  std::size_t capacity() const noexcept {
    return cap_;
  }
};

template <std::size_t Cap>
struct CapBaseFalse {
  CapBaseFalse() = default;
  explicit CapBaseFalse(std::size_t) noexcept {}
  CapBaseFalse(const CapBaseFalse&) noexcept = default;
  CapBaseFalse& operator=(CapBaseFalse const&) noexcept {
    return *this;
  }
  std::size_t capacity() const noexcept {
    return Cap;
  }
};

template<bool Dynamic, std::size_t Cap>
using CapBase = std::conditional_t<Dynamic, CapBaseTrue<Cap>, CapBaseFalse<Cap>>;


template<typename T, std::size_t Capacity = DYNAMIC_CAPACITY>
class CircularBuffer {
 public:
  using value_type = T;

  template<std::size_t N>
  struct StaticStorage {
    alignas(T) std::byte data[sizeof(T) * N];
    StaticStorage() noexcept = default;
    explicit StaticStorage(std::size_t cap) noexcept {assert(cap == N); }
    ~StaticStorage() = default;
    std::size_t capacity() const noexcept { return N; }
    T* ptr(std::size_t i) noexcept { return reinterpret_cast<T*>(data + i * sizeof(T)); }
    const T* ptr(std::size_t i) const noexcept { return reinterpret_cast<const T*>(data + i * sizeof(T)); }
  };

  struct DynamicStorage {
    std::byte* data;
    std::size_t size;

    DynamicStorage() noexcept = default;
    explicit DynamicStorage(std::size_t cap)
            : data(static_cast<std::byte*>(::operator new(cap * sizeof(T)))),
              size(cap) {}
    ~DynamicStorage() {
      ::operator delete(data);
    }

    std::size_t capacity() const noexcept { return size; }
    T* ptr(std::size_t i) noexcept { return reinterpret_cast<T*>(data + i * sizeof(T)); }
    const T* ptr(std::size_t i) const noexcept { return reinterpret_cast<const T*>(data + i * sizeof(T)); }
  };

  static constexpr bool is_static = (Capacity != DYNAMIC_CAPACITY);
  using storage_type = std::conditional_t<(!is_static),
          DynamicStorage,
          StaticStorage<Capacity>>;


  template<typename Pointer>
  class my_iterator_impl
          : private CapBase<!is_static, Capacity>
  {
    friend class CircularBuffer;
    template<typename> friend class my_iterator_impl;

    using Base = CapBase<!is_static, Capacity>;

   public:
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = Pointer;
    using reference         = std::remove_pointer_t<Pointer>&;
    using iterator_category = std::random_access_iterator_tag;

    my_iterator_impl() = default;

    my_iterator_impl(pointer base, std::size_t pos, std::size_t head)
            : Base{}, base_(base), pos_(pos), head_(head) {}

    my_iterator_impl(pointer base, std::size_t pos, std::size_t head, std::size_t cap)
            : Base{cap}, base_(base), pos_(pos), head_(head) {}

    template<typename OtherPtr,
            typename = std::enable_if_t<
                    std::is_convertible_v<OtherPtr, pointer>
            >>
    my_iterator_impl(const my_iterator_impl<OtherPtr> &other)
            : Base(other), base_(other.base_), pos_(other.pos_), head_(other.head_) {}

    reference operator*() const {
      return base_[(head_ + pos_) % Base::capacity()];
    }

    pointer operator->() const {
      return &base_[(head_ + pos_) % Base::capacity()];
    }

    my_iterator_impl& operator++() { ++pos_; return *this; }
    my_iterator_impl operator++(int) {
      auto tmp = *this; ++pos_; return tmp;
    }
    my_iterator_impl& operator--() { --pos_; return *this; }
    my_iterator_impl operator--(int) {
      auto tmp = *this; --pos_; return tmp;
    }
    my_iterator_impl& operator+=(difference_type n) { pos_ += n; return *this; }
    my_iterator_impl operator+(difference_type n) const {
      auto tmp = *this; tmp.pos_ += n; return tmp;
    }
    friend my_iterator_impl operator+(difference_type n, const my_iterator_impl &it) {
      auto tmp = it; tmp.pos_ += n; return tmp;
    }
    my_iterator_impl& operator-=(difference_type n) { pos_ -= n; return *this; }
    my_iterator_impl operator-(difference_type n) const {
      auto tmp = *this; tmp.pos_ -= n; return tmp;
    }
    difference_type operator-(const my_iterator_impl &o) const {
      return (difference_type)pos_ - (difference_type)o.pos_;
    }
    bool operator==(const my_iterator_impl &o) const { return pos_ == o.pos_; }
    bool operator!=(const my_iterator_impl &o) const { return pos_ != o.pos_; }
    bool operator<(const my_iterator_impl &o) const { return pos_ < o.pos_; }
    bool operator>(const my_iterator_impl &o) const { return pos_ > o.pos_; }
    bool operator<=(const my_iterator_impl &o) const { return pos_ <= o.pos_; }
    bool operator>=(const my_iterator_impl &o) const { return pos_ >= o.pos_; }

   private:

    pointer base_ = nullptr;
    std::size_t pos_ = 0;
    std::size_t head_ = 0;
  };

  using iterator = my_iterator_impl<T*>;
  using const_iterator = my_iterator_impl<const T*>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  template<bool S = is_static,
          typename = std::enable_if_t<S>>
  CircularBuffer()
          : storage_(), head_(0), tail_(0), full_(false) {
  }

  explicit CircularBuffer(std::size_t cap)
          : head_(0), tail_(0), full_(false) {
    if constexpr (is_static) {
      if (Capacity != cap)
        throw std::invalid_argument("Circular buffer capacity mismatch");
    } else {
      new (&storage_) storage_type(cap);
    }
  }

  CircularBuffer(const CircularBuffer &other)
          : head_(other.head_), tail_(other.tail_), full_(other.full_) {
    if constexpr (!is_static) {
      new (&storage_) storage_type(other.capacity());

      T* dst = storage_.ptr(0);
      const T* src = other.storage_.ptr(0);
      for (std::size_t i = 0; i < other.capacity(); ++i) {
        if (other.constructed_at(i)) {
          std::construct_at(&dst[i], src[i]);
        }
      }
    } else {
      T* dst = storage_.ptr(0);
      const T* src = other.storage_.ptr(0);
      for (std::size_t i = 0; i < Capacity; ++i) {
        if (other.constructed_at(i)) {
          std::construct_at(&dst[i], src[i]);
        }
      }
    }
  }

  CircularBuffer &operator=(const CircularBuffer& other) {
    if (this == &other)
      return *this;

    for (std::size_t k = 0; k < size(); ++k) {
      std::size_t idx = (head_ + k) % capacity();
      std::destroy_at(storage_.ptr(idx));
    }

    if constexpr (!is_static) {
      if (other.capacity() != capacity()) {
        auto *ptr = std::launder(reinterpret_cast<storage_type*>(&storage_));
        ptr->~storage_type();
        new (&storage_) storage_type(other.capacity());
      }
    }

    for (std::size_t i = 0; i < other.capacity(); ++i) {
      if (other.constructed_at(i)) {
        std::construct_at(&storage_.ptr(i)[0], other.storage_.ptr(i)[0]);
      }
    }

    head_ = other.head_;
    tail_ = other.tail_;
    full_ = other.full_;

    return *this;
  }

  ~CircularBuffer() {
    std::size_t n = size();
    for (std::size_t k = 0; k < n; ++k) {
      std::size_t idx = (head_ + k) % capacity();
      std::destroy_at(storage_.ptr(idx));
    }
  }

  T &operator[](std::size_t index) {
    return storage_.ptr(0)[(head_ + index) % capacity()];
  }

  const T &operator[](std::size_t index) const {
    return storage_.ptr(0)[(head_ + index) % capacity()];
  }

  T &at(std::size_t index) {
    if (index >= size())
      throw std::out_of_range("Index out of range");
    return (*this)[index];
  }

  const T &at(std::size_t index) const {
    if (index >= size())
      throw std::out_of_range("Index out of range");
    return (*this)[index];
  }

  std::size_t size() const {
    if (full_) return capacity();
    if (tail_ >= head_)
      return tail_ - head_;
    else
      return capacity() - head_ + tail_;
  }

  std::size_t capacity() const {
    if constexpr (!is_static)
      return storage_.size;
    else
      return Capacity;
  }

  bool empty() const { return (head_ == tail_) && !full_; }
  bool full() const { return full_; }

  bool constructed_at(std::size_t index) const noexcept {
    if (full_) {
      return true;
    }
    if (head_ <= tail_) {
      return (index >= head_) && (index < tail_);
    } else {
      return (index >= head_) || (index < tail_);
    }
  }

  bool constructed_at(std::size_t index) noexcept {
    return static_cast<const CircularBuffer*>(this)->constructed_at(index);
  }

  void push_back(const T &value) {
    T* d = storage_.ptr(tail_);
    std::size_t c = capacity();
    if (!constructed_at(tail_)) {
      std::construct_at(d, value);
    } else {
      *d = value;
    }
    if (full_)
      head_ = (head_ + 1) % c;
    tail_ = (tail_ + 1) % c;
    full_ = (tail_ == head_);
  }

  void push_front(const T &value) {

    std::size_t c = capacity();
    std::size_t new_head = (head_ + c - 1) % c;
    T* d =storage_.ptr(new_head);
    if (!constructed_at(new_head)) {
      std::construct_at(d, value);
    } else {
      *d = value;
    }
    head_ = new_head;
    if (full_)
      tail_ = head_;
    full_ = (tail_ == head_);
  }

  void pop_front() {
    if (empty())
      throw std::out_of_range("Buffer empty");
    T* d = storage_.ptr(head_);
    std::destroy_at(d);
    head_ = (head_ + 1) % capacity();
    full_ = false;
  }

  void pop_back() {
    if (empty())
      throw std::out_of_range("Buffer empty");
    std::size_t c = capacity();
    std::size_t new_tail = (tail_ + c - 1) % c;
    T* d = storage_.ptr(new_tail);
    std::destroy_at(d);
    tail_ = new_tail;
    full_ = false;
  }

  void swap(CircularBuffer &other) noexcept {
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    std::swap(full_, other.full_);

    if constexpr (is_static) {
      auto raw1 = reinterpret_cast<std::byte*>( storage_.ptr(0) );
      auto raw2 = reinterpret_cast<std::byte*>( other.storage_.ptr(0) );
      std::swap_ranges(raw1,
                       raw1 + capacity() * sizeof(T),
                       raw2);
    } else {
      std::swap(storage_.data, other.storage_.data);
      std::swap(storage_.size, other.storage_.size);
    }
  }

  friend void swap(CircularBuffer &a, CircularBuffer &b) noexcept {
    a.swap(b);
  }

  auto insert(const const_iterator pos, const T &value) {
    std::size_t pos_index = pos.pos_;
    T* d = storage_.ptr(0);
    std::size_t n = size();
    std::size_t c = capacity();

    if (!full_) {
      for (std::size_t i = n; i > pos_index; --i) {
        std::size_t dst = (head_ + i) % c;
        std::size_t src = (head_ + i - 1) % c;
        if (!constructed_at(dst)) {
          std::construct_at(&d[dst], d[src]);
        } else {
          d[dst] = d[src];
        }
      }
      std::size_t ins = (head_ + pos_index) % c;
      if (!constructed_at(ins)) {
        std::construct_at(&d[ins], value);
      } else {
        d[ins] = value;
      }
      tail_ = (tail_ + 1) % c;
      if (tail_ == head_) {
        full_ = true;
      }
    } else {
      if (pos_index == 0)
        return begin();
      for (std::size_t i = n; i > pos_index; --i) {
        std::size_t dst = (head_ + i) % c;
        std::size_t src = (head_ + i - 1) % c;
        d[dst] = d[src];
      }
      std::size_t ins = (head_ + pos_index) % c;
      d[ins] = value;
      head_ = (head_ + 1) % c;
    }

    if constexpr (!is_static)
      return iterator(storage_.ptr(0), pos_index, head_, c);
    else
      return iterator(storage_.ptr(0), pos_index, head_);
  }

  auto erase(const const_iterator pos)
  {
    std::size_t idx = pos.pos_;
    std::size_t n   = size();
    if (idx >= n)
      throw std::out_of_range("Invalid iterator for erase");

    for (std::size_t k = idx; k + 1 < n; ++k) {
      (*this)[k] = (*this)[k + 1];
    }
    pop_back();
    if constexpr (!is_static)
      return iterator(storage_.ptr(0), idx, head_, capacity());
    else
      return iterator(storage_.ptr(0), idx, head_);
  }

  auto begin() -> iterator {
    if constexpr (!is_static)
      return iterator(storage_.ptr(0), 0, head_, capacity());
    else
      return iterator(storage_.ptr(0), 0, head_);
  }

  auto end() -> iterator {
    if constexpr (!is_static)
      return iterator(storage_.ptr(0), size(), head_, capacity());
    else
      return iterator(storage_.ptr(0), size(), head_);
  }

  auto begin() const -> const_iterator {
    if constexpr (!is_static)
      return const_iterator(storage_.ptr(0), 0, head_, capacity());
    else
      return const_iterator(storage_.ptr(0), 0, head_);
  }

  auto end() const -> const_iterator {
    if constexpr (!is_static)
      return const_iterator(storage_.ptr(0), size(), head_, capacity());
    else
      return const_iterator(storage_.ptr(0), size(), head_);
  }

  auto cbegin() const -> const_iterator { return begin(); }
  auto cend() const -> const_iterator { return end(); }

  auto rbegin() -> reverse_iterator { return reverse_iterator(end()); }
  auto rend() -> reverse_iterator { return reverse_iterator(begin()); }
  auto rbegin() const -> const_reverse_iterator { return const_reverse_iterator(end()); }
  auto rend() const -> const_reverse_iterator { return const_reverse_iterator(begin()); }
  auto crbegin() const -> const_reverse_iterator { return const_reverse_iterator(cend()); }
  auto crend() const -> const_reverse_iterator { return const_reverse_iterator(cbegin()); }

 private:

  storage_type storage_;
  std::size_t head_ = 0;
  std::size_t tail_ = 0;
  bool full_ = false;
};
