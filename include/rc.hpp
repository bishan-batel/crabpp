// ReSharper disable CppNonExplicitConversionOperator
#pragma once

#include <preamble.hpp>

#include "ref.hpp"

namespace crab::rc::helper {}

template<typename T> requires (not std::is_array_v<T>)
class Rc final {
  T *data;
  usize *ref_count;

  explicit Rc(T *data, usize *ref_count)
    : data(data), ref_count(ref_count) {
    debug_assert(data != nullptr, "Corrupted Rc<T>, data is nullptr");
    debug_assert(ref_count != nullptr, "Corrupted Rc<T>, ref_count is nullptr");
    // ReSharper disable once CppDFANullDereference
    *ref_count += 1;
  }

  void release() {
    if (ref_count) {
      debug_assert(data != nullptr, "Corrupted Rc<T>, missing data but kept reference count");
      *ref_count -= 1;

      if (*ref_count == 0) {
        delete data;
        delete ref_count;
      }
      return;
    }
    debug_assert(data == nullptr, "Corrupted Rc<T>, missing reference count but kept data");
  }

public:
  explicit Rc(T &&value) : Rc(new T{value}, new usize{0}) {}

  [[nodiscard]]
  Rc from_owned_unchecked(const T *box) { return Rc(box, new usize{0}); }

  Rc(const Rc &from)
    : Rc(from.data, from.ref_count) {}

  Rc(Rc &&from) noexcept
    : Rc(
      std::exchange(from.data, nullptr),
      std::exchange(from.ref_count, nullptr)
    ) {
    debug_assert(data != nullptr, "Corrupted Rc<T>, data is nullptr while moved");
    debug_assert(ref_count != nullptr, "Corrupted Rc<T>, ref_count is nullptr while moved");
  }

  Rc &operator=(const Rc &from) {
    release();
    data = from.data;
    ref_count = from.ref_count;
    debug_assert(data != nullptr, "Corrupted Rc<T>, data is nullptr while copied");
    debug_assert(ref_count != nullptr, "Corrupted Rc<T>, ref_count is nullptr while copied");
    return *this;
  }

  Rc &operator=(Rc &&from) noexcept {
    release();
    data = std::exchange(from.data, nullptr);
    ref_count = std::exchange(from.ref_count, nullptr);
    debug_assert(data != nullptr, "Corrupted Rc<T>, data is nullptr while moved");
    debug_assert(ref_count != nullptr, "Corrupted Rc<T>, ref_count is nullptr while moved");
    return *this;
  }

  ~Rc() {
    release();
  }

  operator const T &() const { return *raw_ptr(); }

  operator const T *() const { return raw_ptr(); }

  operator const Ref<T>() const { return as_ref(); }

  [[nodiscard]] Ref<T> as_ref() const { return *raw_ptr(); }

private:
  const T *raw_ptr() const {
    debug_assert(data != nullptr, "Corrupted Rc<T>, data is nullptr");
    debug_assert(ref_count != nullptr, "Corrupted Rc<T>, ref_count is nullptr");
    return data;
  }
};

namespace crab::rc {
  template<typename T, typename... Args> requires std::is_constructible_v<T, Args...>
  Rc<T> make(Args... args) {
    return Rc<T>::from_owned_unchecked(new T{args...});
  }
}