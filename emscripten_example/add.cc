#include "wasmblr.h"

std::vector<uint8_t> gen_add_loop(int len) {
  assert(len % 4 == 0);
  wasmblr::CodeGenerator cg;
  cg.memory(1, 20).export_("mem");
  auto add_func = cg.function({cg.i32, cg.i32, cg.i32}, {}, [&]() {
    auto iter = cg.local(cg.i32);
    cg.i32.const_(0);
    cg.local.set(iter);

    cg.loop(cg.void_);
    {
      cg.local.get(2);
      cg.local.get(iter);
      cg.i32.add();

      cg.local.get(0);
      cg.local.get(iter);
      cg.i32.add();
      cg.v128.load();

      cg.local.get(1);
      cg.local.get(iter);
      cg.i32.add();
      cg.v128.load();

      cg.v128.f32x4_add();

      cg.v128.store();

      cg.i32.const_(4 * 4);  // vec of 4 floats
      cg.local.get(iter);
      cg.i32.add();
      cg.local.set(iter);

      cg.i32.const_(len * 4);  // bytes
      cg.local.get(iter);
      cg.i32.ge_u();
      cg.br_if(0);
    }
    cg.end();
  });
  cg.export_(add_func, "add");
  return cg.emit();
}

std::vector<uint8_t> gen_add_unroll(int len) {
  assert(len % 4 == 0);
  wasmblr::CodeGenerator cg;
  cg.memory(1, 20).export_("mem");
  auto add_func = cg.function({cg.i32, cg.i32, cg.i32}, {}, [&]() {
    // no loop at all
    for (auto i = 0; i < len / 4; ++i) {
      cg.local.get(2);

      cg.local.get(0);
      cg.v128.load(0, i * 16);

      cg.local.get(1);
      cg.v128.load(0, i * 16);

      cg.v128.f32x4_add();

      cg.v128.store(0, i * 16);
    }
  });
  cg.export_(add_func, "add");
  return cg.emit();
}

std::vector<uint8_t> gen_add_mix(int len, int unroll) {
  assert(len % (unroll * 4) == 0);
  wasmblr::CodeGenerator cg;
  cg.memory(1, 20).export_("mem");
  auto add_func = cg.function({cg.i32, cg.i32, cg.i32}, {}, [&]() {
    auto iter = cg.local(cg.i32);
    cg.i32.const_(0);
    cg.local.set(iter);

    cg.loop(cg.void_);

    for (auto i = 0; i < unroll; ++i) {
      cg.local.get(2);
      cg.local.get(iter);
      cg.i32.add();

      cg.local.get(0);
      cg.local.get(iter);
      cg.i32.add();
      cg.v128.load(0, i * 16);

      cg.local.get(1);
      cg.local.get(iter);
      cg.i32.add();
      cg.v128.load(0, i * 16);

      cg.v128.f32x4_add();

      cg.v128.store(0, i * 16);
    }

    cg.i32.const_(unroll * 16);
    cg.local.get(iter);
    cg.i32.add();
    cg.local.set(iter);

    cg.i32.const_(len * 4);  // bytes
    cg.local.get(iter);
    cg.i32.ge_s();
    cg.br_if(0);

    cg.end();
  });
  cg.export_(add_func, "add");
  return cg.emit();
}

std::vector<uint8_t> gen_add(int len, int unroll) {
  if (unroll == len) {
    return gen_add_unroll(len);
  } else if (unroll <= 1) {
    return gen_add_loop(len);
  }
  return gen_add(len, unroll);
}

extern "C" {

void add(const float* a, const float* b, float* c, int len) {
  for (auto i = 0; i < len; ++i) {
    c[i] = a[i] + b[i];
  }
}

uint8_t* jit_add(int len, int unroll) {
  auto bytes = gen_add(len, unroll);
  uint8_t* out = (uint8_t*)malloc(bytes.size());
  memcpy(out, bytes.data(), bytes.size());
  return out;
}

int jit_add_len(int len, int unroll) {
  auto bytes = gen_add(len, unroll);
  return bytes.size();
}
}
