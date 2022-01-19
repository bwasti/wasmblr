function log(...args) {
  const str = args.reduce((a, b) => {
    return a + " " + b;
  }, "");
  document.querySelector('#output').appendChild(document.createTextNode(str));
  document.querySelector('#output').appendChild(document.createElement('br'));
}

async function has_simd(Module) {
  const wasm = Module._jit_mm(4,4,4,1,1,1);
  const wasm_len = Module._jit_mm_len(4,4,4,1,1,1);
  const wasm_data = new Uint8Array(Module.HEAP8.buffer, wasm, wasm_len);
  let has = true;
  const m = await WebAssembly.compile(wasm_data).catch(e => {
    has = false;
  });
  Module._free(wasm);
  return has;
}

async function jit(Module, M, N, K, Mu, Nu, Ku) {
  let [jit, len] = [Module._jit_mm, Module._jit_mm_len];
  const simd = await has_simd(Module);
  if (!simd) {
    [jit, len] = [Module._jit_mm_nosimd, Module._jit_mm_nosimd_len];
  }
  const wasm = jit(M, N, K, Mu, Nu, Ku);
  const wasm_len = len(M, N, K, Mu, Nu, Ku);
  const wasm_data = new Uint8Array(Module.HEAP8.buffer, wasm, wasm_len);
  const m = await WebAssembly.compile(wasm_data).catch(e => log('Error compiling ->', e));
  const instance = await WebAssembly.instantiate(m, {});
  Module._free(wasm);
  const mem = instance.exports.mem;
  let a = new Float32Array(mem.buffer, 0, M * K);
  let b = new Float32Array(mem.buffer, M * K * 4, K * N);
  let c = new Float32Array(mem.buffer, (M * K + K * N) * 4, M * N);
  return [instance.exports.mm, a, b, c, simd];
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
  const [fn, a, b, c, simd] = await jit(m, M, N, K, Mu, Nu, Ku);
  for (let i = 0; i < N * N; ++i) {
    a[i] = Math.random();
    b[i] = Math.random();
    c[i] = 0;
  }
  fn();
  const ref_c = ref_mm(a, b, M, N, K);
  let max_diff = 0;
  for (let i = 0; i < M * N; ++i) {
    max_diff = Math.max(max_diff, Math.abs(ref_c[i] - c[i]));
  }
  console.log("max diff", max_diff);
  if (max_diff > 0.1) {
    log("error! max diff", max_diff);
  }
  for (let i = 0; i < 10; ++i) {
    fn();
  }
  // ~0.1if we hit 40gflops
  const iters = 4e9 / (M * N * K * 2) / (simd ? 1 : 4);
  const t = performance.now();
  for (let _ = 0; _ < iters; ++_) {
    fn();
  }
  const diff = performance.now() - t;
  return 1e3 * N * N * N * 2 * iters / diff / 1e9;
}

async function init(N) {
  document.getElementById("output").textContent = '';
  document.getElementById("highlight").textContent = '';
  let mod = await createMyModule();
  const M = N;
  const K = N;
  let best_gflops = 0;
  let best_str = '';
  const simd = await has_simd(mod);
  if (!simd) {
    log('No simd found, falling back to scalar code.');
  }
  for (let m of [1, 2, 4, 8, 16, 32]) {
    for (let n of [1, 2, 4, 8, 16, 32]) {
      for (let k of [1, 2, 4, 8, 16, 32]) {
        if (k > K) {
          continue;
        }
        if (m > M) {
          continue;
        }
        if (n * 4 > N) {
          continue;
        }
        let gflops = await bench(mod, M, N, K, m, n, k);
        if (gflops > best_gflops) {
          best_gflops = gflops;
          let pre = document.getElementById("highlight");
          best_str = `best gflops: ${best_gflops} (unroll m: ${m}, n: ${n}, k: ${k})`;
          pre.textContent = best_str;
        }
        log(m, n, k, "gflops", gflops);
      }
    }
  }
  let pre = document.getElementById("highlight");
  let str = `(done) ${best_str}`;
  pre.textContent = str;
}

window.addEventListener('load', function() {
  document.getElementById('mm128').addEventListener('click', () => init(128));
  document.getElementById('mm256').addEventListener('click', () => init(256));
  document.getElementById('mm512').addEventListener('click', () => init(512));
});
