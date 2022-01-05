# Demo of dynamically determining peak FLOPs

This file sweeps through a range of arithmetic intensities
to help determine the best configurations for running MAC-based
operations such as matrix multiplication or convolution.

To run it in browser right now: https://bwasti.github.io/wasmblr/flops/

## Build the jit.js file

```
make
```

## Use the jit.js file

The benchmark code uses the generated `jit.js` file.
An `index.html` file is provided to run the benchmark 
in the browser.

```
python3 -m http.server
```
