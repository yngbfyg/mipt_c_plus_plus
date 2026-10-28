# C++ Data Structures & Standard Library Implementations

**MIPT C++ / Object-Oriented Programming coursework**

A collection of educational C++ assignments focused on implementing data structures and familiar standard-library abstractions from scratch. The exercises explore templates, memory management, object lifetime, iterators, and generic programming.

## Implementations

| File | Implementation |
| --- | --- |
| [`biginteger.h`](biginteger.h) | Arbitrary-precision `BigInteger` and rational-number `Rational` arithmetic |
| [`string.h`](string.h) | Dynamically allocated `String` class and string operations |
| [`shared_ptr.h`](shared_ptr.h) | Reference-counted `SharedPtr`, `WeakPtr`, and `EnableSharedFromThis` |
| [`stackallocator.h`](stackallocator.h) | Stack-based storage and allocator, plus an allocator-aware doubly linked `List` |
| [`unordered_map.h`](unordered_map.h) | Hash-table-based `UnorderedMap` and supporting linked-list implementation |
| [`circular_buffer.h`](circular_buffer.h) | `CircularBuffer` with fixed or dynamic capacity and iterator support |
| [`tuple.h`](tuple.h) | Variadic-template `Tuple` and element access utilities |
| [`function.h`](function.h) | Callable wrappers `Function` and `MoveOnlyFunction` using type erasure |

## Concepts explored

- **Generic programming:** class and function templates, variadic templates, type traits, and C++20 constraints.
- **Memory and ownership:** custom allocators, placement `new`, object lifetime, copy/move semantics, and reference counting.
- **Data structures:** linked lists, hash tables, circular buffers, and random-access/bidirectional iterators.
- **Operator overloading and abstractions:** arithmetic types, custom string operations, and callable objects.

## Notes

These are **independent coursework solutions**, not a single integrated library or drop-in replacements for the C++ standard library. Some headers define similarly named helper types, and individual tasks may have different assumptions and dependencies. There is no unified build system or test suite included in this repository.

**Language:** C++ (some implementations use C++20 features).

**Context:** Object-Oriented Programming and C++ coursework at the Moscow Institute of Physics and Technology (MIPT).
