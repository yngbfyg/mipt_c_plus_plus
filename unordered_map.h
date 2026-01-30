#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <iterator>
#include <memory>
#include <utility>
#include <stdexcept>
#include <vector>
#include <cmath>

template <class Key, class Value, class Hash = std::hash<Key>,
        class KeyEqual = std::equal_to<Key>,
        class Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap;

template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct NodeBase {
    NodeBase* prev;
    NodeBase* next;

    NodeBase(NodeBase* p = nullptr, NodeBase* n = nullptr) noexcept
            : prev(p), next(n) {}
  };

  struct Node : NodeBase {
    T data;

    template <typename... Args>
    Node(NodeBase* p, NodeBase* n, Args&& ... args)
            : NodeBase(p, n), data(std::forward<Args>(args)...) {}
  };

  using AllocTraits = std::allocator_traits<Allocator>;
  using NodeAllocator = typename AllocTraits::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAllocator>;

  NodeBase sentinel;
  std::size_t size_;
  Allocator alloc_;

  template <typename... Args>
  Node* create_node(NodeBase* prev, NodeBase* next, Args&& ... args) {
    NodeAllocator nodeAlloc(alloc_);
    Node* newNode = NodeAllocTraits::allocate(nodeAlloc, 1);
    try {
      NodeAllocTraits::construct(nodeAlloc,
                                 newNode,
                                 prev,
                                 next,
                                 std::forward<Args>(args)...);
    } catch (...) {
      NodeAllocTraits::deallocate(nodeAlloc, newNode, 1);
      throw;
    }
    return newNode;
  }

  template <class Key, class Value, class Hash,
          class KeyEqual, class Alloc>
  friend
  class UnorderedMap;

 public:
  using value_type = T;
  using allocator_type = Allocator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = typename AllocTraits::pointer;
  using const_pointer = typename AllocTraits::const_pointer;

  template <bool IsConst>
  class Iterator {
    friend class List;

    using NodeBaseType = std::conditional_t<IsConst,
            const NodeBase*,
            NodeBase*>;
    NodeBaseType node;
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using NodeType = std::conditional_t<IsConst, const Node*, Node*>;

    Iterator(const Iterator& other) noexcept = default;

    Iterator(NodeBaseType n = nullptr) noexcept: node(n) {}

    Iterator& operator=(const Iterator& other) noexcept = default;

    template <bool OtherIsConst,
            typename = std::enable_if_t<IsConst && !OtherIsConst>>
    Iterator(const Iterator<OtherIsConst>& other) noexcept : node(other.node) {}

    template <bool OtherIsConst,
            typename = std::enable_if_t<IsConst && !OtherIsConst>>
    Iterator& operator=(const Iterator<OtherIsConst>& other) noexcept {
      node = other.node;
      return *this;
    }

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

    template <bool C>
    bool operator==(const Iterator<C>& other) const noexcept {
      return node == other.node;
    }

    template <bool C, std::enable_if_t<!IsConst && C, int> = 0>
    bool operator!=(const Iterator& other) const noexcept {
      return node != other.node;
    }

    bool isNull() const noexcept {
      return node == nullptr;
    }
  };

  using reverse_iterator = std::reverse_iterator<Iterator<false>>;
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
            alloc_(std::allocator_traits<Allocator>::select_on_container_copy_construction(
                    other.alloc_)) {
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
          : sentinel(&sentinel, &sentinel),
            size_(0),
            alloc_(std::move(other.alloc_)) {
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
      } else {
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
  (AllocTraits::propagate_on_container_swap::value ||
   AllocTraits::is_always_equal::value)) {
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
    while (!empty()) {
      pop_back();
    }
  }

  iterator insert(const_iterator pos, const T& value) {
    return emplace(pos, value);
  }

  iterator insert(const_iterator pos, T&& value) {
    return emplace(pos, std::move(value));
  }

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&& ... args) {
    NodeBase* nextNode = const_cast<NodeBase*>(pos.node);
    NodeBase* prevNode = nextNode->prev;
    Node* newNode = create_node(prevNode,
                                nextNode,
                                std::forward<Args>(args)...);
    prevNode->next = newNode;
    nextNode->prev = newNode;
    ++size_;
    return iterator(newNode);
  }

  void push_back(const T& value) { emplace_back(value); }

  void push_back(T&& value) { emplace_back(std::move(value)); }

  template <typename... Args>
  reference emplace_back(Args&& ... args) {
    Node* newNode = create_node(sentinel.prev,
                                &sentinel,
                                std::forward<Args>(args)...);
    sentinel.prev->next = newNode;
    sentinel.prev = newNode;
    ++size_;
    return newNode->data;
  }

  void push_front(const T& value) { emplace_front(value); }

  void push_front(T&& value) { emplace_front(std::move(value)); }

  template <typename... Args>
  reference emplace_front(Args&& ... args) {
    Node* newNode = create_node(&sentinel,
                                sentinel.next,
                                std::forward<Args>(args)...);
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

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  size_type size() const noexcept { return size_; }

  bool empty() const noexcept { return size_ == 0; }

  allocator_type get_allocator() const noexcept { return alloc_; }

  NodeAllocator get_node_allocator() const noexcept {
    return NodeAllocator(alloc_);
  }

 private:
  iterator emplace_node(const_iterator pos, Node* node) {
    NodeBase* nextNode = const_cast<NodeBase*>(pos.node);
    NodeBase* prevNode = nextNode->prev;
    node->prev = prevNode;
    node->next = nextNode;
    prevNode->next = node;
    nextNode->prev = node;
    ++size_;
    return iterator(node);
  }

  iterator emplace_node_front(Node* node) {
    node->prev = &sentinel;
    node->next = sentinel.next;
    sentinel.next->prev = node;
    sentinel.next = node;
    ++size_;
    return iterator(node);
  }
};

