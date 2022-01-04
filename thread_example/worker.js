self.addEventListener('message', function(e) {
  const wasm_module = e.data[0];
  const memory = e.data[1];
  WebAssembly.instantiate(wasm_module, {
    env: {
      memory: memory
    }
  }).then((wasm_instance) => {
    const square = wasm_instance.exports.square;
    const off = e.data[2];
    const stride = e.data[3];
    square(off, stride);
    self.postMessage("done");
  });
});