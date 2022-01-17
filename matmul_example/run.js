emsc = require('./mm.js');
const { performance } = require('perf_hooks');


function log(...args) {
  const str = args.reduce((a, b) => {
    return a + " " + b;
  }, "");
  console.log(str);
}

async function jit(Module, M, N, K, Mu, Nu, Ku) {
  const wasm = Module._jit_mm(M, N, K, Mu, Nu, Ku);
  const wasm_len = Module._jit_mm_len(M, N, K, Mu, Nu, Ku);
  const wasm_data = new Uint8Array(Module.HEAP8.buffer, wasm, wasm_len);
  const m = await WebAssembly.compile(wasm_data);
  const instance = await WebAssembly.instantiate(m, {});
  Module._free(wasm);
  const mem = instance.exports.mem;
  let a = new Float32Array(mem.buffer, 0, M * K);
  let b = new Float32Array(mem.buffer, M * K * 4, K * N);
  let c = new Float32Array(mem.buffer, (M * K + K * N) * 4, M * N);
  return [instance.exports.mm, a, b, c];
}

function ref_mm(a, b, M, N, K) {
  c = new Float32Array(M * N);
  for (let m = 0; m < M; ++m) {
    for (let n = 0; n < N; ++n) {
      for (let k = 0; k < K; ++k) {
        c[m * N + n] += a[m * K + k] * b[k * N + n];
      }
    }
  }
  return c;
}

async function bench(m, M, N, K, Mu, Nu, Ku) {
  const [fn, a, b, c] = await jit(m, N, N, N, Mu, Nu, Ku);
  //log("jit done");
  for (let i = 0; i < N*N; ++i) {
    a[i] = Math.random();
    b[i] = Math.random();
    c[i] = 0;
  }
  fn();
  //console.log(c);
  const ref_c = ref_mm(a, b, M, N, K);
  let max_diff = 0;
  for (let i = 0; i <  M * N; ++i) {
    max_diff = Math.max(max_diff, Math.abs(ref_c[i] - c[i]));
  }
  //console.log("max diff", max_diff);
  //console.log(a, b, c);
  for (let i = 0; i < 100; ++i) {
    fn();
  }
  const t = performance.now();
  for (let _ = 0; _ < 1000; ++_) {
    fn();
  }
  const diff = performance.now() - t;
  return 1e3 * N * N * N * 2 * 1000 / diff / 1e9;
}

async function init() {
  let mod = await emsc();
  //log("loaded");
  const N = 512;
  const M = N;
  const K = N;
  let gflops = await bench(mod, M, N, K, 4, 2, 1);
  log("gflops", gflops);
  //for (let m of [1, 2, 4, 8]) {
  //  for (let n of [1, 2, 4, 8, 16]) {
  //    for (let k of [1, 2, 4]) {
  //      let gflops = await bench(mod, M, N, K, m, n, k);
  //      log(m, n, k, "gflops", gflops);
  //    }
  //  }
  //}
}

init();

