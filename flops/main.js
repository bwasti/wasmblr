async function jit(Module, mac_per_load, loads_per_loop, loops, simd) {
  const wasm = Module._jit_mac(mac_per_load, loads_per_loop, loops, simd);
  const wasm_len = Module._jit_mac_len(mac_per_load, loads_per_loop, loops, simd);
  const wasm_data = new Uint8Array(Module.HEAP8.buffer, wasm, wasm_len);
  const m = await WebAssembly.compile(wasm_data);
  const instance = await WebAssembly.instantiate(m, {});
  Module._free(wasm);
  const mem = instance.exports.mem;
  const elems = loads_per_loop * (simd ? 4 : 1);
  let a = new Float32Array(mem.buffer, 0, elems);
  let b = new Float32Array(mem.buffer, elems * 4, elems);
  let c = new Float32Array(mem.buffer, elems * 8, elems);
  return [instance.exports.mac, a, b, c];
}

function gen_ref(mac_per_load, loads_per_loop, loops, simd) {
  return function(A, B, C) {
    const elems = loads_per_loop * (simd ? 4 : 1);
    for (let l = 0; l < loops; ++l) {
      for (let ll = 0; ll < elems; ++ll) {
        let a = A[ll];
        let b = B[ll];
        let c = C[ll];
        for (let m = 0; m < mac_per_load; ++m) {
          c = a * b + c
        }
        C[ll] = c;
      }
    }
  }
}

function log(...args) {
  const str = args.reduce((a, b) => {
    return a + " " + b;
  }, "");
  document.querySelector('#output').appendChild(document.createTextNode(str));
  document.querySelector('#output').appendChild(document.createElement('br'));
}

function log_best(...args) {
  document.querySelector('#best').innerHTML = '';
  const str = args.reduce((a, b) => {
    return a + " " + b;
  }, "");
  document.querySelector('#best').appendChild(document.createTextNode(str));
  document.querySelector('#best').appendChild(document.createElement('br'));
  document.querySelector('#best').appendChild(document.createElement('br'));
}

function rand(a) {
  for (let i = 0; i < a.length; ++i) {
    a[i] = Math.random() / 100;
  }
}

function diff(a, b) {
  let max_diff = 0;
  for (let i = 0; i < a.length; ++i) {
    Math.max(Math.abs(a[i] - b[i]), max_diff);
  }
  return max_diff;
}

async function launch_mac_benchmark() {
  const Module = await createMyModule();
  let simd_support = [0];

  jit(Module, 1, 1, 1, true).then(() => {
    simd_support.push(1);
  }).catch(() => {
    log("no simd support");
  });

  let best_gflops = 0;
  let best_str = '';
  for (let mac_per_load of [1, 2, 4, 8, 16, 32]) {
    for (let loads_per_loop of [1, 2, 4, 8, 16, 32]) {
      for (let loops of [1, 16, 64, 128]) {
        for (let simd of simd_support) {
          const [fn, a, b, c] = await jit(Module, mac_per_load, loads_per_loop, loops, simd);
          const ops = loops * loads_per_loop * mac_per_load * (simd ? 4 : 1);
          rand(a);
          rand(b);
          rand(c);
          const ref_c = new Float32Array(c.length);
          ref_c.set(c);
          fn();
          const str = `(MACs per load: ${mac_per_load}, Loads per loop ${loads_per_loop}, Loops: ${loops}, SIMD: ${simd})`;
          const err = diff(c, ref_c);
          if (err > 0.1) {
            log("error!", str, 'example elem:', c[0]);
            continue;
          }

          const iters_sec = bench(100, fn);
          const gflops = ops * 2 * iters_sec / 1e9;
          log(`${gflops} GFlops`, str);
          if (gflops > best_gflops) {
            best_gflops = gflops;
            best_str = str;
            log_best(`Best: ${gflops} GFlops`, str);
          }
        }
      }
    }
  }
  log_best(`done. Best: ${best_gflops} GFlops`, best_str);
}

function run_bench(target, fn) {
  let diff = 0;
  let num_iters = 1;
  while (diff < (target / 2)) {
    num_iters *= 2;
    const t0 = performance.now();
    for (let i = 0; i < num_iters; ++i) {
      fn();
    }
    const t1 = performance.now();
    diff = t1 - t0;
  }
  const iters_sec = 1e3 * num_iters / diff;
  return iters_sec;
}

function bench(target_ms, fn) {
  const warmup_ms = target_ms / 10;
  run_bench(warmup_ms, fn);
  return run_bench(target_ms, fn);
}