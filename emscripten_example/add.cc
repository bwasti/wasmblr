#include "wasmblr.h"

std::vector<uint8_t> gen_add_hardcode(int len, int unroll) {
  assert(len % (unroll * 4) == 0);
  wasmblr::CodeGenerator cg;
  // we hardcode the inputs to be
  // 0 * N * sizeof(float), 1 * N * sizeof(float)
  // and the output to be
  // 2 * N * sizeof(float)
  auto pages = (len * 3 * 4) / (1 << 16) + 1;
  cg.memory(pages).export_("mem");
  auto add_func = cg.function({cg.i32, cg.i32, cg.i32}, {}, [&]() {
    auto iter = cg.local(cg.i32);
    cg.i32.const_(0);
    cg.local.set(iter);

    cg.loop(cg.void_);

    for (auto i = 0; i < unroll; ++i) {
      cg.local.get(iter);

      cg.local.get(iter);
      cg.v128.load(0, i * 16);

      cg.local.get(iter);
      cg.v128.load(0, (len * 4) + i * 16);

      cg.v128.f32x4_add();

      cg.v128.store(0, (len * 8) + i * 16);
    }

    cg.local.get(iter);
    cg.i32.const_(unroll * 16);
    cg.i32.add();
    cg.local.set(iter);

    cg.i32.const_(len * 4);  // bytes
    cg.local.get(iter);
    cg.i32.ge_u();
    cg.br_if(0);

    cg.end();
  });
  cg.export_(add_func, "add");
  return cg.emit();
}

std::vector<uint8_t> gen_add_loop(int len) {
  assert(len % 4 == 0);
  wasmblr::CodeGenerator cg;
  auto pages = (len * 3 * 4) / (1 << 16) + 1;
  cg.memory(pages).export_("mem");
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
  auto pages = (len * 3 * 4) / (1 << 16) + 1;
  cg.memory(pages).export_("mem");
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

std::vector<uint8_t> gen_add_mix_no_simd(int len, int unroll) {
  if (len < unroll) {
    unroll = len;
  }
  assert(len % (unroll) == 0);
  wasmblr::CodeGenerator cg;
  auto pages = (len * 3 * 4) / (1 << 16) + 1;
  cg.memory(pages).export_("mem");
  auto add_func = cg.function({cg.i32, cg.i32, cg.i32}, {}, [&]() {
    auto iter = cg.local(cg.i32);
    cg.i32.const_(0);
    cg.local.set(iter);

    cg.loop(cg.void_);

    for (auto i = 0; i < unroll; ++i) {
      cg.local.get(2);

      cg.local.get(0);
      cg.f32.load(0, i * 4);

      cg.local.get(1);
      cg.f32.load(0, i * 4);

      cg.f32.add();

      cg.f32.store(0, i * 4);
    }

    cg.local.get(0);
    cg.i32.const_(unroll * 4);
    cg.i32.add();
    cg.local.set(0);

    cg.local.get(1);
    cg.i32.const_(unroll * 4);
    cg.i32.add();
    cg.local.set(1);

    cg.local.get(2);
    cg.i32.const_(unroll * 4);
    cg.i32.add();
    cg.local.set(2);

    cg.local.get(iter);
    cg.i32.const_(unroll * 4);
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

std::vector<uint8_t> gen_add_mix(int len, int unroll) {
  assert(len % (unroll * 4) == 0);
  wasmblr::CodeGenerator cg;
  auto pages = (len * 3 * 4) / (1 << 16) + 1;
  cg.memory(pages).export_("mem");
  auto add_func = cg.function({cg.i32, cg.i32, cg.i32}, {}, [&]() {
    auto iter = cg.local(cg.i32);
    cg.i32.const_(0);
    cg.local.set(iter);

    cg.loop(cg.void_);

    for (auto i = 0; i < unroll; ++i) {
      cg.local.get(2);

      cg.local.get(0);
      cg.v128.load(0, i * 16);

      cg.local.get(1);
      cg.v128.load(0, i * 16);

      cg.v128.f32x4_add();

      cg.v128.store(0, i * 16);
    }

    cg.local.get(0);
    cg.i32.const_(unroll * 16);
    cg.i32.add();
    cg.local.set(0);

    cg.local.get(1);
    cg.i32.const_(unroll * 16);
    cg.i32.add();
    cg.local.set(1);

    cg.local.get(2);
    cg.i32.const_(unroll * 16);
    cg.i32.add();
    cg.local.set(2);

    cg.local.get(iter);
    cg.i32.const_(unroll * 16);
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

std::vector<uint8_t> gen_add(int len, int unroll, bool simd) {
  if (!simd) {
    return gen_add_mix_no_simd(len, unroll);
  }
  if (unroll * 4 >= len) {
    return gen_add_unroll(len);
  } else if (unroll <= 1) {
    return gen_add_loop(len);
  }
  return gen_add_mix(len, unroll);
}

extern "C" {

void add(const float* a, const float* b, float* c, int len) {
  for (auto i = 0; i < len; ++i) {
    c[i] = a[i] + b[i];
  }
}

uint8_t* jit_add(int len, int unroll, bool simd) {
  auto bytes = gen_add(len, unroll, simd);
  uint8_t* out = (uint8_t*)malloc(bytes.size());
  memcpy(out, bytes.data(), bytes.size());
  return out;
}

int jit_add_len(int len, int unroll, bool simd) {
  auto bytes = gen_add(len, unroll, simd);
  return bytes.size();
}
}
