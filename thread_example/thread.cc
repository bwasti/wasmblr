#include "wasmblr.h"
#include <fstream>

int main() {

  int64_t len = 1024;

  wasmblr::CodeGenerator cg;

  // shared() makes threading possible
  auto pages = (len * 2 * 4) / (1 << 16) + 1;
  cg.memory(pages, pages + 1).import_("env", "memory").shared();

  auto square = cg.function({cg.i32, cg.i32}, {}, [&]() {
    auto iter = cg.local(cg.i32);
    //cg.i32.const_(0);
    cg.local.get(0);
    cg.local.set(iter);

    cg.loop(cg.void_);
    {
      cg.local.get(iter);

      cg.local.get(iter);
      cg.f32.load(0, 0);

      cg.local.get(iter);
      cg.f32.load(0, 0);

      cg.f32.mul();
      cg.f32.store(0, len * 4);

      //cg.i32.const_(4);
      cg.local.get(1);
      cg.local.get(iter);
      cg.i32.add();
      cg.local.set(iter);

      cg.i32.const_(len * 4);
      cg.local.get(iter);
      cg.i32.ge_u();
      cg.br_if(0);
    }
    cg.end();
    
  });

  cg.export_(square, "square");

  // write to a loadable binary
  std::ofstream wasm("square.wasm", std::ios::binary);
  auto bytes = cg.emit();
  wasm.write((char*)bytes.data(), bytes.size());

}
