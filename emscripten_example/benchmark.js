const em = require('./add.js');
var Module;
const wasmblr_unroll = 16;
const warmup = 100;
const target_ms = 1000;

async function gen_pure(N) {
  let a = new Array(N).fill(0);
  let b = new Array(N).fill(0);
  let c = new Array(N).fill(0);

  function add() {
    for (let i = 0; i < N; ++i) {
      c[i] = a[i] + b[i];
    }
  }

  return [add, a, b, c];
}

async function gen_typed(N) {
  let a = new Float32Array(N);
  let b = new Float32Array(N);
  let c = new Float32Array(N);

  function add() {
    for (let i = 0; i < N; ++i) {
      c[i] = a[i] + b[i];
    }
  }

  return [add, a, b, c];
}

async function gen_emscripten(N) {
  function emscripten_array(len) {
    var ptr = Module._malloc(len * 4);
    return [new Float32Array(Module.HEAPF32.buffer, ptr, len), ptr];
  }

  let [a, a_] = emscripten_array(N);
  let [b, b_] = emscripten_array(N);
  let [c, c_] = emscripten_array(N);
  const add = Module._add;

  return [() => add(a_, b_, c_, N), a, b, c, () => {
    Module._free(a_);
    Module._free(b_);
    Module._free(c_);
  }];
}

async function gen_wasmblr(N, unroll) {
  const wasm = Module._jit_add(N, unroll);
  const wasm_len = Module._jit_add_len(N, unroll);
  const wasm_data = new Uint8Array(Module.HEAP8.buffer, wasm, wasm_len);
  const m = await WebAssembly.compile(wasm_data);
  const instance = await WebAssembly.instantiate(m, {});

  let wasmblr_malloc_height = 0;
  let mem = instance.exports.mem;

  function wasmblr_array(len) {
    console.assert((mem.buffer.byteLength - wasmblr_malloc_height) > len * 4);
    let ptr = wasmblr_malloc_height;
    console.assert(([0, N * 4, N * 8]).indexOf(ptr) > -1, "allocated invalid ptr")
    let array = new Float32Array(mem.buffer, ptr, len);
    wasmblr_malloc_height += len * 4;
    return [array, ptr];
  }
  let [a, a_] = wasmblr_array(N);
  let [b, b_] = wasmblr_array(N);
  let [c, c_] = wasmblr_array(N);

  const add = instance.exports.add;

  return [() => add(a_, b_, c_), a, b, c];
}

async function gen_wasmblr_tuned(N) {
  let best = 0;
  let best_time = 1e9;
  for (let i = 0; Math.pow(2, i) < Math.min(1024, N / 4 + 2); ++i) {
    let [fn, w_a, w_b, w_c] = await gen_wasmblr(N, Math.pow(2, i));
    for (let _ = 0; _ < 100; ++_) {
      fn();
    }
    const t = performance.now();
    for (let _ = 0; _ < 1000; ++_) {
      fn();
    }
    const diff = performance.now() - t;
    if (diff < best_time) {
      best = i;
      best_time = diff;
    }
  }
  return [...await gen_wasmblr(N, Math.pow(2, best)), Math.pow(2, best)];
}

async function perf(N, name, fn) {
  const w0 = performance.now();
  for (let i = 0; i < warmup; ++i) {
    fn();
  }
  const w1 = performance.now();
  let iters = Math.min(Math.max(warmup * target_ms / (w1 - w0), 1), 1e6);
  const t0 = performance.now();
  for (let i = 0; i < iters; ++i) {
    fn();
  }
  const t1 = performance.now();
  const iters_sec = 1e3 * iters / (t1 - t0);
  const elem_sec = N * iters_sec;
  const gb_sec = elem_sec * 4 * 3 /* 2 read 1 write */ / 1e9;
  const round = (num) => Math.round(num * 100) / 100
  console.log(name, round(iters_sec), "iters/sec", `(${round(gb_sec)} GB/s)`);
}

async function benchmark(N) {
  let [pure_fn, p_a, p_b, p_c] = await gen_pure(N);
  let [typed_fn, t_a, t_b, t_c] = await gen_typed(N);
  let [emscripten_fn, e_a, e_b, e_c, emscripten_cleanup] = await gen_emscripten(N);
  let [wasmblr_fn, w_a, w_b, w_c] = await gen_wasmblr(N, wasmblr_unroll);
  let [wasmblr_tuned_fn, wt_a, wt_b, wt_c, unroll] = await gen_wasmblr_tuned(N);

  for (let i = 0; i < N; ++i) {
    let a = Math.random();
    let b = Math.random();
    p_a[i] = a;
    t_a[i] = a;
    e_a[i] = a;
    w_a[i] = a;
    wt_a[i] = a;

    p_b[i] = b;
    t_b[i] = b;
    e_b[i] = b;
    w_b[i] = b;
    wt_b[i] = b;
  }

  pure_fn();
  typed_fn();
  emscripten_fn();
  wasmblr_fn();
  wasmblr_tuned_fn();

  for (let i = 0; i < N; ++i) {
    function check(arr, name) {
      if (Math.abs(t_c[i] - arr[i]) > 0.01) {
        console.log("difference found at index", i, t_c[i], "vs", name, arr[i]);
        return false;
      }
      return true;
    }
    if (!check(p_c, "pure")) {
      return;
    }
    if (!check(e_c, "emscripten")) {
      return;
    }
    if (!check(w_c, "wasmblr")) {
      return;
    }
    if (!check(wt_c, "wasmblr (tuned)")) {
      return;
    }
  }

  console.log("benchmarking vec add of size", N);
  await perf(N, "  pure javascript:        ", pure_fn);
  await perf(N, "  typed arrays:           ", typed_fn);
  await perf(N, "  emscripten:             ", emscripten_fn);
  await perf(N, "  wasmblr:                ", wasmblr_fn);
  await perf(N, `  wasmblr (tuned ${unroll}):`.padEnd(26), wasmblr_tuned_fn);

  emscripten_cleanup()
}

em().then(function(m) {
  Module = m;
  // any larger and you'll need to recompile to give emscripten more memory
  (async () => {
  for (let i of [4, 64, 1024, 16 * 1024, 256 * 1024]) {
    await benchmark(i);
  }
  })();
});
