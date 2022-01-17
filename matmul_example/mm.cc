#include "wasmblr.h"

struct MMGen : public wasmblr::CodeGenerator {
  MMGen(int M, int N, int K, int M_unroll, int N_unroll, int K_unroll) {
    auto pages = (M * N + K * N + M * K) * 4 / (1 << 16) + 1;
    auto A_off = 0;
    auto B_off = M * K * 4;
    auto C_off = (M * K + K * N) * 4;
    memory(pages).export_("mem");
    auto fn = function({}, {}, [=]() {
      auto m = local(i32);
      auto n = local(i32);
      auto k = local(i32);
      std::vector<int> load_a;
      std::vector<int> load_b;
      for (auto i = 0; i < M_unroll; ++i) {
        load_a.emplace_back(local(v128));
      }
      for (auto i = 0; i < N_unroll; ++i) {
        load_b.emplace_back(local(v128));
      }
      auto a_off = local(i32);
      auto b_off = local(i32);
      auto c_off = local(i32);
      std::vector<int> accs;
      for (auto i = 0; i < M_unroll * N_unroll; ++i) {
        accs.emplace_back(local(v128));
      }

      i32.const_(0);
      local.set(m);
      loop(void_); // M

      i32.const_(0);
      local.set(n);
      loop(void_); // N

      local.get(m);
      i32.const_(N / 4 / N_unroll);
      i32.mul();
      local.get(n);
      i32.add();
      i32.const_(4 * 4 * N_unroll);
      i32.mul();
      local.set(c_off);

      for (auto m_unroll = 0; m_unroll < M_unroll; ++m_unroll) {
      for (auto n_unroll = 0; n_unroll < N_unroll; ++n_unroll) {

        // load prev value
        local.get(c_off);
        v128.load(0, C_off + n_unroll * 4 * 4 + m_unroll * N * 4);
        local.set(accs.at(m_unroll * N_unroll + n_unroll));

      }
      }

      i32.const_(0);
      local.set(k);
      loop(void_); // K

      local.get(m);
      i32.const_(K);
      i32.mul();
      local.get(k);
      i32.add();
      i32.const_(2);
      i32.shl();
      local.set(a_off);

      local.get(k);
      i32.const_(N / 4 / N_unroll);
      i32.mul();
      local.get(n);
      i32.add();
      i32.const_(4 * 4 * N_unroll);
      i32.mul();
      local.set(b_off);

      for (auto k_unroll = 0; k_unroll < K_unroll; ++k_unroll) {

      for (auto m_unroll = 0; m_unroll < M_unroll; ++m_unroll) {
        local.get(a_off);
        v128.load32_splat(0, A_off + (m_unroll * K + k_unroll) * 4);
        local.set(load_a.at(m_unroll));
      }

      for (auto n_unroll = 0; n_unroll < N_unroll; ++n_unroll) {
        local.get(b_off);
        v128.load(0, B_off + (k_unroll * N + n_unroll * 4) * 4);
        local.set(load_b.at(n_unroll));
      }

      for (auto m_unroll = 0; m_unroll < M_unroll; ++m_unroll) {
      for (auto n_unroll = 0; n_unroll < N_unroll; ++n_unroll) {

        local.get(accs.at(m_unroll * N_unroll + n_unroll));
        local.get(load_a.at(m_unroll));
        local.get(load_b.at(n_unroll));
        v128.f32x4_mul();
        v128.f32x4_add();
        local.set(accs.at(m_unroll * N_unroll + n_unroll));

      }
      }

      //for (auto m_unroll = M_unroll - 1; m_unroll >= 0; --m_unroll) {
      //for (auto n_unroll = N_unroll - 1; n_unroll >= 0; --n_unroll) {
      //  local.set(accs.at(m_unroll * N_unroll + n_unroll));
      //}
      //}

      }
      
      local.get(k);
      i32.const_(K_unroll);
      i32.add();
      local.tee(k);
      i32.const_(K);
      i32.lt_u();
      br_if(0);

      end();

      // store output of C
      for (auto m_unroll = 0; m_unroll < M_unroll; ++m_unroll) {
      for (auto n_unroll = 0; n_unroll < N_unroll; ++n_unroll) {
        //local.get(m);
        //i32.const_(N / 4 / N_unroll);
        //i32.mul();
        //local.get(n);
        //i32.add();
        //i32.const_(4 * 4 * N_unroll);
        //i32.mul();

        local.get(c_off);
        local.get(accs.at(m_unroll * N_unroll + n_unroll));
        v128.store(0, C_off + n_unroll * 4 * 4 + m_unroll * N * 4);
      }
      }

      local.get(n);
      i32.const_(1);
      i32.add();
      local.tee(n);
      i32.const_(N / 4 / N_unroll);
      i32.lt_u();
      br_if(0);

      end();

      local.get(m);
      i32.const_(M_unroll);
      i32.add();
      local.tee(m);
      i32.const_(M);
      i32.lt_u();
      br_if(0);

      end();

    });
    export_(fn, "mm");
  }
};

extern "C" {

uint8_t* jit_mm(int M, int N, int K, int Mu, int Nu, int Ku) {
  MMGen mm(M, N, K, Mu, Nu, Ku);
  auto bytes = mm.emit();
  uint8_t* out = (uint8_t*)malloc(bytes.size());
  memcpy(out, bytes.data(), bytes.size());
  return out;
}

int jit_mm_len(int M, int N, int K, int Mu, int Nu, int Ku) {
  MMGen mm(M, N, K, Mu, Nu, Ku);
  auto bytes = mm.emit();
  return bytes.size();
}

}
