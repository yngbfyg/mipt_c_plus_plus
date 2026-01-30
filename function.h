#pragma once
//
#include <utility>
#include <type_traits>
#include <functional>
#include <new>
#include <cstddef>
#include <typeinfo>

template<bool MoveOnly, class>
struct BasicFunction;

template<class>
struct Function;

template<class>
struct MoveOnlyFunction;

template<bool MoveOnly, typename ReturnType, typename... Args>
struct BasicFunction<MoveOnly, ReturnType(Args...)> {
 private:
  alignas(std::max_align_t) std::byte heap[alignof(std::max_align_t)];

  template<class T>
  static constexpr bool fits() noexcept { return sizeof(T) <= alignof(std::max_align_t); }

  void *func = nullptr;

  ReturnType (*invoke)(void *, Args &&...) = nullptr;

  void (*copy)(BasicFunction &, const BasicFunction &) = nullptr;

  void (*move)(BasicFunction &, BasicFunction &) noexcept = nullptr;

  void (*destroy)(BasicFunction &) noexcept = nullptr;

  const std::type_info* type = nullptr;

  template<class Func>
  static ReturnType invoke_impl_(void *h, Args &&... args) { return std::invoke(*static_cast<Func *>(h), std::forward<Args>(args)...); }

  template<class Func>
  static void destroy_impl_(BasicFunction &s) noexcept {
    if (s.func == s.heap) static_cast<Func *>(s.func)->~Func();
    else delete static_cast<Func *>(s.func);
  }

  template<class Func>
  static void move_impl_(BasicFunction &oth, BasicFunction &prev) noexcept {
    if constexpr (fits<Func>()) {
      oth.func = oth.heap;
      new(oth.func) Func(std::move(*static_cast<Func *>(prev.func)));
      static_cast<Func *>(prev.func)->~Func();
    } else {
      oth.func = prev.func;
    }
    oth.setMethods<Func>();
    prev.func = nullptr, prev.invoke = nullptr, prev.copy = nullptr, prev.move = nullptr, prev.destroy = nullptr, prev.type = nullptr;
  }

  template<class Func>
  static void copy_impl_(BasicFunction &oth, const BasicFunction &prev) {
    if constexpr (fits<Func>()) {
      oth.func = oth.heap;
      new(oth.func) Func(*static_cast<const Func *>(prev.func));
    } else {
      oth.func = new Func(*static_cast<const Func *>(prev.func));
    }
    oth.setMethods<Func>();
  }

  friend bool operator==(const BasicFunction &f, std::nullptr_t) noexcept { return !f; }
  friend bool operator==(std::nullptr_t, const BasicFunction &f) noexcept { return !f; }
  friend bool operator!=(const BasicFunction &f, std::nullptr_t) noexcept { return !!f; }
  friend bool operator!=(std::nullptr_t, const BasicFunction &f) noexcept { return !!f; }

  void clear() {
    if (destroy) destroy(*this);
    func = nullptr, invoke = nullptr, copy = nullptr, move = nullptr, destroy = nullptr, type = nullptr;
  }

  template<class Func>
  void setMethods() noexcept {
    invoke = &invoke_impl_<Func>;
    move = &move_impl_<Func>;
    destroy = &destroy_impl_<Func>;
    if constexpr (!MoveOnly) copy = &copy_impl_<Func>;
    else copy = nullptr;
    type = &typeid(Func);
  }

 public:
  BasicFunction() noexcept = default;
  BasicFunction(std::nullptr_t) noexcept { clear(); }

  template<class Callable, class Func = std::decay_t<Callable> > requires (
  !std::is_base_of_v<BasicFunction, Func > &&
          std::is_invocable_r_v<ReturnType, Func &, Args...> &&
  (MoveOnly || std::is_copy_constructible_v<Func>)
  )
  BasicFunction(Callable &&callable) {
    if constexpr (fits<Func>()) {
      func = heap;
      new(func) Func(std::forward<Callable>(callable));
    } else {
      func = new Func(std::forward<Callable>(callable));
    }
    setMethods<Func>();
  }

  BasicFunction(const BasicFunction &oth) requires (!MoveOnly) {
    if (!oth) {
      clear();
      return;
    }
    oth.copy(*this, oth);
  }

  BasicFunction(const BasicFunction &) requires (MoveOnly) = delete;

  BasicFunction(BasicFunction &&oth) noexcept {
    if (!oth) {
      clear();
      return;
    }
    oth.move(*this, oth);
  }

  BasicFunction &operator=(std::nullptr_t) noexcept { return clear(), *this; }

  BasicFunction &operator=(const BasicFunction &oth) requires (!MoveOnly) {
    if (this == &oth) return *this;
    clear();
    if (oth) oth.copy(*this, oth);
    return *this;
  }

  BasicFunction &operator=(const BasicFunction &) requires (MoveOnly) = delete;

  BasicFunction &operator=(BasicFunction &&oth) noexcept {
    if (this == &oth) return *this;
    clear();
    if (oth) oth.move(*this, oth);
    return *this;
  }

  ~BasicFunction() noexcept {
    if (destroy) destroy(*this);
    func = nullptr, invoke = nullptr, copy = nullptr, move = nullptr, destroy = nullptr, type = nullptr;
  }

  ReturnType operator()(Args... args) const {
    if (!invoke) throw std::bad_function_call();
    return invoke(func, std::forward<Args>(args)...);
  }

  explicit operator bool() const noexcept { return invoke; }

  const std::type_info& target_type() const noexcept {
    if (type) return *type;
    return typeid(void);
  }

  template<class T>
  T* target() noexcept {
    if (type && *type == typeid(T)) return static_cast<T*>(func);
    return nullptr;
  }

  template<class T>
  const T* target() const noexcept {
    if (type && *type == typeid(T)) return static_cast<const T*>(func);
    return nullptr;
  }
};

template<typename ReturnType, typename... Args>
struct Function<ReturnType(Args...)> : BasicFunction<false, ReturnType(Args...)> {
  using BasicFunction<false, ReturnType(Args...)>::BasicFunction;
};


template<typename ReturnType, typename... Args>
struct MoveOnlyFunction<ReturnType(Args...)> : BasicFunction<true, ReturnType(Args...)> {
  using BasicFunction<true, ReturnType(Args...)>::BasicFunction;
};

template<class T>
struct __function;

template<class Class, class ReturnType, class... Args>
struct __function<ReturnType(Class::*)(Args...) const> {
  using type = ReturnType(Args...);
};

template<class Class, class ReturnType, class... Args>
struct __function<ReturnType(Class::*)(Args...)> {
  using type = ReturnType(Args...);
};

template<class ReturnType, class... Args>
Function(ReturnType (*)(Args...)) -> Function<ReturnType(Args...)>;

template<class Func>
Function(Func) -> Function<typename __function<decltype(&Func::operator())>::type>;

// template<class ReturnType, class... Args>
// MoveOnlyFunction(ReturnType (*)(Args...)) -> MoveOnlyFunction<ReturnType(Args...)>;

template<class Func>
MoveOnlyFunction(Func) -> MoveOnlyFunction<typename __function<decltype(&Func::operator())>::type>;
