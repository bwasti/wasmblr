## Matmul Example

This folder contains an example using emscripten and wasmblr to generate various tuned matrix multiplication implementations
on the fly in the browser.

A writeup can be found here: https://jott.live/markdown/mm_wasm

To use the demo ensure that `emcc` is in your path.

```
make
```

and then host a server

```
python3 -m http.server
```

http://localhost should have a tuning script up and running.
