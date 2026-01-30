#pragma once
template<typename... Ti>
struct Tuple;

template<>
struct Tuple<> {
  Tuple() = default;
};

template<std::size_t I, class Head, class... Tail>
decltype(auto) get(const Tuple<Head, Tail...> &t) {
  if constexpr (I == 0) return (t.head);
  else return get<I - 1>(t.tail);
}

template<std::size_t I, class Head, class... Tail>
decltype(auto) get(Tuple<Head, Tail...> &t) {
  if constexpr (I == 0) return (t.head);
  else return get<I - 1>(t.tail);
}

template<std::size_t I, class Head, class... Tail>
decltype(auto) get(Tuple<Head, Tail...> &&t) {
  if constexpr (I == 0) return std::forward<Head>(t.head);
  else return get<I - 1>(std::move(t.tail));
}

template<class T, class Head, class... Tail>
requires (std::is_same_v<T, Head> + (0 + ... + (std::is_same_v<T, Tail> ? 1 : 0)) == 1)
decltype(auto) get(Tuple<Head, Tail...> &t) {
  if constexpr (std::is_same_v<T, Head>) return (t.head);
  else return get<T>(t.tail);
}

template<class T, class Head, class... Tail>
requires (std::is_same_v<T, Head> + (0 + ... + (std::is_same_v<T, Tail> ? 1 : 0)) == 1)
decltype(auto) get(const Tuple<Head, Tail...> &t) {
  if constexpr (std::is_same_v<T, Head>) return (t.head);
  else return get<T>(t.tail);
}

template<class T, class Head, class... Tail>
requires (std::is_same_v<T, Head> + (0 + ... + (std::is_same_v<T, Tail> ? 1 : 0)) == 1)
decltype(auto) get(Tuple<Head, Tail...> &&t) {
  if constexpr (std::is_same_v<T, Head>) return std::forward<Head>(t.head);
  else return get<T>(std::move(t.tail));
}

template<std::size_t I, class Head, class... Tail>
decltype(auto) get(const Tuple<Head, Tail...> &&t) {
  if constexpr (I == 0) {
    if constexpr (std::is_lvalue_reference_v<Head>) return (t.head);
    else return std::move(t.head);
  } else return get<I - 1>(std::move(t.tail));
}

template<class T, class Head, class... Tail>
requires (std::is_same_v<T, Head> + (0 + ... + (std::is_same_v<T, Tail> ? 1 : 0)) == 1)
decltype(auto) get(const Tuple<Head, Tail...> &&t) {
  if constexpr (std::is_same_v<T, Head>) {
    if constexpr (std::is_lvalue_reference_v<Head>) return (t.head);
    else return std::move(t.head);
  } else return get<T>(std::move(t.tail));
}

template<typename Head, typename... Tail>
struct Tuple<Head, Tail...> {
  Head head;
  Tuple<Tail...> tail;

  Tuple(Tuple &&oth)  noexcept requires (std::is_move_constructible_v<Head> && (std::is_move_constructible_v<Tail> && ...)): head(std::move(oth.head)), tail(std::move(oth.tail)) {}

  Tuple(const Tuple &) requires (std::is_copy_constructible_v<Head> && (std::is_copy_constructible_v<Tail> && ...)) = default;

  Tuple(const Tuple &) requires (!(std::is_copy_constructible_v<Head> && (std::is_copy_constructible_v<Tail> && ...))) = delete;

  Tuple() requires (std::is_default_constructible_v<Head> && (std::is_default_constructible_v<Tail> && ...)) : head(), tail() {}

  template<class UHead, class... UTail>
  explicit(!(std::is_convertible_v<UHead &&, Head> && (std::is_convertible_v<UTail &&, Tail> && ...)))
  Tuple(Tuple<UHead, UTail...> &&oth) requires ((sizeof...(UTail) == sizeof...(Tail)) && std::is_constructible_v<Head, UHead &&> && (std::is_constructible_v<Tail, UTail &&> && ...)): head(std::forward<UHead>(oth.head)), tail(std::move(oth.tail)) {}

  Tuple(const Head &h, const Tail &... t) requires (std::is_copy_constructible_v<Head> && (std::is_copy_constructible_v<Tail> && ...)) : head(h), tail(t...) {}


  template<class UHead, class... UTail>
  explicit(!(std::is_convertible_v<const UHead &, Head> && (std::is_convertible_v<const UTail &, Tail> && ...)))
  Tuple(const Tuple<UHead, UTail...> &oth) requires ((sizeof...(UTail) == sizeof...(Tail)) && std::is_constructible_v<Head, const UHead &> && (std::is_constructible_v<Tail, const UTail &> && ...)): head(oth.head), tail(oth.tail) {}

  template<typename UHead, typename... UTail>
  explicit (!(std::is_convertible_v<UHead &&, Head> && (std::is_convertible_v<UTail &&, Tail> && ...))) Tuple(UHead &&h, UTail &&... t) requires ((sizeof...(UTail) == sizeof...(Tail)) && (sizeof...(Tail) > 0) && std::is_constructible_v<Head, UHead &&> && (std::is_constructible_v<Tail, UTail &&> && ...)): head(std::forward<UHead>(h)), tail(std::forward<UTail>(t)...) {}