template <class Key, class Value, class Hash,
        class KeyEqual, class Alloc>
class UnorderedMap {
 public:
  template <bool IsConst>
  class Iterator;

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;


  using NodeType = std::pair<const Key, Value>;


 private:
  struct Node {
    NodeType value;
    size_t hash = 0;

    template <class... Args>
    Node(Args&& ... args) : value(std::forward<Args>(args)...) {};

    Node(const Node& other) = default;

    Node(Node&& other) = default;
  };

  using AllocTraits = std::allocator_traits<Alloc>;
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeList = List<Node, NodeAlloc>;
  using ListNodeAllocTraits = NodeList::NodeAllocTraits;
  using NodeIter = NodeList::iterator;
  using NodeIterAlloc = typename AllocTraits::template rebind_alloc<NodeIter>;
 public:
  UnorderedMap() = default;

  UnorderedMap(const UnorderedMap& other)
          : hash_func_(other.hash_func_), eq_func_(other.eq_func_),
            alloc_(AllocTraits::select_on_container_copy_construction(other.alloc_)),
            max_load_factor_(other.max_load_factor_) {
    try {
      for (auto iter = other.begin(); iter != other.end(); ++iter) {
        emplace(*iter);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  UnorderedMap(UnorderedMap&& other) = default;

  void swap(UnorderedMap& other) {
    force_swap(other);
    if (!AllocTraits::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
    }
  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    if (&other != this) {
      UnorderedMap tmp(other);
      if (AllocTraits::propagate_on_container_copy_assignment::value) {
        tmp.alloc_ = other.alloc_;
      }
      force_swap(tmp);
    }
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) {
    if (&other != this) {
      UnorderedMap tmp(std::move(other));
      if (AllocTraits::propagate_on_container_move_assignment::value) {
        tmp.alloc_ = other.alloc_;
      }
      force_swap(tmp);
    }
    return *this;
  }

  double get_load_factor() const {
    return static_cast<double>(elements_.size()) /
           static_cast<double>(iters_.size());
  }

  double get_load_factor(size_t inserted_elements) const {
    return static_cast<double>(elements_.size() + inserted_elements) /
           static_cast<double>(iters_.size());
  }

  void set_max_load_factor(double load_factor) {
    max_load_factor_ = load_factor;
    if (get_load_factor() > max_load_factor_) {
      rehash(2 * iters_.size());
    }
  }

  double get_max_load_factor() const {
    return max_load_factor_;
  }

  iterator find(const Key& key) {
    std::size_t hash = hash_func_(key) % iters_.size();
    for (auto iter = iters_[hash];
         iter != elements_.end() &&
         (iter->hash % iters_.size()) == hash; ++iter) {
      if (eq_func_(key, iter->value.first)) {
        return iter;
      }
    }
    return end();
  }

  const_iterator find(const Key& key) const {
    int hash = hash_func_(key) % iters_.size();
    for (auto iter = iters_[hash];
         iter != elements_.end() &&
         (iter->hash % iters_.size()) == hash; ++iter) {
      if (eq_func_(key, iter->value.first)) {
        return iter;
      }
    }
    return end;
  }

  Value& at(const Key& key) {
    auto iter = find(key);
    if (iter != end()) {
      return iter->second;
    }
    throw std::out_of_range("Key not found");
  }

  const Value& at(const Key& key) const {
    auto iter = find(key);
    if (iter != end()) {
      return iter->value.second;
    }
    throw std::out_of_range("Key not found");
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&& ... args) {
    auto alloc = elements_.get_node_allocator();
    auto* node = ListNodeAllocTraits::allocate(alloc, 1);
    NodeType* position = &(node->data.value);
    try {
      AllocTraits::construct(alloc_, position, std::forward<Args>(args)...);
      node->data.hash = hash_func_(node->data.value.first);

      if (find(node->data.value.first) != end()) {
        auto alloc = elements_.get_node_allocator();
        auto path = find(node->data.value.first);
        AllocTraits::destroy(alloc_, position);
        ListNodeAllocTraits::deallocate(alloc, node, 1);
        return {path, false};
      }

      if (get_load_factor(1) > max_load_factor_) {
        rehash((elements_.size() * 2) + 2);
      }
      NodeIter next = iters_[node->data.hash % iters_.size()];
      if (next.isNull()) {
        elements_.emplace_node_front(node);
        iters_[node->data.hash % iters_.size()] = elements_.begin();
        return {elements_.begin(), true};
      }
      iters_[node->data.hash % iters_.size()] = elements_.emplace_node(next,
                                                                       node);
      return {iters_[node->data.hash % iters_.size()], true};
    } catch (...) {
      AllocTraits::destroy(alloc_, position);
      ListNodeAllocTraits::deallocate(alloc, node, 1);
      throw;
    }
  }

  Value& operator[](const Key& key) {
    iterator iter = find(key);
    if (iter == end()) {
      return emplace(std::piecewise_construct,
                     std::forward_as_tuple(key),
                     std::forward_as_tuple()).first->second;
    }
    return iter->second;
  }

  void rehash(size_t new_size) {
    List<Node, NodeAlloc> cpy(std::move(elements_));
    iters_.assign(new_size, elements_.end());
    auto next = cpy.sentinel.next->next;

    for (auto bnode = cpy.sentinel.next;
         bnode != &cpy.sentinel; bnode = next) {
      next = bnode->next;
      auto node = static_cast<List<Node, NodeAlloc>::Node*>(bnode);
      auto hash = node->data.hash;
      if (iters_[hash % iters_.size()] == elements_.end()) {
        elements_.emplace_node_front(node);
        iters_[hash % iters_.size()] = elements_.begin();
      } else {
        iters_[hash % iters_.size()] = elements_.emplace_node(iters_[
                                                                      hash %
                                                                      iters_.size()],
                                                              node);
      }
    }
    cpy.size_ = 0;
  }

  void reserve(size_t count) {
    if (std::ceil(count / max_load_factor_) < elements_.size()) {
      return;
    }
    rehash(std::ceil(count / max_load_factor_));
  }

  void erase(iterator iter) {
    if (iter == end()) return;
    size_t bucket = iter.hash() % iters_.size();
    if (iters_[bucket] == iter.get_iter()) {
      auto next = iter;
      ++next;
      if (next != end() && next.hash() % iters_.size() == bucket) {
        iters_[bucket] = next.get_iter();
      } else {
        iters_[bucket] = elements_.end();
      }
    }
    elements_.erase(iter.get_iter());
  }

  void erase(iterator begin, iterator end) {
    auto next = begin;
    for (auto iter = begin; iter != end; iter = next) {
      next = std::next(iter);
      erase(iter);
    }
  }

  std::pair<iterator, bool> insert(const NodeType& node) {
    return emplace(std::move(node));
  }

  std::pair<iterator, bool> insert(NodeType&& node) {
    return emplace(std::piecewise_construct,
                   std::forward_as_tuple(std::move(node.first)),
                   std::forward_as_tuple(std::move(node.second)));
  }

  template<class P>
  std::pair<iterator,bool> insert(P&& node) {
    return emplace(std::forward<P>(node));
  }

  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    auto iter = first;
    while (iter != last) {
      insert(*iter);
      iter++;
    }
  }


  void clear() {
    UnorderedMap tmp;
    swap(tmp);
  }

  size_t size() {
    return elements_.size();
  }

  bool empty() {
    return elements_.size() == 0;
  }

  iterator begin() noexcept { return iterator(elements_.begin()); }

  const_iterator begin() const noexcept { return const_iterator(elements_.begin()); }

  const_iterator cbegin() const noexcept { return const_iterator(elements_.cbegin()); }

  iterator end() noexcept { return iterator(elements_.end()); }

  const_iterator end() const noexcept { return const_iterator(elements_.end()); }

  const_iterator cend() const noexcept { return const_iterator(elements_.cend()); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }


 private:
  void force_swap(UnorderedMap& other) {
    std::swap(hash_func_, other.hash_func_);
    std::swap(eq_func_, other.eq_func_);
    std::swap(alloc_, other.alloc_);
    elements_.swap(other.elements_);
    std::swap(iters_, other.iters_);
    std::swap(max_load_factor_, other.max_load_factor_);
  };

  [[no_unique_address]] Hash hash_func_{};
  [[no_unique_address]] KeyEqual eq_func_{};
  [[no_unique_address]] Alloc alloc_{};
  List<Node, NodeAlloc> elements_{};
  std::vector<NodeIter, NodeIterAlloc> iters_{8, elements_.end(), alloc_};
  double max_load_factor_ = 1.0;
};

template <typename Key, typename Value, typename Hash, typename KeyEqual,
        typename Alloc>
template <bool IsConst>
class UnorderedMap<Key, Value, Hash, KeyEqual, Alloc>::Iterator {
  using IterType =
          std::conditional_t<IsConst, typename NodeList::const_iterator,
                  typename NodeList::iterator>;
 public:
  using value_type = std::conditional_t<IsConst, const NodeType, NodeType>;
  using reference =
          typename std::conditional_t<IsConst, const NodeType&, NodeType&>;
  using pointer =
          typename std::conditional_t<IsConst, const NodeType*, NodeType*>;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = int;

  Iterator(const IterType& list_iter) : iter(list_iter) {};

  Iterator(const Iterator&) noexcept = default;

  Iterator& operator=(const Iterator& other) = default;

  template <bool OtherIsConst>
  Iterator(const Iterator<OtherIsConst>& other) : iter(other.iter) {}

  template <bool OtherIsConst>
  Iterator& operator=(const Iterator<OtherIsConst>& other) noexcept {
    iter = other.iter;
    return *this;
  }

  reference operator*() const {
    return iter->value;
  }

  pointer operator->() const {
    return &(iter->value);
  }

  Iterator& operator++() {
    ++iter;
    return *this;
  }

  Iterator operator++(int) {
    return Iterator(iter++);
  }

  template <bool OtherIsConst>
  bool operator==(const Iterator<OtherIsConst>& other) {
    return iter == other.iter;
  }

  template <bool OtherIsConst>
  bool operator!=(const Iterator<OtherIsConst>& other) {
    return iter != other.iter;
  }

  size_t hash() {
    return iter->hash;
  }

  IterType get_iter() {
    return iter;
  }

 private:
  IterType iter;

  template <bool D>
  friend
  class Iterator;
};


