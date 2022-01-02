# Emscripten Integration Demo

A detailed writeup of the contents of this folder can be found here: https://jott.live/markdown/javascript_vector_addition

See `add.cc` for various implementations of vector addition and `Makefile` for the build command (I added `-O3 -msimd128` to make the benchmark more competitve).
To try this example, ensure that `emcc` is in your path.

```
cd emscripten_example
make
node benchmark.js
```

If you change the value of `wasmblr_unroll` at the top of `benchmark.js`, different code will be generated.
Amping it all the way up to 1024 shows some benefit over the default 16.

On my MacBook M1, these are the results I get in node.js (`wasmblr_unroll = 16`):

![](https://i.imgur.com/SuInbUY.png)
