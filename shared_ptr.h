#include <iostream>
#include <vector>
#include <memory>

template <typename T>
class EnableSharedFromThis;

template <typename T>
class WeakPtr;

struct BaseControlBlock {
  size_t shared_count;
  size_t weak_count;

  virtual void deleter() = 0;
  virtual void destroyer() {
    delete this;
  }
  virtual ~BaseControlBlock() = default;
  BaseControlBlock() noexcept : shared_count(0), weak_count(0) {}
  BaseControlBlock(size_t shared_count, size_t weak_count) : shared_count(shared_count), weak_count(weak_count) {}
};

template <typename T>
class SharedPtr {
 public:

  template <typename U, typename Deleter = std::default_delete<U>, typename Allocator = std::allocator<U>>
  struct ControlBlockRegular : BaseControlBlock {
    U* ptr;
    Allocator alloc;
    Deleter del;

    ControlBlockRegular(size_t shared_count, size_t weak_count, U* ptr, const Deleter& del, const Allocator& alloc) :
            BaseControlBlock(shared_count, weak_count), ptr(ptr), alloc(alloc), del(del) {}

    void deleter() noexcept override {
      del(ptr);
    }

    void destroyer() noexcept override {
      using BlockAlloc = typename std::allocator_traits<Allocator>::template
      rebind_alloc<ControlBlockRegular>;
      using BlockTraits = std::allocator_traits<BlockAlloc>;
      BlockAlloc ba = alloc;
      BlockTraits::deallocate(ba, this, 1);
    }
  };

  template <typename U, typename Allocator = std::allocator<U>>
  struct ControlBlockMakeShared : BaseControlBlock {
    Allocator alloc;
    U value;

    template<typename... Args>
    explicit ControlBlockMakeShared(const Allocator& alloc, Args&&... args)
            : BaseControlBlock(), alloc(alloc), value(std::forward<Args>(args)...) {}

    void deleter() noexcept override {
      std::allocator_traits<Allocator>::destroy(alloc, &value);
    }

    void destroyer() noexcept override {
      using BlockAlloc = typename std::allocator_traits<Allocator>::template
      rebind_alloc<ControlBlockMakeShared>;
      using BlockTraits = std::allocator_traits<BlockAlloc>;
      BlockAlloc ba = alloc;
      BlockTraits::deallocate(ba, this, 1);
    }
  };

 private:
  BaseControlBlock* count = nullptr;
  T* ptr = nullptr;

  template <typename Deleter = std::default_delete<T>, typename Allocator = std::allocator<T>>
  auto construct_control_block(T* ptr, const Deleter& del = Deleter(), const Allocator& alloc = Allocator()) {
    using BlockAlloc = typename std::allocator_traits<Allocator>::template
    rebind_alloc<ControlBlockRegular<T, Deleter, Allocator>>;
    BlockAlloc ba = alloc;
    auto* new_ptr = ba.allocate(1);
    new(new_ptr) ControlBlockRegular<T, Deleter, Allocator>(1, 0, ptr, del, alloc);
    return new_ptr;
  }

  template <typename U>
  SharedPtr(const WeakPtr<U>& weak_ptr) : count(weak_ptr.count), ptr(weak_ptr.ptr) {
    if (count) {
      ++count->shared_count;
    }
  }

  template <typename Allocator = std::allocator<T>>
  SharedPtr(ControlBlockMakeShared<T, Allocator>* count) : count(count), ptr(&count->value) {}

 public:

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

  SharedPtr() noexcept
          : count(nullptr), ptr(nullptr) {}

  SharedPtr(const SharedPtr& other)
          : count(other.count),
            ptr(other.ptr)
  {
    if (count) {
      ++count->shared_count;
    }
  }

  template <typename U, typename = std::enable_if_t<!std::is_same_v<T, U> &&
                                                    std::is_base_of_v<T, U> > >
  SharedPtr(const SharedPtr<U>& other) : count(other.count), ptr(other.ptr) {
    if (count) {
      ++count->shared_count;
    }
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  SharedPtr(SharedPtr<U>&& other) : count(other.count), ptr(other.ptr) {
    other.ptr = nullptr;
    other.count = nullptr;
  }

  template <typename U, typename Deleter = std::default_delete<U>, typename Allocator = std::allocator<U>,
          typename = std::enable_if_t<std::is_same_v<T, U> ||
                                      std::is_base_of_v<T, U> > >
  SharedPtr(U* ptr, const Deleter& del = Deleter(), const Allocator& alloc = Allocator()) {
    if (ptr) {
      count = construct_control_block(ptr, del, alloc);
      this->ptr = ptr;
      if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
        ptr->weak_ptr = *this;
      }
    } else {
      count = nullptr;
    }
  }

