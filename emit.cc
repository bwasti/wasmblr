#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "wasmblr.h"

//// If you're using C++11, you'll need this:
// #define X(symbol, value) constexpr uint8_t wasmblr::CodeGenerator::_##symbol;
//   SECTIONS(X);
//   TYPES(X);
//   VALTYPES(X);
//   OPCODES(X);
// #undef X

void test(wasmblr::CodeGenerator& c, std::string invoke, std::string expected) {
  std::stringstream ss;
  ss << "const wasm = new Uint8Array([";
  for (const auto& b : c.emit()) {
    ss << "0x" << std::hex << static_cast<int>(b) << ", ";
  }
  ss << "]);\n";
  ss << "const m = new WebAssembly.Module(wasm);\n";
  ss << "const instance = new WebAssembly.Instance(m, {});\n";
  ss << invoke;

  std::string node_file = "/tmp/test.js";
  std::string out_file = "/tmp/test.out";
  std::ofstream nf(node_file);  //, std::ios::binary);
  nf << ss.str();
  nf << std::flush;
  std::system(("node " + node_file + " > " + out_file).c_str());
  std::stringstream ss_out;
  ss_out << std::ifstream(out_file).rdbuf();
  if (ss_out.str() != expected) {
    std::cout << "got: " << ss_out.str() << "\n";
    std::cout << "expected: " << expected << "\n";
  }
  assert(ss_out.str() == expected && "failed");
}

void testInherit() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      auto add_func = function({}, {f32}, [&]() {
        // declarations are handled by the assembler
        auto tmp = local(f32);
        {
          constant(4.0f);
          local.set(tmp);
        }
        {  // add takes 2 args, push them to the stack
          local.get(tmp);
          constant(8.0f);
          add();
        }
      });
      export_(add_func, "add");
    }
  };
  Code c;
  test(c, "console.log(instance.exports.add());", "12\n");
}

void testNoInherit() {
  wasmblr::CodeGenerator cg;
  auto add_func = cg.function({cg.f32}, {cg.f32}, [&]() {
    // declarations are handled by the assembler
    auto tmp = cg.local(cg.f32);
    {
      cg.constant(4.0f);
      cg.local.set(tmp);
    }
    {  // add takes 2 args, push them to the stack
      cg.local.get(tmp);
      cg.local.get(0);
      cg.add();
    }
  });
  cg.export_(add_func, "add");

  test(cg, "console.log(instance.exports.add(-2));", "2\n");
  test(cg, "console.log(instance.exports.add(2));", "6\n");
}

void testIfStatement() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      auto if_func = function({f32}, {f32}, [&]() {
        constant(0.0f);
        local.get(0);
        gt();
        if_(f32);
        { constant(0.0f); }
        else_();
        { local.get(0); }
        end();
      });
      export_(if_func, "relu");
    }
  };
  Code c;
  test(c, "console.log(instance.exports.relu(-2));", "0\n");
  test(c, "console.log(instance.exports.relu(2));", "2\n");
}

void testRecursion() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      int factorial;
      factorial = function({f32}, {f32}, [&]() {
        local.get(0);
        constant(1.0f);
        lt();
        if_(f32);
        { constant(1.0f); }
        else_();
        {
          local.get(0);
          local.get(0);
          constant(1.0f);
          sub();
          call(factorial);
          mul();
        }
        end();
      });
      export_(factorial, "factorial");
    }
  };
  Code c;
  test(c, "console.log(instance.exports.factorial(4));", "24\n");
  test(c, "console.log(instance.exports.factorial(7));", "5040\n");
}

int main() {
  testInherit();
  testNoInherit();
  testIfStatement();
  testRecursion();
  return 0;
}
