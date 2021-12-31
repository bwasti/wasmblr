all:
	emcc add.cc -I../ -s EXPORTED_FUNCTIONS="['_add', '_jit_add', '_jit_add_len', '_free']" -s EXTRA_EXPORTED_RUNTIME_METHODS="['cwrap', 'ccall']" -O3 -msimd128 -s ASSERTIONS=1

no_simd:
	emcc add.cc -I../ -s EXPORTED_FUNCTIONS="['_add', '_jit_add', '_jit_add_len', '_free']" -s EXTRA_EXPORTED_RUNTIME_METHODS="['cwrap', 'ccall']" -O3 -s ASSERTIONS=1

wasmblr_only:
	emcc add.cc -I../ -s EXPORTED_FUNCTIONS="['_jit_add', '_jit_add_len']" -O3

test:
	node benchmark.js
