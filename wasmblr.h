#include <array>
#include <cassert>
#include <cstdint>
#include <list>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace wasmblr {

constexpr std::array<uint8_t, 4> magic_module_header = {0x00, 0x61, 0x73, 0x6d};
constexpr std::array<uint8_t, 4> module_version = {0x01, 0x00, 0x00, 0x00};

#define SECTIONS(_) \
  _(custom, 0)      \
  _(type, 1)        \
  _(import, 2)      \
  _(func, 3)        \
  _(table, 4)       \
  _(memory, 5)      \
  _(global, 6)      \
  _(export_, 7)     \
  _(start, 8)       \
  _(element, 9)     \
  _(code, 10)       \
  _(data, 11)

#define TYPES(_)    \
  _(functype, 0x60) \
  _(funcidx, 0x00)

#define VALTYPES(_) \
  _(i32, 0x7f)      \
  _(i64, 0x7e)      \
  _(f32, 0x7d)      \
  _(f64, 0x7c)      \
  _(v128, 0x7b)

#define CONTROL_INSTRUCTIONS(_) \
  _(unreachable, 0x00)          \
  _(nop, 0x01)                  \
  _(block, 0x02)                \
  _(loop, 0x03)                 \
  _(if_, 0x04)                  \
  _(else_, 0x05)                \
  _(br, 0x0c)                   \
  _(br_if, 0x0d)                \
  _(return_, 0x0f)              \
  _(call, 0x10)                 \
  _(call_indirect, 0x11)

#define REFERENCE_INSTRUCTIONS(_) \
  _(ref_null, 0xd0)               \
  _(ref_is_null, 0xd1)            \
  _(ref_func, 0xd2)

#define PARAMETRIC_INSTRUCTIONS(_) \
  _(drop, 0x1a)                    \
  _(select, 0x1b)                  \
  _(select_type, 0x1c)

#define VARIABLE_INSTRUCTIONS(_) \
  _(local_get, 0x20)             \
  _(local_set, 0x21)             \
  _(local_tee, 0x22)             \
  _(global_get, 0x23)            \
  _(global_get, 0x24)

#define TABLE_INSTRUCTIONS(_) \
  _(table_get, 0x25)          \
  _(table_set, 0x26)          \
  _(table_mutate, 0xFC)  // prefix for init, copy, grow, size, fill

#define MEMORY_INSTRUCTIONS(_) \
  _(i32_load, 0x28)            \
  _(i64_load, 0x29)            \
  _(f32_load, 0x2A)            \
  _(f64_load, 0x2B)            \
  _(i32_load8_s, 0x2C)         \
  _(i32_load8_u, 0x2D)         \
  _(i32_load16_s, 0x2E)        \
  _(i32_load16_u, 0x2F)        \
  _(i64_load8_s, 0x30)         \
  _(i64_load8_u, 0x31)         \
  _(i64_load16_s, 0x32)        \
  _(i64_load16_u, 0x33)        \
  _(i64_load32_s, 0x34)        \
  _(i64_load32_u, 0x35)        \
  _(i32_store, 0x36)           \
  _(i64_store, 0x37)           \
  _(f32_store, 0x38)           \
  _(f64_store, 0x39)           \
  _(i32_store8, 0x3a)          \
  _(i32_store16, 0x3b)         \
  _(i64_store8, 0x3c)          \
  _(i64_store16, 0x3d)         \
  _(i64_store32, 0x3e)         \
  _(memory_size, 0x3f)         \
  _(memory_grow, 0x40)         \
  _(memory_mutate, 0xFC)  // prefix for init, copy, fill, drop

#define OPCODES(_)     \
  _(block, 0x02)       \
  _(loop, 0x03)        \
  _(if_, 0x04)         \
  _(else_, 0x05)       \
  _(br, 0x0c)          \
  _(br_if, 0x0d)       \
  _(end, 0x0b)         \
  _(call, 0x10)        \
  _(get_local, 0x20)   \
  _(set_local, 0x21)   \
  _(i32_store_8, 0x3a) \
  _(i32_const, 0x41)   \
  _(f32_const, 0x43)   \
  _(i32_eqz, 0x45)     \
  _(i32_eq, 0x46)      \
  _(f32_eq, 0x5b)      \
  _(f32_lt, 0x5d)      \
  _(f32_gt, 0x5e)      \
  _(i32_and, 0x71)     \
  _(f32_add, 0x92)     \
  _(f32_sub, 0x93)     \
  _(f32_mul, 0x94)     \
  _(f32_div, 0x95)     \
  _(i32_trunc_f32_s, 0xa8)

