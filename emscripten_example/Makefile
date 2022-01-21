all:
	emcc add.cc -I../ -s EXPORTED_FUNCTIONS="['_add', '_jit_add', '_jit_add_len', '_free']" -s EXTRA_EXPORTED_RUNTIME_METHODS="['cwrap', 'ccall']" -O3 -DSIMD=1 -msimd128 -s ASSERTIONS=1 -s SINGLE_FILE=1 -s MODULARIZE=1 -s 'EXPORT_NAME="createMyModule"' -o add.js

no_simd:
	emcc add.cc -I../ -s EXPORTED_FUNCTIONS="['_add', '_jit_add', '_jit_add_len', '_free']" -s EXTRA_EXPORTED_RUNTIME_METHODS="['cwrap', 'ccall']" -O3 -s ASSERTIONS=1 -s SINGLE_FILE=1 -s MODULARIZE=1 -s 'EXPORT_NAME="createMyModule"' -o add.js

wasmblr_only:
	emcc add.cc -I../ -s EXPORTED_FUNCTIONS="['_jit_add', '_jit_add_len']" -Os -s SINGLE_FILE=1 -s MODULARIZE=1 -s ENVIRONMENT='web' -s 'EXPORT_NAME="createMyModule"' -fno-rtti -fno-exceptions -o add.js

benchmark:
	node benchmark.js
