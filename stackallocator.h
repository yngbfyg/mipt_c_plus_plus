#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <iterator>
#include <memory>
#include <utility>
#include <stdexcept>

template <std::size_t N>
class StackStorage {
 private:
  alignas(std::max_align_t) unsigned char buffer[N];
  std::size_t offset = 0;
 public:
  StackStorage() noexcept = default;
  StackStorage(const StackStorage&) = delete;
  StackStorage& operator=(const StackStorage&) = delete;

  void* allocateBlock(std::size_t bytes, std::size_t alignment) {
    uintptr_t currentPtr = reinterpret_cast<uintptr_t>(buffer) + offset;
    std::size_t adjustment = 0;
    if (alignment != 0 && currentPtr % alignment != 0) {
      adjustment = alignment - (currentPtr % alignment);
    }
    if (offset + adjustment + bytes > N) {
      throw std::bad_alloc();
    }
    std::size_t alignedOffset = offset + adjustment;
    offset = alignedOffset + bytes;
    return buffer + alignedOffset;
  }
};


template <typename T, std::size_t N>
class StackAllocator {
 private:
  StackStorage<N>* storage;
 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  using propagate_on_container_copy_construction = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_copy_assignment = std::false_type;
  using is_always_equal = std::false_type;

  template<typename X, std::size_t M> friend class StackAllocator;

  explicit StackAllocator(StackStorage<N>& arena) noexcept : storage(&arena) {}
  StackAllocator(const StackAllocator& other) noexcept : storage(other.storage) {}
  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) noexcept : storage(other.storage) {}
  ~StackAllocator() = default;
  StackAllocator& operator=(const StackAllocator& other) noexcept {
    storage = other.storage;
    return *this;
  }

  T* allocate(std::size_t n) {
    void* ptr = storage->allocateBlock(n * sizeof(T), alignof(T));
    return static_cast<T*>(ptr);
  }

  void deallocate(T*, std::size_t) noexcept {}

  template <typename U>
  struct rebind { using other = StackAllocator<U, N>; };

  bool operator==(const StackAllocator& other) const noexcept {
    return storage == other.storage;
  }
  bool operator!=(const StackAllocator& other) const noexcept {
    return storage != other.storage;
  }
};