struct CodeGenerator {
#define X(symbol, value) static constexpr uint8_t _##symbol = value;
  SECTIONS(X);
  TYPES(X);
  VALTYPES(X);
  OPCODES(X);
#undef X

#define X(symbol, value) static constexpr uint8_t symbol = _##symbol;
  VALTYPES(X);
#undef X

  struct Function {
    Function(std::vector<uint8_t> input_types_,
             std::vector<uint8_t> output_types_, std::function<void()> body_)
        : input_types(input_types_), output_types(output_types_), body(body_) {}
    std::vector<uint8_t> input_types;
    std::vector<uint8_t> output_types;
    std::function<void()> body;
    std::vector<uint8_t> locals;  // resolved later
  };

  using memarg = std::pair<uint32_t, uint32_t>;
  void emit(uint8_t byte) { cur_bytes_.emplace_back(byte); }
  void emit(std::vector<uint8_t> bytes) { concat(cur_bytes_, bytes); }
  void emit(const memarg& m) {
    emit(encode_unsigned(std::get<0>(m)));
    emit(encode_unsigned(std::get<1>(m)));
  }

  class V128 {
    V128(CodeGenerator& cg_) : cg(cg_) {}
    CodeGenerator& cg;

   public:
#define MEM(_)    \
  _(load, 0)      \
  _(load8x8_s, 1) \
  _(load8x8_u, 1)
#define X(name, op)                  \
  void name(const memarg& m) {       \
    cg.emit(0xfd);                   \
    cg.emit(cg.encode_unsigned(op)); \
    cg.emit(m);                      \
  }
    MEM(X);
#undef X
#undef MEM
    void load8_lane(const memarg& m, uint8_t lane) {
      cg.emit(0xfd);
      cg.emit(cg.encode_unsigned(84));
      cg.emit(m);
      cg.emit(lane);
    }

    friend CodeGenerator;
  };
  class Local {
    Local(CodeGenerator& cg) : cg_(cg) {}
    CodeGenerator& cg_;

   public:
    int operator()(uint8_t type) { return cg_.declare_local(type); };

    void set(int idx) {
      cg_.cur_bytes_.emplace_back(_set_local);
      cg_.cur_bytes_.emplace_back(idx);
      auto t = cg_.pop();

      const auto& input_types = cg_.cur_function_->input_types;
      auto expected_type = [&]() {
        if (idx < input_types.size()) {
          return input_types.at(idx);
        }
        return cg_.cur_function_->locals.at(idx - input_types.size());
      }();
      assert(expected_type == t &&
             "can't set local to this value (wrong type)");
    }

    void get(int idx) {
      cg_.cur_bytes_.emplace_back(_get_local);
      auto shifted_idx = +idx;
      cg_.cur_bytes_.emplace_back(shifted_idx);
      const auto& input_types = cg_.cur_function_->input_types;
      if (idx < input_types.size()) {
        cg_.push(input_types.at(idx));
      } else {
        cg_.push(cg_.cur_function_->locals.at(idx - input_types.size()));
      }
    }
    friend CodeGenerator;
  };

  std::vector<Function> functions_;
  // function index -> function name
  std::unordered_map<int, std::string> exported_functions_;
  Function* cur_function_ = nullptr;
  std::vector<uint8_t> cur_bytes_;
  std::vector<uint8_t> emitted_bytes_;
  std::stack<uint8_t> type_stack_;
  Local local;

  int declare_local(uint8_t type) {
    assert(cur_function_);
    int idx = cur_function_->locals.size() + cur_function_->input_types.size();
    cur_function_->locals.emplace_back(type);
    return idx;
  }

  CodeGenerator() : local(*this) {}
  CodeGenerator(const CodeGenerator&) = delete;
  CodeGenerator(CodeGenerator&&) = delete;

  void export_(int fn, std::string name) { exported_functions_[fn] = name; }
  int function(std::vector<uint8_t> input_types,
               std::vector<uint8_t> output_types, std::function<void()> body) {
    functions_.emplace_back(input_types, output_types, body);
    return functions_.size() - 1;
  }

