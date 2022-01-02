const wasmblr_unroll = 16;
const target_ms = 50;
const tuning_ms = 10;

function log(...args) {
    const str = args.reduce((a, b) => {
        return a + " " + b;
    }, "");
    document.querySelector('pre').appendChild(document.createTextNode(str));
    document.querySelector('pre').appendChild(document.createElement('br'));
}

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
    let a = new Float32Array(N).fill(0);
    let b = new Float32Array(N).fill(0);
    let c = new Float32Array(N).fill(0);

    function add() {
        for (let i = 0; i < N; ++i) {
            c[i] = a[i] + b[i];
        }
    }

    return [add, a, b, c];
}

async function gen_emscripten(Module, N) {
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

async function gen_wasmblr(Module, N, unroll) {
    const wasm = Module._jit_add(N, unroll);
    const wasm_len = Module._jit_add_len(N, unroll);
    const wasm_data = new Uint8Array(Module.HEAP8.buffer, wasm, wasm_len);
    const m = await WebAssembly.compile(wasm_data);
    const instance = await WebAssembly.instantiate(m, {});
    Module._free(wasm);

    let wasmblr_malloc_height = 0;
    let mem = instance.exports.mem;

    function wasmblr_array(len) {
        console.assert((mem.buffer.byteLength - wasmblr_malloc_height) > len * 4);
        let ptr = wasmblr_malloc_height;
        let array = new Float32Array(mem.buffer, ptr, len);
        wasmblr_malloc_height += len * 4;
        return [array, ptr];
    }
    let [a, a_] = wasmblr_array(N);
    let [b, b_] = wasmblr_array(N);
    let [c, c_] = wasmblr_array(N);

    const add = instance.exports.add;

    return [() => add(a_, b_, c_), a, b, c, () => {}];
}

async function gen_wasmblr_tuned(Module, N) {
    let best = 0;
    let best_time = 0;
    for (let i = 0; Math.pow(2, i) < Math.min(1024, N / 4 + 2); ++i) {
        let [fn, w_a, w_b, w_c, cleanup] = await gen_wasmblr(Module, N, Math.pow(2, i));

        let diff = 0;
        let num_iters = 10;
        while (diff < tuning_ms) {
            num_iters *= 2;
            const t0 = performance.now();
            for (let i = 0; i < num_iters; ++i) {
                fn();
            }
            const t1 = performance.now();
            diff = t1 - t0;
        }

        if ((num_iters / diff) > best_time) {
            best = i;
            best_time = num_iters / diff;
        }
        cleanup();
    }
    return [...await gen_wasmblr(Module, N, Math.pow(2, best)), Math.pow(2, best)];
}

function perf(N, name, fn) {
    let diff = 0;
    let num_iters = 10;
    fn(); // warmup with 1 run
    while (diff < (target_ms / 2)) {
        num_iters *= 2;
        const w0 = performance.now();
        for (let i = 0; i < num_iters; ++i) {
            fn();
        }
        const w1 = performance.now();
        diff = w1 - w0;
    }
    const iters_sec = 1e3 * num_iters / diff;
    const elem_sec = N * iters_sec;
    const gb_sec = elem_sec * 4 * 3 /* 2 read 1 write */ / 1e9;
    const round = (num) => Math.round(num * 100) / 100
    log(name, `${round(gb_sec)} GB/s`, `(${round(iters_sec)} iters/sec)`);
    return gb_sec;
}

async function benchmark(Module, N) {
    let [pure_fn, p_a, p_b, p_c] = await gen_pure(N);
    let [typed_fn, t_a, t_b, t_c] = await gen_typed(N);
    let [emscripten_fn, e_a, e_b, e_c, emscripten_cleanup] = await gen_emscripten(Module, N);
    let [wasmblr_fn, w_a, w_b, w_c, wasmblr_cleanup] = await gen_wasmblr(Module, N, wasmblr_unroll);
    let [wasmblr_tuned_fn, wt_a, wt_b, wt_c, wasmblr_tuned_cleanup, unroll] = await gen_wasmblr_tuned(Module, N);

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
            console.log("error");
            return;
        }
        if (!check(e_c, "emscripten")) {
            console.log("error");
            return;
        }
        if (!check(w_c, "wasmblr")) {
            console.log("error");
            return;
        }
        if (!check(wt_c, "wasmblr (tuned)")) {
            console.log("error");
            return;
        }
    }

    log();
    log("benchmarking vec add of size", N);
    const p_gbs = perf(N, "  pure javascript:        ", pure_fn);
    const t_gbs = perf(N, "  typed arrays:           ", typed_fn);
    const e_gbs = perf(N, "  emscripten (simd):      ", emscripten_fn);
    const w_gbs = perf(N, "  wasmblr:                ", wasmblr_fn);
    const wt_gbs = perf(N, `  wasmblr (tuned ${unroll}):`.padEnd(26), wasmblr_tuned_fn);

    emscripten_cleanup()
    wasmblr_cleanup()
    wasmblr_tuned_cleanup()

    return [p_gbs, t_gbs, e_gbs, w_gbs, wt_gbs];
}