  template<typename UHead>
  explicit(!std::is_convertible_v<UHead &&, Head>) Tuple(UHead &&h) requires (sizeof...(Tail) == 0 && std::is_constructible_v<Head, UHead &&>) : head(std::forward<UHead>(h)) {}

  Tuple &operator=(const Tuple &oth) requires (std::is_copy_assignable_v<Head> && (std::is_copy_assignable_v<Tail> && ...)) { return head = oth.head, tail = oth.tail, *this; }


  Tuple &operator=(Tuple &&oth)  noexcept requires (std::is_move_assignable_v<Head> && (std::is_move_assignable_v<Tail> && ...)) { return head = std::forward<Head>(oth.head), tail = std::move(oth.tail), *this; }


  template<class UHead, class... UTail>
  Tuple &operator=(Tuple<UHead, UTail...> &oth) requires (sizeof...(UTail) == sizeof...(Tail) && std::is_assignable_v<Head &, UHead &> && (std::is_assignable_v<Tail &, UTail &> && ...)) { return head = oth.head, tail = oth.tail, *this; }

  template<class UHead, class... UTail>
  Tuple &operator=(Tuple<UHead, UTail...> &&oth) requires (sizeof...(UTail) == sizeof...(Tail) && std::is_assignable_v<Head &, UHead &&> && (std::is_assignable_v<Tail &, UTail &&> && ...)) { return head = std::forward<UHead>(oth.head), tail = std::move(oth.tail), *this; }

  template<class T1, class T2>
  Tuple &operator=(std::pair<T1, T2> &&p) requires (sizeof...(Tail) == 1 && std::is_assignable_v<Head &, T1 &&> && std::is_assignable_v<decltype((tail.head)), T2 &&>) { return head = std::move(p.first), tail.head = std::move(p.second), *this; }

  template<class U1, class U2>
  explicit(!(std::is_convertible_v<const U1 &, Head> && std::is_convertible_v<const U2 &, Tuple<Tail...> >))
  Tuple(const std::pair<U1, U2> &p) requires ((sizeof...(Tail) == 1) && std::is_constructible_v<Head, const U1 &> && std::is_constructible_v<Tuple<Tail...>, const U2 &>): head(p.first), tail(p.second) {}

  template<class U1, class U2>
  explicit(!(std::is_convertible_v<U1 &&, Head> && std::is_convertible_v<U2 &&, Tuple<Tail...> >))
  Tuple(std::pair<U1, U2> &&p) requires ((sizeof...(Tail) == 1) && std::is_constructible_v<Head, U1 &&> && std::is_constructible_v<Tuple<Tail...>, U2 &&>): head(std::forward<U1>(p.first)), tail(std::forward<U2>(p.second)) {}

  Tuple &operator=(const Tuple &oth) requires (!(std::is_copy_assignable_v<Head> && (std::is_copy_assignable_v<Tail> && ...))) = delete;
};


template<class T1, class T2>
Tuple(const std::pair<T1, T2> &) -> Tuple<T1, T2>;

template<class T1, class T2>
Tuple(std::pair<T1, T2> &) -> Tuple<T1, T2>;

template<class T1, class T2>
Tuple(std::pair<T1, T2> &&) -> Tuple<T1 &&, T2 &&>;


template<class... Ti>
constexpr auto makeTuple(Ti &&... t) { return Tuple<std::unwrap_ref_decay_t<Ti>...>(std::forward<Ti>(t)...); }

template<class... Ti>
constexpr auto tie(Ti &... t) noexcept { return Tuple<Ti &...>(t...); }

template<class... Ti>
constexpr auto forwardAsTuple(Ti &&... t) noexcept { return Tuple<Ti &&...>(std::forward<Ti>(t)...); }


template<class T>
struct tuple_size;

template<class... Ti>
struct tuple_size<Tuple<Ti...> > : std::integral_constant<std::size_t, sizeof...(Ti)> {};

template<class T>
inline constexpr std::size_t tuple_size_v = tuple_size<std::remove_cvref_t<T> >::value;

template<std::size_t K, class T>
constexpr decltype(auto) cat_get(T &&t) { return get<K>(std::forward<T>(t)); }

template<std::size_t K, class T1, class T2, class... Oths>
constexpr decltype(auto) cat_get(T1 &&t1, T2 &&t2, Oths &&... rest) {
  if constexpr (K < tuple_size_v<T1>) return get<K>(std::forward<T1>(t1));
  else return cat_get<K - tuple_size_v<T1>>(std::forward<T2>(t2), std::forward<Oths>(rest)...);
}

template<class... Tuples, std::size_t... Id>
constexpr auto tupleCat_impl(std::index_sequence<Id...>, Tuples &&... tuples) { return makeTuple(cat_get<Id>(std::forward<Tuples>(tuples)...)...); }

template<class... Tuples>
constexpr auto catTuple(Tuples &&... tuples) { return tupleCat_impl(std::make_index_sequence<(0 + ... + tuple_size_v<Tuples>)>{}, std::forward<Tuples>(tuples)...); }

template<class... Tuples>
constexpr auto tupleCat(Tuples &&... tuples) { return catTuple(std::forward<Tuples>(tuples)...); }

