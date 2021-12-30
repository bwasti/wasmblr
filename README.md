# wasmblr
A single header file Web Assembly assembler.

This library makes it easier to generate web assembly binaries directly from C++.
Perhaps useful for JIT compilation from within projects compiled with Emscripten.
For examples see below, or read the `test.cc` file.

Contributions welcome!

# Usage

`#include "wasmblr.h"` and compile with `-std=c++11` or higher.

In C++:

```cpp

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
auto bytes = c.emit();
std::ofstream wasm("factorial.wasm", std::ios::binary);
wasm.write((char*)bytes.data(), bytes.size());
```

If you'd prefer to avoid inheritance, you can use the code generator directly:

```cpp
wasmblr::CodeGenerator cg;
auto add_func = cg.function({cg.f32, cg.f32}, {cg.f32}, [&]() {
  cg.local.get(0);
  cg.local.get(1);
  cg.f32.add();
});
cg.export_(add_func, "add");

auto bytes = cg.emit();
std::ofstream wasm("factorial.wasm", std::ios::binary);
wasm.write((char*)bytes.data(), bytes.size());
```

And then, in JavaScript:

```javascript
const wasm = fs.readFileSync('factorial.wasm'); // or however you'd like to load it
const m = new WebAssembly.Module(wasm);
const instance = new WebAssembly.Instance(m, {});
// use the function
console.log(instance.exports.add(8, 9));
```

# Supported Features

### Recursion

```cpp
struct Code : wasmblr::CodeGenerator {
  // NB: Needs to be a class variable; the function body is evaluated later
  uint32_t factorial;
  Code() : wasmblr::CodeGenerator() {
    factorial = function({f32}, {f32}, [&]() {
      local.get(0);
      f32.const_(1.0f);
      f32.lt();
      // base case
      if_(f32);
      {
        f32.const_(1.0f);
      }
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
```

### Blocks

If-statements

```cpp
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
```

Loops

```cpp
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
```

### Memory

```cpp
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
```

### SIMD (32-bit lanes for now)

```cpp
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
```


# TODO

Many things. I would appreciate any help filing issues for missing things!
