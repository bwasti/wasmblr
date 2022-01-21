#pragma once

#ifdef ASSERT
#include <cassert>
#else
#define assert(x)
#endif
#include <cstdlib>
#include <functional>
#include <iterator>

namespace wasmblr {

template <typename T> struct vector_iterator {
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = T;
  using pointer = T *;
  using reference = T &;
  vector_iterator(pointer ptr) : ptr_(ptr) {}
  reference operator*() const { return *ptr_; }
  pointer operator->() { return ptr_; }

  vector_iterator &operator++() {
    ptr_++;
    return *this;
  }
  vector_iterator operator++(int) {
    vector_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  friend bool operator==(const vector_iterator &a, const vector_iterator &b) {
    return a.ptr_ == b.ptr_;
  };
  friend bool operator!=(const vector_iterator &a, const vector_iterator &b) {
    return a.ptr_ != b.ptr_;
  };

private:
  pointer ptr_;
};

template <typename T> struct vector {
  T *data_ = nullptr;
  uint32_t size_ = 0;
  uint32_t avail_ = 0;
  vector() {}
  vector(std::initializer_list<T> elems) {
    for (const auto &elem : elems) {
      emplace_back(elem);
    }
  }
  ~vector() {
    if (data_) {
      free(data_);
    }
  }
  void increase_avail() {
    auto new_avail = avail_ * 2;
    if (data_) {
      data_ = (T *)realloc(data_, new_avail * sizeof(T));
    } else {
      new_avail = 1;
      data_ = (T *)malloc(sizeof(T));
    }
    avail_ = new_avail;
  }

  void emplace_back(const T &elem) {
    if (size_ == avail_) {
      increase_avail();
    }
    data_[size_] = elem;
  }

  vector_iterator<T> begin() { return vector_iterator<T>(data_); }
  vector_iterator<T> end() { return vector_iterator<T>(&data_[size_]); }
  vector_iterator<const T> begin() const {
    return vector_iterator<const T>(data_);
  }
  vector_iterator<const T> end() const {
    return vector_iterator<const T>(&data_[size_]);
  }
  uint32_t size() const { return size_; }
  T *data() { return data_; }
  const T &at(uint32_t i) const {
    assert(size_ > i);
    return data_[i];
  }

  void clear() {
    if (data_) {
      free(data_);
    }
    avail_ = 0;
    data_ = nullptr;
    size_ = 0;
  }

  T &back() const {
    assert(size_);
    return data_[size_ - 1];
  }

  void pop_back() {
    assert(size_);
    size_--;
  }
};

namespace detail {

struct string {
  vector<char> data;
  string() = default;
  template <int N> string(const char (&str)[N]) {
    for (auto i = 0; i < N; ++i) {
      data.emplace_back(str[i]);
    }
  }
  uint32_t size() const { return data.size(); }
};

template <typename A, typename B> struct pair {
  pair(const A &f, const B &s) : first(f), second(s) {}
  A first;
  B second;
};

}; // namespace detail

struct CodeGenerator;

class Local {
public:
  int operator()(uint8_t type);
  void get(int idx);
  void set(int idx);
  void tee(int idx);

private:
  Local(CodeGenerator &cg_) : cg(cg_) {}
  CodeGenerator &cg;
  friend CodeGenerator;
};

class I32 {
public:
  operator uint8_t();
  void const_(int32_t i);
  void clz();
  void ctz();
  void popcnt();
  void lt_s();
  void lt_u();
  void gt_s();
  void gt_u();
  void le_s();
  void le_u();
  void ge_s();
  void ge_u();
  void add();
  void sub();
  void mul();
  void div_s();
  void div_u();
  void rem_s();
  void rem_u();
  void and_();
  void or_();
  void xor_();
  void shl();
  void shr_s();
  void shr_u();
  void rotl();
  void rotr();
  void eqz();
  void eq();
  void ne();

  void load(uint32_t alignment = 1, uint32_t offset = 0);
  void store(uint32_t alignment = 1, uint32_t offset = 0);

  void load8_s(uint32_t alignment = 1, uint32_t offset = 0);
  void load8_u(uint32_t alignment = 1, uint32_t offset = 0);
  void load16_s(uint32_t alignment = 1, uint32_t offset = 0);
  void load16_u(uint32_t alignment = 1, uint32_t offset = 0);
  void store8(uint32_t alignment = 1, uint32_t offset = 0);
  void store16(uint32_t alignment = 1, uint32_t offset = 0);

private:
  I32(CodeGenerator &cg_) : cg(cg_) {}
  CodeGenerator &cg;
  friend CodeGenerator;
};

class F32 {
public:
  operator uint8_t();
  void const_(float f);
  void eq();
  void ne();
  void lt();
  void gt();
  void le();
  void ge();
  void abs();
  void neg();
  void ceil();
  void floor();
  void trunc();
  void nearest();
  void sqrt();
  void add();
  void sub();
  void mul();
  void div();
  void min();
  void max();
  void copysign();

  void load(uint32_t alignment = 1, uint32_t offset = 0);
  void store(uint32_t alignment = 1, uint32_t offset = 0);

private:
  F32(CodeGenerator &cg_) : cg(cg_) {}
  CodeGenerator &cg;
  friend CodeGenerator;
};

class V128 {
public:
  operator uint8_t();

  void i32x4_extract_lane(uint8_t lane);
  void i32x4_replace_lane(uint8_t lane);
  void f32x4_extract_lane(uint8_t lane);
  void f32x4_replace_lane(uint8_t lane);
  void i32x4_splat();
  void f32x4_splat();
  void i32x4_eq();
  void i32x4_ne();
  void i32x4_lt_s();
  void i32x4_lt_u();
  void i32x4_gt_s();
  void i32x4_gt_u();
  void i32x4_le_s();
  void i32x4_le_u();
  void i32x4_ge_s();
  void i32x4_ge_u();
  void f32x4_eq();
  void f32x4_ne();
  void f32x4_lt();
  void f32x4_gt();
  void f32x4_le();
  void f32x4_ge();
  void not_();
  void any_true();
  void and_();
  void andnot();
  void or_();
  void xor_();
  void i32x4_abs();
  void i32x4_neg();
  void i32x4_all_true();
  void i32x4_bitmask();
  void i32x4_shl();
  void i32x4_shr_s();
  void i32x4_shr_u();
  void i32x4_add();
  void i32x4_sub();
  void i32x4_mul();
  void i32x4_min_s();
  void i32x4_min_u();
  void i32x4_max_s();
  void i32x4_max_u();
  void f32x4_ceil();
  void f32x4_floor();
  void f32x4_trunc();
  void f32x4_nearest();
  void f32x4_abs();
  void f32x4_neg();
  void f32x4_sqrt();
  void f32x4_add();
  void f32x4_sub();
  void f32x4_mul();
  void f32x4_div();
  void f32x4_min();
  void f32x4_max();
  void f32x4_pmin();
  void f32x4_pmax();

  void load(uint32_t alignment = 1, uint32_t offset = 0);
  void load32x2_s(uint32_t alignment = 1, uint32_t offset = 0);
  void load32x2_u(uint32_t alignment = 1, uint32_t offset = 0);
  void load32_splat(uint32_t alignment = 1, uint32_t offset = 0);
  void load32_zero(uint32_t alignment = 1, uint32_t offset = 0);
  void store(uint32_t alignment = 1, uint32_t offset = 0);

private:
  V128(CodeGenerator &cg_) : cg(cg_) {}
  CodeGenerator &cg;
  friend CodeGenerator;
};

class Memory {
public:
  Memory &operator()(uint32_t min);
  Memory &operator()(uint32_t min, uint32_t max);
  Memory &export_(detail::string);
  Memory &shared(bool = true);
  Memory &import_(detail::string, detail::string);
  void size();
  void grow();

private:
  Memory(CodeGenerator &cg_) : cg(cg_) {}
  CodeGenerator &cg;
  uint32_t min = 0;
  uint32_t max = 0;
  bool is_shared = false;
  detail::string a_string;
  detail::string b_string;
  bool is_import() const { return a_string.size() && b_string.size(); }
  bool is_export() const { return a_string.size() && !b_string.size(); }
  friend CodeGenerator;
};

struct Function {
  Function(vector<uint8_t> input_types_, vector<uint8_t> output_types_)
      : input_types(input_types_), output_types(output_types_) {}
  Function(vector<uint8_t> input_types_, vector<uint8_t> output_types_,
           std::function<void()> body_)
      : input_types(input_types_), output_types(output_types_), body(body_) {}
  vector<uint8_t> input_types;
  vector<uint8_t> output_types;
  std::function<void()> body;
  vector<uint8_t> locals; // resolved later
  void emit() {
    locals.clear();
    body();
  };
};

struct CodeGenerator {
  // API
  Local local;
  I32 i32;
  F32 f32;
  V128 v128;
  Memory memory;
  uint8_t void_ = 0x40;

  void nop();
  void block(uint8_t type);
  void loop(uint8_t type);
  void if_(uint8_t type);
  void else_();
  void br(uint32_t labelidx);
  void br_if(uint32_t labelidx);
  void end();
  void call(uint32_t funcidx);

  void export_(uint32_t fn_idx, detail::string name);

  // returns function index
  uint32_t function(vector<uint8_t> input_types, vector<uint8_t> output_types,
                    std::function<void()> body);

  vector<uint8_t> emit();

  // Implementation

  CodeGenerator()
      : local(*this), i32(*this), f32(*this), v128(*this), memory(*this) {}
  CodeGenerator(const CodeGenerator &) = delete;
  CodeGenerator(CodeGenerator &&) = delete;

  vector<Function> functions_;
  vector<detail::pair<uint32_t, detail::string>> exported_functions_;
  Function *cur_function_ = nullptr;
  // cur_bytes_ is used as a temporary storage
  vector<uint8_t> cur_bytes_;
  // a running type checker, purely for safety
  vector<uint8_t> type_stack_;

  using memarg = detail::pair<uint32_t, uint32_t>;

  // From LLVM
  vector<uint8_t> encode_signed(int32_t n) {
    vector<uint8_t> out;
    auto more = true;
    do {
      uint8_t byte = n & 0x7f;
      n >>= 7;
      more = !((((n == 0) && ((byte & 0x40) == 0)) ||
                ((n == -1) && ((byte & 0x40) != 0))));
      if (more) {
        byte |= 0x80;
      }
      out.emplace_back(byte);
    } while (more);
    return out;
  }

  vector<uint8_t> encode_unsigned(uint32_t n) {
    vector<uint8_t> out;
    do {
      uint8_t byte = n & 0x7f;
      n >>= 7;
      if (n != 0) {
        byte |= 0x80;
      }
      out.emplace_back(byte);
    } while (n != 0);
    return out;
  }

  vector<uint8_t> encode_string(detail::string s) {
    vector<uint8_t> out;
    out.emplace_back(s.size());
    for (const auto &c : s.data) {
      out.emplace_back(c);
    }
    return out;
  }

  template <typename T> void concat(vector<uint8_t> &out, const T &inp) {
    for (const auto &i : inp) {
      out.emplace_back(i);
    }
    // out.insert(out.end(), inp.begin(), inp.end());
  };

  int declare_local(uint8_t type) {
    assert(cur_function_);
    int idx = cur_function_->locals.size() + cur_function_->input_types.size();
    cur_function_->locals.emplace_back(type);
    return idx;
  }

  const vector<uint8_t> &input_types() {
    assert(cur_function_);
    return cur_function_->input_types;
  }

  const vector<uint8_t> &locals() {
    assert(cur_function_);
    return cur_function_->locals;
  }

  void push(uint8_t type) { type_stack_.emplace_back(type); };

  uint8_t pop() {
    assert(type_stack_.size() && "popping empty stack");
    auto type = type_stack_.back();
    type_stack_.pop_back();
    return type;
  };

  void emit(uint8_t byte) { cur_bytes_.emplace_back(byte); }
  void emit(vector<uint8_t> bytes) { concat(cur_bytes_, bytes); }
  void emit(const memarg &m) {
    emit(encode_unsigned(m.first));
    emit(encode_unsigned(m.second));
  }
};

inline int Local::operator()(uint8_t type) { return cg.declare_local(type); };

inline void Local::set(int idx) {
  auto t = cg.pop();
  const auto &input_types = cg.input_types();
  auto expected_type = [&]() {
    if (idx < input_types.size()) {
      return input_types.at(idx);
    }
    return cg.locals().at(idx - input_types.size());
  }();
  assert(expected_type == t && "can't set local to this value (wrong type)");

  cg.emit(0x21);
  cg.emit(cg.encode_unsigned(idx));
}

inline void Local::get(int idx) {
  const auto &input_types = cg.input_types();
  if (idx < input_types.size()) {
    cg.push(input_types.at(idx));
  } else {
    cg.push(cg.locals().at(idx - input_types.size()));
  }

  cg.emit(0x20);
  cg.emit(cg.encode_unsigned(idx));
}

inline void Local::tee(int idx) {
  auto t = cg.pop();
  const auto &input_types = cg.input_types();
  auto expected_type = [&]() {
    if (idx < input_types.size()) {
      return input_types.at(idx);
    }
    return cg.locals().at(idx - input_types.size());
  }();
  assert(expected_type == t && "can't set local to this value (wrong type)");

  cg.emit(0x22);
  cg.emit(cg.encode_unsigned(idx));
  cg.push(expected_type);
}

inline I32::operator uint8_t() { return 0x7f; }

inline void I32::const_(int32_t i) {
  cg.emit(0x41);
  cg.emit(cg.encode_signed(i));
  cg.push(cg.i32);
}

inline F32::operator uint8_t() { return 0x7d; }

inline void F32::const_(float f) {
  cg.emit(0x43);
  uint8_t r[4];
  memcpy(&r, &f, sizeof(float));
  for (auto i = 0; i < 4; ++i) {
    cg.emit(r[i]);
  }
  cg.push(cg.f32);
}

inline V128::operator uint8_t() { return 0x7b; }

#define UNARY_OP(classname, op, opcode, in_type, out_type)                     \
  inline void classname::op() {                                                \
    bool valid = cg.pop() == cg.in_type;                                       \
    assert(valid && "invalid type for " #op);                                  \
    cg.emit(opcode);                                                           \
    cg.push(cg.out_type);                                                      \
  }

#define BINARY_OP(classname, op, opcode, type_a, type_b, out_type)             \
  inline void classname::op() {                                                \
    bool valid = cg.pop() == cg.type_a && cg.pop() == cg.type_b;               \
    assert(valid && "invalid type for " #op);                                  \
    cg.emit(opcode);                                                           \
    cg.push(cg.out_type);                                                      \
  }

#define LOAD_OP(classname, op, opcode, out_type)                               \
  inline void classname::op(uint32_t alignment, uint32_t offset) {             \
    auto idx_type = cg.pop();                                                  \
    assert(idx_type == cg.i32);                                                \
    cg.emit(opcode);                                                           \
    cg.emit(cg.encode_unsigned(alignment));                                    \
    cg.emit(cg.encode_unsigned(offset));                                       \
    cg.push(cg.out_type);                                                      \
  }

#define STORE_OP(classname, op, opcode)                                        \
  inline void classname::op(uint32_t alignment, uint32_t offset) {             \
    auto val_type = cg.pop();                                                  \
    auto idx_type = cg.pop();                                                  \
    assert(idx_type == cg.i32);                                                \
    cg.emit(opcode);                                                           \
    cg.emit(cg.encode_unsigned(alignment));                                    \
    cg.emit(cg.encode_unsigned(offset));                                       \
  }

UNARY_OP(I32, clz, 0x67, i32, i32);
UNARY_OP(I32, ctz, 0x68, i32, i32);
UNARY_OP(I32, popcnt, 0x69, i32, i32);
BINARY_OP(I32, lt_s, 0x48, i32, i32, i32);
BINARY_OP(I32, lt_u, 0x49, i32, i32, i32);
BINARY_OP(I32, gt_s, 0x4a, i32, i32, i32);
BINARY_OP(I32, gt_u, 0x4b, i32, i32, i32);
BINARY_OP(I32, le_s, 0x4c, i32, i32, i32);
BINARY_OP(I32, le_u, 0x4d, i32, i32, i32);
BINARY_OP(I32, ge_s, 0x4e, i32, i32, i32);
BINARY_OP(I32, ge_u, 0x4f, i32, i32, i32);
BINARY_OP(I32, add, 0x6a, i32, i32, i32);
BINARY_OP(I32, sub, 0x6b, i32, i32, i32);
BINARY_OP(I32, mul, 0x6c, i32, i32, i32);
BINARY_OP(I32, div_s, 0x6d, i32, i32, i32);
BINARY_OP(I32, div_u, 0x6e, i32, i32, i32);
BINARY_OP(I32, rem_s, 0x6f, i32, i32, i32);
BINARY_OP(I32, rem_u, 0x70, i32, i32, i32);
BINARY_OP(I32, and_, 0x71, i32, i32, i32);
BINARY_OP(I32, or_, 0x72, i32, i32, i32);
BINARY_OP(I32, xor_, 0x73, i32, i32, i32);
BINARY_OP(I32, shl, 0x74, i32, i32, i32);
BINARY_OP(I32, shr_s, 0x75, i32, i32, i32);
BINARY_OP(I32, shr_u, 0x76, i32, i32, i32);
BINARY_OP(I32, rotl, 0x77, i32, i32, i32);
BINARY_OP(I32, rotr, 0x78, i32, i32, i32);
BINARY_OP(I32, eqz, 0x45, i32, i32, i32);
BINARY_OP(I32, eq, 0x46, i32, i32, i32);
BINARY_OP(I32, ne, 0x47, i32, i32, i32);
LOAD_OP(I32, load, 0x28, i32);
LOAD_OP(I32, load8_s, 0x2c, i32);
LOAD_OP(I32, load8_u, 0x2d, i32);
LOAD_OP(I32, load16_s, 0x2e, i32);
LOAD_OP(I32, load16_u, 0x2f, i32);
STORE_OP(I32, store, 0x36);
STORE_OP(I32, store8, 0x3a);
STORE_OP(I32, store16, 0x3b);

BINARY_OP(F32, eq, 0x5b, f32, f32, i32);
BINARY_OP(F32, ne, 0x5c, f32, f32, i32);
BINARY_OP(F32, lt, 0x5d, f32, f32, i32);
BINARY_OP(F32, gt, 0x5e, f32, f32, i32);
BINARY_OP(F32, le, 0x5f, f32, f32, i32);
BINARY_OP(F32, ge, 0x60, f32, f32, i32);
UNARY_OP(F32, abs, 0x99, f32, f32);
UNARY_OP(F32, neg, 0x9a, f32, f32);
UNARY_OP(F32, ceil, 0x9b, f32, f32);
UNARY_OP(F32, floor, 0x9c, f32, f32);
UNARY_OP(F32, trunc, 0x9d, f32, f32);
UNARY_OP(F32, nearest, 0x9e, f32, f32);
UNARY_OP(F32, sqrt, 0x9f, f32, f32);
BINARY_OP(F32, add, 0x92, f32, f32, f32);
BINARY_OP(F32, sub, 0x93, f32, f32, f32);
BINARY_OP(F32, mul, 0x94, f32, f32, f32);
BINARY_OP(F32, div, 0x95, f32, f32, f32);
BINARY_OP(F32, min, 0x96, f32, f32, f32);
BINARY_OP(F32, max, 0x97, f32, f32, f32);
BINARY_OP(F32, copysign, 0x98, f32, f32, f32);
LOAD_OP(F32, load, 0x2a, f32);
STORE_OP(F32, store, 0x38);

#undef UNARY_OP
#undef BINARY_OP
#undef LOAD_OP
#undef STORE_OP

#define VECTOR_LOAD(op, vopcode)                                               \
  inline void V128::op(uint32_t alignment, uint32_t offset) {                  \
    auto idx_type = cg.pop();                                                  \
    assert(idx_type == cg.i32);                                                \
    cg.emit(0xfd);                                                             \
    cg.emit(cg.encode_unsigned(vopcode));                                      \
    cg.emit(cg.encode_unsigned(alignment));                                    \
    cg.emit(cg.encode_unsigned(offset));                                       \
    cg.push(cg.v128);                                                          \
  }

VECTOR_LOAD(load, 0);
VECTOR_LOAD(load32x2_s, 5);
VECTOR_LOAD(load32x2_u, 6);
VECTOR_LOAD(load32_splat, 9);
VECTOR_LOAD(load32_zero, 92);

inline void V128::store(uint32_t alignment, uint32_t offset) {
  auto val_type = cg.pop();
  assert(val_type == cg.v128);
  auto idx_type = cg.pop();
  assert(idx_type == cg.i32);
  cg.emit(0xfd);
  cg.emit(cg.encode_unsigned(11));
  cg.emit(cg.encode_unsigned(alignment));
  cg.emit(cg.encode_unsigned(offset));
}

inline void V128::i32x4_extract_lane(uint8_t lane) {
  auto val_type = cg.pop();
  assert(val_type == cg.v128);
  cg.emit(0xfd);
  cg.emit(cg.encode_unsigned(27));
  cg.push(cg.i32);
}

inline void V128::f32x4_extract_lane(uint8_t lane) {
  auto val_type = cg.pop();
  assert(val_type == cg.v128);
  cg.emit(0xfd);
  cg.emit(cg.encode_unsigned(31));
  cg.push(cg.f32);
}

inline void V128::i32x4_replace_lane(uint8_t lane) {
  auto vec_type = cg.pop();
  assert(vec_type == cg.v128);
  auto val_type = cg.pop();
  assert(val_type == cg.i32);
  cg.emit(0xfd);
  cg.emit(cg.encode_unsigned(28));
  cg.push(cg.v128);
}

inline void V128::f32x4_replace_lane(uint8_t lane) {
  auto vec_type = cg.pop();
  assert(vec_type == cg.v128);
  auto val_type = cg.pop();
  assert(val_type == cg.f32);
  cg.emit(0xfd);
  cg.emit(cg.encode_unsigned(32));
  cg.push(cg.v128);
}

inline void V128::i32x4_splat() {
  auto val_type = cg.pop();
  assert(val_type == cg.i32);
  cg.emit(0xfd);
  cg.emit(cg.encode_unsigned(17));
  cg.push(cg.v128);
}

inline void V128::f32x4_splat() {
  auto val_type = cg.pop();
  assert(val_type == cg.f32);
  cg.emit(0xfd);
  cg.emit(cg.encode_unsigned(19));
  cg.push(cg.v128);
}

#define VECTOR_BINARY_OP(op, vopcode, a_type, b_type, out_type)                \
  inline void V128::op() {                                                     \
    auto b = cg.pop();                                                         \
    assert(cg.b_type == b);                                                    \
    auto a = cg.pop();                                                         \
    assert(cg.a_type == a);                                                    \
    cg.emit(0xfd);                                                             \
    cg.emit(cg.encode_unsigned(vopcode));                                      \
    cg.push(cg.out_type);                                                      \
  }

#define VECTOR_UNARY_OP(op, vopcode, inp_type, out_type)                       \
  inline void V128::op() {                                                     \
    auto inp = cg.pop();                                                       \
    assert(cg.inp_type == inp);                                                \
    cg.emit(0xfd);                                                             \
    cg.emit(cg.encode_unsigned(vopcode));                                      \
    cg.push(cg.out_type);                                                      \
  }

VECTOR_BINARY_OP(i32x4_eq, 55, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_ne, 56, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_lt_s, 57, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_lt_u, 58, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_gt_s, 59, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_gt_u, 60, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_le_s, 61, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_le_u, 62, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_ge_s, 63, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_ge_u, 64, v128, v128, v128);

VECTOR_BINARY_OP(f32x4_eq, 65, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_ne, 66, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_lt, 67, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_gt, 68, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_le, 69, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_ge, 70, v128, v128, v128);

VECTOR_UNARY_OP(not_, 77, v128, v128);
VECTOR_UNARY_OP(any_true, 83, v128, i32);

// TODO
// VECTOR_TERNARY_OP(bitselect, 82, v128);

VECTOR_BINARY_OP(and_, 78, v128, v128, v128);
VECTOR_BINARY_OP(andnot, 79, v128, v128, v128);
VECTOR_BINARY_OP(or_, 80, v128, v128, v128);
VECTOR_BINARY_OP(xor_, 81, v128, v128, v128);

VECTOR_UNARY_OP(i32x4_abs, 160, v128, v128);
VECTOR_UNARY_OP(i32x4_neg, 161, v128, v128);
VECTOR_UNARY_OP(i32x4_all_true, 163, v128, i32);
VECTOR_UNARY_OP(i32x4_bitmask, 164, v128, i32);
VECTOR_BINARY_OP(i32x4_shl, 171, v128, i32, v128);
VECTOR_BINARY_OP(i32x4_shr_s, 172, v128, i32, v128);
VECTOR_BINARY_OP(i32x4_shr_u, 173, v128, i32, v128);
VECTOR_BINARY_OP(i32x4_add, 174, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_sub, 177, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_mul, 181, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_min_s, 182, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_min_u, 183, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_max_s, 184, v128, v128, v128);
VECTOR_BINARY_OP(i32x4_max_u, 185, v128, v128, v128);

VECTOR_UNARY_OP(f32x4_ceil, 103, v128, v128);
VECTOR_UNARY_OP(f32x4_floor, 104, v128, v128);
VECTOR_UNARY_OP(f32x4_trunc, 105, v128, v128);
VECTOR_UNARY_OP(f32x4_nearest, 106, v128, v128);
VECTOR_UNARY_OP(f32x4_abs, 224, v128, v128);
VECTOR_UNARY_OP(f32x4_neg, 225, v128, v128);
VECTOR_UNARY_OP(f32x4_sqrt, 227, v128, v128);
VECTOR_BINARY_OP(f32x4_add, 228, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_sub, 229, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_mul, 230, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_div, 231, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_min, 232, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_max, 233, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_pmin, 234, v128, v128, v128);
VECTOR_BINARY_OP(f32x4_pmax, 235, v128, v128, v128);

inline Memory &Memory::operator()(uint32_t min_) {
  assert(min == 0 && max == 0);
  min = min_;
  return *this;
}

inline Memory &Memory::operator()(uint32_t min_, uint32_t max_) {
  assert(min == 0 && max == 0);
  min = min_;
  max = max_;
  return *this;
}

inline Memory &Memory::export_(detail::string a) {
  assert(!(is_import() || is_export()) && "already set");
  a_string = a;
  return *this;
}

inline Memory &Memory::shared(bool make_shared) {
  is_shared = make_shared;
  return *this;
}

inline Memory &Memory::import_(detail::string a, detail::string b) {
  assert(!(is_import() || is_export()) && "already set");
  a_string = a;
  b_string = b;
  return *this;
}

inline void Memory::size() {
  cg.emit(0x3f);
  cg.emit(0x00);
}
inline void Memory::grow() {
  cg.emit(0x40);
  cg.emit(0x00);
}

inline void CodeGenerator::nop() { emit(0x01); }
inline void CodeGenerator::block(uint8_t type) {
  emit(0x02);
  emit(type);
}
inline void CodeGenerator::loop(uint8_t type) {
  emit(0x03);
  emit(type);
}

inline void CodeGenerator::if_(uint8_t type) {
  auto t = pop();
  assert(t == i32);
  emit(0x04);
  emit(type);
}
inline void CodeGenerator::else_() { emit(0x05); }
inline void CodeGenerator::br(uint32_t labelidx) {
  emit(0x0c);
  emit(encode_unsigned(labelidx));
}
inline void CodeGenerator::br_if(uint32_t labelidx) {
  auto t = pop();
  assert(t == i32);
  emit(0x0d);
  emit(encode_unsigned(labelidx));
}
inline void CodeGenerator::end() { emit(0x0b); }
inline void CodeGenerator::call(uint32_t fn_idx) {
  assert(fn_idx < functions_.size() && "function index does not exist");
  emit(0x10);
  emit(encode_unsigned(fn_idx));
}

inline void CodeGenerator::export_(uint32_t fn, detail::string name) {
  exported_functions_.emplace_back(
      detail::pair<uint32_t, detail::string>(fn, name));
}

inline uint32_t CodeGenerator::function(vector<uint8_t> input_types,
                                        vector<uint8_t> output_types,
                                        std::function<void()> body) {
  auto idx = functions_.size();
  functions_.emplace_back(Function(input_types, output_types, body));
  return idx;
}

inline vector<uint8_t> CodeGenerator::emit() {
  cur_bytes_.clear();
  vector<uint8_t> emitted_bytes = {// magic module header
                                   0x00, 0x61, 0x73, 0x6d,
                                   // module version
                                   0x01, 0x00, 0x00, 0x00};

  vector<uint8_t> type_section_bytes;
  concat(type_section_bytes, encode_unsigned(functions_.size()));
  for (const auto &f : functions_) {
    type_section_bytes.emplace_back(0x60);
    concat(type_section_bytes, encode_unsigned(f.input_types.size()));
    for (const auto &t : f.input_types) {
      type_section_bytes.emplace_back(t);
    }
    concat(type_section_bytes, encode_unsigned(f.output_types.size()));
    for (const auto &t : f.output_types) {
      type_section_bytes.emplace_back(t);
    }
  }

  emitted_bytes.emplace_back(0x1);
  concat(emitted_bytes, encode_unsigned(type_section_bytes.size()));
  concat(emitted_bytes, type_section_bytes);

  vector<uint8_t> import_section_bytes;
  if (memory.is_import()) {
    concat(import_section_bytes, encode_unsigned(1)); // 1 import
    concat(import_section_bytes, encode_string(memory.a_string));
    concat(import_section_bytes, encode_string(memory.b_string));
    import_section_bytes.emplace_back(0x2); // memory flag
    if (memory.min && memory.max) {
      if (memory.is_shared) {
        import_section_bytes.emplace_back(0x3);
      } else {
        import_section_bytes.emplace_back(0x01);
      }
      concat(import_section_bytes, encode_unsigned(memory.min));
      concat(import_section_bytes, encode_unsigned(memory.max));
    } else {
      assert(!memory.is_shared && "shared memory must have a max size");
      concat(import_section_bytes, encode_unsigned(memory.min));
    }
    emitted_bytes.emplace_back(0x2);
    concat(emitted_bytes, encode_unsigned(import_section_bytes.size()));
    concat(emitted_bytes, import_section_bytes);
  }

  vector<uint8_t> function_section_bytes;
  concat(function_section_bytes, encode_unsigned(functions_.size()));
  for (auto i = 0; i < functions_.size(); ++i) {
    concat(function_section_bytes, encode_unsigned(i));
  }
  emitted_bytes.emplace_back(0x3);
  concat(emitted_bytes, encode_unsigned(function_section_bytes.size()));
  concat(emitted_bytes, function_section_bytes);

  vector<uint8_t> memory_section_bytes;
  if (!memory.is_import() && (memory.min || memory.max)) {
    memory_section_bytes.emplace_back(0x01); // always 1 memory
    if (memory.min && memory.max) {
      if (memory.is_shared) {
        memory_section_bytes.emplace_back(0x03);
      } else {
        memory_section_bytes.emplace_back(0x01);
      }
      concat(memory_section_bytes, encode_unsigned(memory.min));
      concat(memory_section_bytes, encode_unsigned(memory.max));
    } else {
      assert(!memory.is_shared && "shared memory must have a max size");
      memory_section_bytes.emplace_back(0x00);
      concat(memory_section_bytes, encode_unsigned(memory.min));
    }
    emitted_bytes.emplace_back(0x05);
    concat(emitted_bytes, encode_unsigned(memory_section_bytes.size()));
    concat(emitted_bytes, memory_section_bytes);
  }

  vector<uint8_t> export_section_bytes;

  auto num_exports = exported_functions_.size() + memory.is_export();
  concat(export_section_bytes, encode_unsigned(num_exports));
  if (memory.is_export()) {
    concat(export_section_bytes, encode_string(memory.a_string));
    export_section_bytes.emplace_back(0x02);
    export_section_bytes.emplace_back(0x00); // always 1 memory at index 0
  }
  for (const auto &p : exported_functions_) {
    concat(export_section_bytes, encode_string(p.second));
    export_section_bytes.emplace_back(0x00);
    concat(export_section_bytes, encode_unsigned(p.first));
  }
  emitted_bytes.emplace_back(0x7);
  concat(emitted_bytes, encode_unsigned(export_section_bytes.size()));
  concat(emitted_bytes, export_section_bytes);

  vector<uint8_t> code_section_bytes;
  concat(code_section_bytes, encode_unsigned(functions_.size()));
  for (auto &f : functions_) {
    cur_function_ = &f;

    cur_bytes_.clear();
    f.emit();
    end();
    vector<uint8_t> body_bytes = cur_bytes_;

    cur_bytes_.clear();
    concat(cur_bytes_, encode_unsigned(f.locals.size()));
    for (const auto &l : f.locals) {
      emit(0x1);
      emit(l);
    }

    vector<uint8_t> header_bytes = cur_bytes_;
    auto fn_size = header_bytes.size() + body_bytes.size();

    concat(code_section_bytes, encode_unsigned(fn_size));
    concat(code_section_bytes, header_bytes);
    concat(code_section_bytes, body_bytes);
  }
  cur_function_ = nullptr;

  emitted_bytes.emplace_back(0xa);
  concat(emitted_bytes, encode_unsigned(code_section_bytes.size()));
  concat(emitted_bytes, code_section_bytes);

  return emitted_bytes;
}

} // namespace wasmblr
