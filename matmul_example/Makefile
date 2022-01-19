all:
	emcc mm.cc -I../ -s EXPORTED_FUNCTIONS="['_jit_mm', '_jit_mm_len', '_jit_mm_naive', '_jit_mm_naive_len', '_jit_mm_nosimd', '_jit_mm_nosimd_len', '_free']" -O3 -s ASSERTIONS=1 -s SINGLE_FILE=1 -s MODULARIZE -s 'EXPORT_NAME="createMyModule"' -s INITIAL_MEMORY=67108864 -o mm.js
