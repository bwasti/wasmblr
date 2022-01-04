async function test() {
  const response = await fetch('./square.wasm');
  const wasm = await response.arrayBuffer();
  const wasm_module = await WebAssembly.compile(wasm);
  const len = 1024;
  const pages = (len * 2 * 4) / (1 << 16) + 1;
  let memory = new WebAssembly.Memory({
    initial: pages,
    maximum: pages + 1,
    shared: true
  });
  const wasm_instance = await WebAssembly.instantiate(wasm_module, {
    env: {
      memory: memory
    }
  });

  const input = new Float32Array(memory.buffer, 0, 1024);
  const output = new Float32Array(memory.buffer, 1024 * 4, 1024);
  input[0] = 0.3;
  input[1] = 0.3;
  input[2] = 0.3;
  const worker0 = new Worker('./worker.js');
  const worker1 = new Worker('./worker.js');
  let w0_done = false;
  let w1_done = false;
  worker0.addEventListener('message', function(e) {
    w0_done = true;
    if (w1_done) {
      console.log(output[0], output[1]);
    }
  });
  worker1.addEventListener('message', function(e) {
    w1_done = true;
    if (w0_done) {
      console.log(output[0], output[1]);
    }
  });
  worker0.postMessage([wasm_module, memory, 0, 8]);
  worker1.postMessage([wasm_module, memory, 4, 8]);
}

test();