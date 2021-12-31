# Emscripten Integration Demo

See `add.cc` for example code and `Makefile` for the build command (I added `-O3 -msimd128` to make the benchmark more competitve).
To try this example, ensure that `emcc` is in your path.

```
cd emscripten_example
make
node benchmark.js
```

On my MacBook M1, these are the results I get:

```
benchmarking vec add of size 4
  pure javascript:     40755994.44 iters/sec (1.96 GB/s)
  typed arrays:        122821305.14 iters/sec (5.9 GB/s)
  emscripten (simd):   78090168.77 iters/sec (3.75 GB/s)
  wasmblr:             94504835.94 iters/sec (4.54 GB/s)
benchmarking vec add of size 8
  pure javascript:     79657378.12 iters/sec (7.65 GB/s)
  typed arrays:        71433339.22 iters/sec (6.86 GB/s)
  emscripten (simd):   26122516.79 iters/sec (2.51 GB/s)
  wasmblr:             56797105.33 iters/sec (5.45 GB/s)
benchmarking vec add of size 16
  pure javascript:     56146383.35 iters/sec (10.78 GB/s)
  typed arrays:        44756080.69 iters/sec (8.59 GB/s)
  emscripten (simd):   25679257.06 iters/sec (4.93 GB/s)
  wasmblr:             56024282.73 iters/sec (10.76 GB/s)
benchmarking vec add of size 32
  pure javascript:     32932391.44 iters/sec (12.65 GB/s)
  typed arrays:        26153595.79 iters/sec (10.04 GB/s)
  emscripten (simd):   25086927.59 iters/sec (9.63 GB/s)
  wasmblr:             54437100.61 iters/sec (20.9 GB/s)
benchmarking vec add of size 64
  pure javascript:     17774456.64 iters/sec (13.65 GB/s)
  typed arrays:        14088466.43 iters/sec (10.82 GB/s)
  emscripten (simd):   23616426.48 iters/sec (18.14 GB/s)
  wasmblr:             49573467.48 iters/sec (38.07 GB/s)
benchmarking vec add of size 128
  pure javascript:     8656331.55 iters/sec (13.3 GB/s)
  typed arrays:        6519041.69 iters/sec (10.01 GB/s)
  emscripten (simd):   20105924.95 iters/sec (30.88 GB/s)
  wasmblr:             35782856.54 iters/sec (54.96 GB/s)
benchmarking vec add of size 256
  pure javascript:     4636554.81 iters/sec (14.24 GB/s)
  typed arrays:        3408403.03 iters/sec (10.47 GB/s)
  emscripten (simd):   15152715.74 iters/sec (46.55 GB/s)
  wasmblr:             27397266.58 iters/sec (84.16 GB/s)
benchmarking vec add of size 512
  pure javascript:     2405497.95 iters/sec (14.78 GB/s)
  typed arrays:        1751368.44 iters/sec (10.76 GB/s)
  emscripten (simd):   10076727.57 iters/sec (61.91 GB/s)
  wasmblr:             19037099.64 iters/sec (116.96 GB/s)
benchmarking vec add of size 1024
  pure javascript:     1208222.18 iters/sec (14.85 GB/s)
  typed arrays:        887353.32 iters/sec (10.9 GB/s)
  emscripten (simd):   5994136.84 iters/sec (73.66 GB/s)
  wasmblr:             11814455.54 iters/sec (145.18 GB/s)
```
