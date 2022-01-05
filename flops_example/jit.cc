#include "wasmblr.h"

std::vector<uint8_t> gen(int32_t mac_per_load, int32_t load_per_loop, int32_t loops, bool simd) {
  wasmblr::CodeGenerator cg;
  // load per loop * 4 * (simd ? 4 : 1)
  int32_t mem_per_elem = load_per_loop * 4 * (simd ? 4 : 1);
  int32_t bytes = 3 * mem_per_elem;
  int32_t pages = bytes / (1 << 16) + 1;
  cg.memory(pages).export_("mem");

  int32_t a_offset = 0 * mem_per_elem;
  int32_t b_offset = 1 * mem_per_elem;
  int32_t c_offset = 2 * mem_per_elem;

  auto func = cg.function({}, {}, [&]() {
    auto gen_local = [&]() {
      if (simd) {
        return cg.local(cg.v128);
      }
      return cg.local(cg.f32);
    };

    std::vector<int32_t> a_locals;
    std::vector<int32_t> b_locals;
    std::vector<int32_t> c_locals;
    for (auto i = 0; i < load_per_loop; ++i) {
      a_locals.emplace_back(gen_local());
      b_locals.emplace_back(gen_local());
      c_locals.emplace_back(gen_local());
    }

    auto iter = -1;
    if (loops > 1) {
      iter = cg.local(cg.i32);
      cg.i32.const_(0);
      cg.local.set(iter);

      cg.loop(cg.void_);
    }

    auto load_local = [&](int local, int32_t off, int i) {
      cg.i32.const_(0);
      if (simd) {
        cg.v128.load(1, off + i * 16);
      } else {
        cg.f32.load(1, off + i * 4);
      }
      cg.local.set(local);
    };

    auto store_local = [&](int local, int32_t off, int i) {
      if (simd) {
        cg.v128.store(1, off + i * 16);
      } else {
        cg.f32.store(1, off + i * 4);
      }
    };

    for (auto i = 0; i < load_per_loop; ++i) {
      load_local(a_locals.at(i), a_offset, i);
      load_local(b_locals.at(i), b_offset, i);
      load_local(c_locals.at(i), c_offset, i);

      for (auto m = 0; m < mac_per_load; ++m) {
        cg.local.get(c_locals.at(i));
        cg.local.get(a_locals.at(i));
        cg.local.get(b_locals.at(i));
        if (simd) {
          cg.v128.f32x4_mul();
          cg.v128.f32x4_add();
        } else {
          cg.f32.mul();
          cg.f32.add();
        }
        cg.local.set(c_locals.at(i));
      }

      cg.i32.const_(0);
      cg.local.get(c_locals.at(i));
      store_local(c_locals.at(i), c_offset, i);
    }

    if (loops > 1) {
      cg.local.get(iter);
      cg.i32.const_(1);
      cg.i32.add();
      cg.local.set(iter);

      cg.i32.const_(loops);
      cg.local.get(iter);
      cg.i32.ge_s();
      cg.br_if(0);

      cg.end();
    }

  });
  cg.export_(func, "mac");
  return cg.emit();
}

extern "C" {

uint8_t* jit_mac(int32_t mac_per_load, int32_t load_per_loop, int32_t loops, bool simd) {
  auto bytes = gen(mac_per_load, load_per_loop, loops, simd);
  uint8_t* out = (uint8_t*)malloc(bytes.size());
  memcpy(out, bytes.data(), bytes.size());
  return out;
}

size_t jit_mac_len(int32_t mac_per_load, int32_t load_per_loop, int32_t loops, bool simd) {
  auto bytes = gen(mac_per_load, load_per_loop, loops, simd);
  return bytes.size();
}

}