template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct NodeBase {
    NodeBase* prev;
    NodeBase* next;
    NodeBase(NodeBase* p = nullptr, NodeBase* n = nullptr) noexcept : prev(p), next(n) {}
  };
  struct Node : NodeBase {
    T data;
    template <typename... Args>
    Node(NodeBase* p, NodeBase* n, Args&&... args)
            : NodeBase(p, n), data(std::forward<Args>(args)...) {}
  };

  using AllocTraits = std::allocator_traits<Allocator>;
  using NodeAllocator = typename AllocTraits::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAllocator>;

  NodeBase sentinel;
  std::size_t size_;
  Allocator alloc_;

  template <typename... Args>
  Node* create_node(NodeBase* prev, NodeBase* next, Args&&... args) {
    NodeAllocator nodeAlloc(alloc_);
    Node* newNode = NodeAllocTraits::allocate(nodeAlloc, 1);
    try {
      NodeAllocTraits::construct(nodeAlloc, newNode, prev, next, std::forward<Args>(args)...);
    } catch (...) {
      NodeAllocTraits::deallocate(nodeAlloc, newNode, 1);
      throw;
    }
    return newNode;
  }

 public:
  using value_type      = T;
  using allocator_type  = Allocator;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference       = T&;
  using const_reference = const T&;
  using pointer         = typename AllocTraits::pointer;
  using const_pointer   = typename AllocTraits::const_pointer;

  template<bool IsConst>
  class Iterator {
    friend class List;
    using NodeBaseType = std::conditional_t<IsConst, const NodeBase*, NodeBase*>;
    NodeBaseType node;
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using reference         = std::conditional_t<IsConst, const T&, T&>;
    using pointer           = std::conditional_t<IsConst, const T*, T*>;
    using NodeType = std::conditional_t<IsConst, const Node*, Node*>;

    Iterator(NodeBaseType n = nullptr) noexcept : node(n) {}
    Iterator(const Iterator& other) noexcept = default;
    Iterator& operator=(const Iterator& other) noexcept = default;

    template <bool type = IsConst, typename = std::enable_if_t<type>>
    Iterator(const Iterator<false>& other) noexcept : node(other.node) {}

    reference operator*() const {
      return static_cast<NodeType>(node)->data;
    }
    pointer operator->() const {
      return std::addressof(static_cast<NodeType>(node)->data);
    }
    Iterator& operator++() {
      node = node->next;
      return *this;
    }
    Iterator operator++(int) {
      iterator tmp(*this);
      node = node->next;
      return tmp;
    }
    Iterator& operator--() {
      node = node->prev;
      return *this;
    }
    Iterator operator--(int) {
      iterator tmp(*this);
      node = node->prev;
      return tmp;
    }
    bool operator==(const Iterator& other) const noexcept { return node == other.node; }
    bool operator!=(const Iterator& other) const noexcept { return node != other.node; }
  };

  using reverse_iterator       = std::reverse_iterator<Iterator<false>>;
  using const_reverse_iterator = std::reverse_iterator<Iterator<true>>;

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;


  List() noexcept(std::is_nothrow_default_constructible_v<Allocator>)
          : sentinel(&sentinel, &sentinel), size_(0), alloc_() {}

  explicit List(const Allocator& alloc) noexcept
          : sentinel(&sentinel, &sentinel), size_(0), alloc_(alloc) {}

  explicit List(size_type count, const Allocator& alloc = Allocator())
          : sentinel(&sentinel, &sentinel), size_(0), alloc_(alloc) {
    if constexpr (!std::is_default_constructible_v<T>) {
      static_assert(std::is_default_constructible_v<T>,
                    "List(size_type) requires default-constructible value_type");
    }
    try {
      for (size_type i = 0; i < count; ++i) {
        emplace_back();
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(const List& other)
          : sentinel(&sentinel, &sentinel), size_(0),
            alloc_(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.alloc_)) {
    try {
      for (const T& value : other) {
        push_back(value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(List&& other) noexcept(std::is_nothrow_move_constructible_v<Allocator>)
          : sentinel(&sentinel, &sentinel), size_(0), alloc_(std::move(other.alloc_)) {
    if (!other.empty()) {
      sentinel.next = other.sentinel.next;
      sentinel.prev = other.sentinel.prev;
      sentinel.next->prev = &sentinel;
      sentinel.prev->next = &sentinel;
      size_ = other.size_;
      other.sentinel.next = &other.sentinel;
      other.sentinel.prev = &other.sentinel;
      other.size_ = 0;
    }
  }

  ~List() {
    clear();
  }

  List& operator=(const List& other) {
    if (this != &other) {
      if constexpr (AllocTraits::propagate_on_container_copy_assignment::value) {
        List tmp(other);
        swap(tmp);
        alloc_ = other.alloc_;
      }
      else {
        if (alloc_ == other.alloc_) {
          List tmp(other);
          swap(tmp);
        } else {
          clear();
          for (const T& x : other) {
            push_back(x);
          }
        }
      }
    }
    return *this;
  }

  List& operator=(List&& other) noexcept(
  AllocTraits::propagate_on_container_move_assignment::value &&
  std::is_nothrow_move_assignable_v<Allocator>) {
    if (this != &other) {
      clear();
      if constexpr (AllocTraits::propagate_on_container_move_assignment::value) {
        alloc_ = std::move(other.alloc_);
        if (!other.empty()) {
          sentinel.next = other.sentinel.next;
          sentinel.prev = other.sentinel.prev;
          sentinel.next->prev = &sentinel;
          sentinel.prev->next = &sentinel;
        }
        size_ = other.size_;
        other.sentinel.next = &other.sentinel;
        other.sentinel.prev = &other.sentinel;
        other.size_ = 0;
      } else {
        if (alloc_ == other.alloc_) {
          if (!other.empty()) {
            sentinel.next = other.sentinel.next;
            sentinel.prev = other.sentinel.prev;
            sentinel.next->prev = &sentinel;
            sentinel.prev->next = &sentinel;
          }
          size_ = other.size_;
          other.sentinel.next = &other.sentinel;
          other.sentinel.prev = &other.sentinel;
          other.size_ = 0;
        } else {
          clear();
          for (auto& x : other) {
            push_back(std::move(x));
          }
          other.clear();
        }
      }
    }
    return *this;
  }

  void swap(List& other) noexcept(
  std::is_nothrow_swappable_v<Allocator> &&
  (AllocTraits::propagate_on_container_swap::value || AllocTraits::is_always_equal::value)) {
    if (this == &other) return;
    NodeBase* thisFirst = sentinel.next;
    NodeBase* thisLast = sentinel.prev;
    NodeBase* otherFirst = other.sentinel.next;
    NodeBase* otherLast = other.sentinel.prev;
    sentinel.next = otherFirst;
    sentinel.prev = otherLast;
    other.sentinel.next = thisFirst;
    other.sentinel.prev = thisLast;
    if (sentinel.next == &other.sentinel) {
      sentinel.next = &sentinel;
      sentinel.prev = &sentinel;
    } else {
      sentinel.next->prev = &sentinel;
      sentinel.prev->next = &sentinel;
    }
    if (other.sentinel.next == &sentinel) {
      other.sentinel.next = &other.sentinel;
      other.sentinel.prev = &other.sentinel;
    } else {
      other.sentinel.next->prev = &other.sentinel;
      other.sentinel.prev->next = &other.sentinel;
    }
    std::swap(size_, other.size_);
    if constexpr (AllocTraits::propagate_on_container_swap::value ||
                  AllocTraits::is_always_equal::value) {
      std::swap(alloc_, other.alloc_);
    }
  }

  void clear() noexcept {
    NodeBase* cur = sentinel.next;
    NodeAllocator nodeAlloc(alloc_);
    while (cur != &sentinel) {
      Node* curNode = static_cast<Node*>(cur);
      cur = cur->next;
      NodeAllocTraits::destroy(nodeAlloc, curNode);
      NodeAllocTraits::deallocate(nodeAlloc, curNode, 1);
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    size_ = 0;
  }

  iterator insert(const_iterator pos, const T& value) {
    return emplace(pos, value);
  }

  iterator insert(const_iterator pos, T&& value) {
    return emplace(pos, std::move(value));
  }

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&&... args) {
    NodeBase* nextNode = const_cast<NodeBase*>(pos.node);
    NodeBase* prevNode = nextNode->prev;
    Node* newNode = create_node(prevNode, nextNode, std::forward<Args>(args)...);
    prevNode->next = newNode;
    nextNode->prev = newNode;
    ++size_;
    return iterator(newNode);
  }

  void push_back(const T& value) { emplace_back(value); }
  void push_back(T&& value) { emplace_back(std::move(value)); }
  template <typename... Args>
  reference emplace_back(Args&&... args) {
    Node* newNode = create_node(sentinel.prev, &sentinel, std::forward<Args>(args)...);
    sentinel.prev->next = newNode;
    sentinel.prev = newNode;
    ++size_;
    return newNode->data;
  }

  void push_front(const T& value) { emplace_front(value); }
  void push_front(T&& value) { emplace_front(std::move(value)); }
  template <typename... Args>
  reference emplace_front(Args&&... args) {
    Node* newNode = create_node(&sentinel, sentinel.next, std::forward<Args>(args)...);
    sentinel.next->prev = newNode;
    sentinel.next = newNode;
    ++size_;
    return newNode->data;
  }

  iterator erase(const_iterator pos) {
    NodeBase* nodeBase = const_cast<NodeBase*>(pos.node);
    if (nodeBase == &sentinel) {
      return end();
    }
    Node* node = static_cast<Node*>(nodeBase);
    NodeBase* prevNode = nodeBase->prev;
    NodeBase* nextNode = nodeBase->next;
    prevNode->next = nextNode;
    nextNode->prev = prevNode;
    --size_;
    NodeAllocator nodeAlloc(alloc_);
    NodeAllocTraits::destroy(nodeAlloc, node);
    NodeAllocTraits::deallocate(nodeAlloc, node, 1);
    return iterator(nextNode);
  }

  iterator erase(const_iterator first, const_iterator last) {
    const_iterator it = first;
    while (it != last) {
      it = erase(it);
    }
    return iterator(const_cast<NodeBase*>(last.node));
  }

  void pop_back() {
    if (!empty()) {
      Node* node = static_cast<Node*>(sentinel.prev);
      NodeBase* prevNode = node->prev;
      prevNode->next = &sentinel;
      sentinel.prev = prevNode;
      --size_;
      NodeAllocator nodeAlloc(alloc_);
      NodeAllocTraits::destroy(nodeAlloc, node);
      NodeAllocTraits::deallocate(nodeAlloc, node, 1);
    }
  }

  void pop_front() {
    if (!empty()) {
      Node* node = static_cast<Node*>(sentinel.next);
      NodeBase* nextNode = node->next;
      sentinel.next = nextNode;
      nextNode->prev = &sentinel;
      --size_;
      NodeAllocator nodeAlloc(alloc_);
      NodeAllocTraits::destroy(nodeAlloc, node);
      NodeAllocTraits::deallocate(nodeAlloc, node, 1);
    }
  }

  reference front() {
    return static_cast<Node*>(sentinel.next)->data;
  }
  const_reference front() const {
    return static_cast<const Node*>(sentinel.next)->data;
  }
  reference back() {
    return static_cast<Node*>(sentinel.prev)->data;
  }
  const_reference back() const {
    return static_cast<const Node*>(sentinel.prev)->data;
  }

  iterator begin() noexcept { return iterator(sentinel.next); }
  const_iterator begin() const noexcept { return const_iterator(sentinel.next); }
  const_iterator cbegin() const noexcept { return const_iterator(sentinel.next); }
  iterator end() noexcept { return iterator(&sentinel); }
  const_iterator end() const noexcept { return const_iterator(&sentinel); }
  const_iterator cend() const noexcept { return const_iterator(&sentinel); }
  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

  size_type size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  allocator_type get_allocator() const noexcept { return alloc_; }
};