  void constant(float f) {
    cur_bytes_.emplace_back(_f32_const);
    uint8_t r[4];
    memcpy(&r, &f, sizeof(float));
    for (auto i = 0; i < 4; ++i) {
      cur_bytes_.emplace_back(r[i]);
    }
    push(_f32);
  }

#define BIN_OP(op)                                     \
  void op() {                                          \
    auto t = pop();                                    \
    assert(pop() == t && "can't add different types"); \
    switch (t) {                                       \
      case _f32:                                       \
        cur_bytes_.emplace_back(_f32_##op);            \
        break;                                         \
      default:                                         \
        assert(0 && "unhandled type for " #op);        \
    };                                                 \
    push(t);                                           \
  }

  BIN_OP(add);
  BIN_OP(sub);
  BIN_OP(mul);
  BIN_OP(div);
  BIN_OP(lt);
  BIN_OP(gt);
  BIN_OP(eq);
#undef BIN_OP

  void if_(uint8_t type) {
    auto t = pop();
    cur_bytes_.emplace_back(_if_);
    cur_bytes_.emplace_back(type);
  };

  void else_() { cur_bytes_.emplace_back(_else_); }

  void end() { cur_bytes_.emplace_back(_end); }

  void call(int fn) {
    cur_bytes_.emplace_back(_call);
    concat(cur_bytes_, encode_unsigned(fn));
  }

  void push(uint8_t type) { type_stack_.push(type); };

  uint8_t pop() {
    assert(type_stack_.size() && "popping empty stack");
    auto type = type_stack_.top();
    type_stack_.pop();
    return type;
  };

  std::vector<uint8_t> encode_unsigned(uint32_t n) {
    std::vector<uint8_t> out;
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

  std::vector<uint8_t> encode_string(std::string s) {
    std::vector<uint8_t> out;
    out.emplace_back(s.size());
    for (const auto& c : s) {
      out.emplace_back(c);
    }
    return out;
  }

  template <typename T>
  void concat(std::vector<uint8_t>& out, const T& inp) {
    out.insert(out.end(), inp.begin(), inp.end());
  };

  std::vector<uint8_t> emit() {
    emitted_bytes_.clear();
    cur_bytes_.clear();

    concat(emitted_bytes_, magic_module_header);
    concat(emitted_bytes_, module_version);

    std::vector<uint8_t> type_section_bytes;
    concat(type_section_bytes, encode_unsigned(functions_.size()));
    for (const auto& f : functions_) {
      type_section_bytes.emplace_back(_functype);
      concat(type_section_bytes, encode_unsigned(f.input_types.size()));
      for (const auto& t : f.input_types) {
        type_section_bytes.emplace_back(t);
      }
      concat(type_section_bytes, encode_unsigned(f.output_types.size()));
      for (const auto& t : f.output_types) {
        type_section_bytes.emplace_back(t);
      }
    }

    emitted_bytes_.emplace_back(_type);
    concat(emitted_bytes_, encode_unsigned(type_section_bytes.size()));
    concat(emitted_bytes_, type_section_bytes);

    std::vector<uint8_t> function_section_bytes;
    concat(function_section_bytes, encode_unsigned(functions_.size()));
    for (auto i = 0; i < functions_.size(); ++i) {
      concat(function_section_bytes, encode_unsigned(i));
    }
    emitted_bytes_.emplace_back(_func);
    concat(emitted_bytes_, encode_unsigned(function_section_bytes.size()));
    concat(emitted_bytes_, function_section_bytes);

    std::vector<uint8_t> export_section_bytes;
    concat(export_section_bytes, encode_unsigned(exported_functions_.size()));
    for (const auto& p : exported_functions_) {
      concat(export_section_bytes, encode_string(p.second));
      export_section_bytes.emplace_back(_funcidx);
      concat(export_section_bytes, encode_unsigned(p.first));
    }
    emitted_bytes_.emplace_back(_export_);
    concat(emitted_bytes_, encode_unsigned(export_section_bytes.size()));
    concat(emitted_bytes_, export_section_bytes);

    std::vector<uint8_t> code_section_bytes;
    concat(code_section_bytes, encode_unsigned(functions_.size()));
    for (auto& f : functions_) {
      cur_function_ = &f;

      cur_bytes_.clear();
      f.body();
      cur_bytes_.emplace_back(_end);
      std::vector<uint8_t> body_bytes = cur_bytes_;

      cur_bytes_.clear();
      concat(cur_bytes_, encode_unsigned(f.locals.size()));
      for (const auto& l : f.locals) {
        cur_bytes_.emplace_back(1);
        cur_bytes_.emplace_back(l);
      }

      std::vector<uint8_t> header_bytes = cur_bytes_;
      auto fn_size = header_bytes.size() + body_bytes.size();

      concat(code_section_bytes, encode_unsigned(fn_size));
      concat(code_section_bytes, header_bytes);
      concat(code_section_bytes, body_bytes);
    }

    emitted_bytes_.emplace_back(_code);
    concat(emitted_bytes_, encode_unsigned(code_section_bytes.size()));
    concat(emitted_bytes_, code_section_bytes);

    return emitted_bytes_;
  }
};

}  // namespace wasmblr