  ~SharedPtr() {
    if (count) {
      if (--count->shared_count == 0) {
        count->deleter();
        if (count->weak_count == 0) {
          count->destroyer();
        }
      }
    }
    count = nullptr;
  }

  void swap(SharedPtr& other) {
    std::swap(count, other.count);
    std::swap(ptr, other.ptr);
  }

  SharedPtr& operator=(const SharedPtr& other) {
    auto new_ptr = SharedPtr<T>(other);
    swap(new_ptr);
    return *this;
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  SharedPtr& operator=(SharedPtr<U>&& other) {
    auto new_ptr = SharedPtr<T>(std::move(other));
    swap(new_ptr);
    return *this;
  }

  size_t use_count() const {
    return count ? count->shared_count : 0;
  }

  void reset() noexcept {
    if (count) {
      if (--count->shared_count == 0) {
        count->deleter();
        if (count->weak_count == 0)
          count->destroyer();
      }
      count = nullptr;
      ptr   = nullptr;
    }
  }

  template <typename U>
  void reset(U* other) noexcept {
    reset();
    if (other) {
      count = construct_control_block(other);
      ptr   = other;
      if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
        other->weak_ptr = *this;
      }
    }
  }

  T& operator*() const noexcept {
    return *ptr;
  }

  T* get() const noexcept {
    if (count) {
      return ptr;
    }
    return nullptr;
  }

  T* operator->() const noexcept {
    return get();
  }

  template <typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&... args);

  template <typename U, typename Allocator, typename... Args>
  friend SharedPtr<U> allocateShared(const Allocator& alloc, Args&&... args);

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  SharedPtr(const SharedPtr<U>& other, T* alias_ptr) : count(other.count), ptr(alias_ptr) {
    if (count) {
      ++(count->shared_count);
    }
  }

};


template <typename T, typename Allocator, typename... Args>
SharedPtr<T> allocateShared(const Allocator& alloc, Args&&... args) {
  using ControlBlock = typename SharedPtr<T>::template ControlBlockMakeShared<T, Allocator>;
  using BlockAlloc = typename std::allocator_traits<Allocator>::template
  rebind_alloc<ControlBlock>;
  BlockAlloc ba = alloc;
  auto ptr = ba.allocate(1);
  std::allocator_traits<BlockAlloc>::construct(
          ba, ptr, alloc, std::forward<Args>(args)...);
  ptr->shared_count = 1;
  return SharedPtr<T>(ptr);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {
 private:

  BaseControlBlock* count = nullptr;
  T* ptr = nullptr;

 public:

  template <typename U>
  friend class SharedPtr;
  template <typename U>
  friend class WeakPtr;

  WeakPtr() : count(nullptr), ptr(nullptr) {}

  WeakPtr(const WeakPtr& other) : count(other.count), ptr(other.ptr) {
    if (count) {
      ++(count->weak_count);
    }
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  WeakPtr(const WeakPtr<U>& other) : count(other.count), ptr(other.ptr) {
    if (count) {
      ++(count->weak_count);
    }
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  WeakPtr(const SharedPtr<U>& other) : count(other.count), ptr(other.ptr) {
    if (count) {
      ++(count->weak_count);
    }
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  WeakPtr(WeakPtr<U>&& other) : count(other.count), ptr(other.ptr) {
    other.count = nullptr;
    other.ptr = nullptr;
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  WeakPtr& operator=(WeakPtr<U>& other) {
    auto new_ptr = WeakPtr<T>(other);
    this->swap(new_ptr);
    return *this;
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  WeakPtr& operator=(WeakPtr<U>&& other) {
    auto new_ptr = WeakPtr<T>(std::move(other));
    this->swap(new_ptr);
    return *this;
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  WeakPtr& operator=(SharedPtr<U>& other) {
    auto new_ptr = WeakPtr<T>(other);
    this->swap(new_ptr);
    return *this;
  }

  ~WeakPtr() {
    if (count) {
      if (--count->weak_count == 0 && count->shared_count == 0) {
        count->destroyer();
        count = nullptr;
      }
    }
  }

  bool expired() const noexcept {
    return count->shared_count == 0;
  }

  SharedPtr<T> lock() const noexcept {
    if (expired()) {
      return SharedPtr<T>();
    } else {
      return SharedPtr<T>(*this);
    }
  }

  size_t use_count() {
    if (count) {
      return count->shared_count;
    }
    return 0;
  }

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
                                                    std::is_base_of_v<T, U> > >
  void swap(WeakPtr<U>& other) {
    std::swap(count, other.count);
    std::swap(ptr, other.ptr);
  }

};

template <typename T>
class EnableSharedFromThis {
 private:

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

  WeakPtr<T> weak_ptr;

 public:

  SharedPtr<T> shared_from_this() const noexcept {
    return weak_ptr.lock();
  }

};
