#include <fstream>
#include <iostream>
#include <sstream>
#include "wasmblr.h"

void testJS(wasmblr::CodeGenerator& c,
            std::string invoke,
            std::string expected) {
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
  std::ofstream nf(node_file);
  nf << ss.str();
  nf << std::flush;
  std::system(("node " + node_file + " > " + out_file).c_str());
  std::stringstream ss_out;
  ss_out << std::ifstream(out_file).rdbuf();
  if (ss_out.str() != expected) {
    std::cerr << "got: " << ss_out.str();
    std::cerr << "expected: " << expected;
    std::ofstream wasm("error.wasm", std::ios::binary);
    std::cerr << "generated wasm saved to error.wasm\n";
    auto bytes = c.emit();
    wasm.write((char*)bytes.data(), bytes.size());
  }
  assert(ss_out.str() == expected && "failed");
}

void testBasic() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      auto add_func = function({f32, f32}, {f32}, [&]() {
        local.get(0);
        local.get(1);
        f32.add();
      });
      export_(add_func, "add");
    }
  };
  Code c;
  testJS(c, "console.log(instance.exports.add(8, 4));", "12\n");
}

void testConstant() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      auto constant_func =
          function({}, {i32}, [&]() { i32.const_(1024 * 1024 * 4); });
      export_(constant_func, "constant");
    }
  };
  Code c;
  testJS(c, "console.log(instance.exports.constant());", "4194304\n");
}

void testRecursive() {
  struct Code : wasmblr::CodeGenerator {
    // NB: Needs to be a class variable,
    // the function body is evaluated later
    uint32_t factorial;
    Code() : wasmblr::CodeGenerator() {
      factorial = function({f32}, {f32}, [&]() {
        local.get(0);
        f32.const_(1.0f);
        f32.lt();
        if_(f32);
        { f32.const_(1.0f); }
        else_();
        {
          local.get(0);
          local.get(0);
          f32.const_(1.0f);
          f32.sub();
          call(factorial);
          f32.mul();
        }
        end();
      });
      export_(factorial, "factorial");
    }
  };
  Code c;
  testJS(c, "console.log(instance.exports.factorial(4));", "24\n");
  testJS(c, "console.log(instance.exports.factorial(7));", "5040\n");
}

void testIfStatement() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      auto if_func = function({f32}, {f32}, [&]() {
        f32.const_(0.0f);
        local.get(0);
        f32.gt();
        if_(f32);
        f32.const_(0.0f);
        else_();
        local.get(0);
        end();
      });
      export_(if_func, "relu");
    }
  };
  Code c;
  testJS(c, "console.log(instance.exports.relu(-2));", "0\n");
  testJS(c, "console.log(instance.exports.relu(2));", "2\n");
}

void testLoop() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      auto loop_fn = function({}, {i32}, [&]() {
        auto i = local(i32);

        loop(void_);
        {
          local.get(i);
          i32.const_(1);
          i32.add();
          local.set(i);

          local.get(i);
          i32.const_(10);
          i32.lt_s();
          br_if(0);
        }
        end();
        local.get(i);
      });
      export_(loop_fn, "loop");
    }
  };
  Code c;
  testJS(c, "console.log(instance.exports.loop());", "10\n");
}

void testMemory() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() { memory(1, 10).export_("mem"); }
  };
  Code c;
  testJS(c, R"(
console.log(instance.exports.mem.buffer.byteLength);
instance.exports.mem.grow(1);
console.log(instance.exports.mem.buffer.byteLength);
  )",
         "65536\n131072\n");
}

void testStore() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      memory(1, 10).export_("mem");
      auto store = function({}, {}, [&]() {
        i32.const_(0);     // index 0
        i32.const_(1337);  // value 1337
        i32.store(0, 0);   // align 0, offset 0
      });
      export_(store, "store");
    }
  };
  Code c;
  testJS(c, R"(
instance.exports.store();
console.log(new Uint32Array(instance.exports.mem.buffer)[0])
  )",
         "1337\n");
}

void testSIMD() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      memory(1, 10).export_("mem");
      auto square = function({}, {}, [&]() {
        auto vec = local(v128);
        i32.const_(0);
        v128.load();
        local.set(vec);

        local.get(vec);
        local.get(vec);
        v128.f32x4_mul();
        local.set(vec);

        i32.const_(0);
        local.get(vec);
        v128.store();
      });
      export_(square, "simd_square");
    }
  };
  Code c;
  testJS(c, R"(
let inp = new Float32Array(instance.exports.mem.buffer);
inp[0] = 1;
inp[1] = 2;
inp[2] = 3;
inp[3] = 4;
instance.exports.simd_square();
console.log(inp[0], inp[1], inp[2], inp[3]);
  )",
         "1 4 9 16\n");
}

void testSIMDShift() {
  struct Code : wasmblr::CodeGenerator {
    Code() : wasmblr::CodeGenerator() {
      memory(1, 10).export_("mem");
      auto square = function({}, {}, [&]() {
        auto vec = local(v128);
        i32.const_(0);
        v128.load();
        local.set(vec);

        local.get(vec);
        i32.const_(2);
        v128.i32x4_shl();
        local.set(vec);

        i32.const_(0);
        local.get(vec);
        v128.store();
      });
      export_(square, "simd_square");
    }
  };
  Code c;
  testJS(c, R"(
let inp = new Int32Array(instance.exports.mem.buffer);
inp[0] = 1;
inp[1] = 2;
inp[2] = 3;
inp[3] = 4;
instance.exports.simd_square();
console.log(inp[0], inp[1], inp[2], inp[3]);
  )",
         "4 8 12 16\n");
}

int main() {
  testBasic();
  testConstant();
  testRecursive();
  testIfStatement();
  testLoop();
  testMemory();
  testStore();
  testSIMD();
  testSIMDShift();
  std::cout << "pass.\n";
}
