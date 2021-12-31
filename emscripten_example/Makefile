all:
	emcc add.cc -I../ -s EXPORTED_FUNCTIONS="['_add', '_jit_add', '_jit_add_len']" -O3 -msimd128 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1

test:
	node benchmark.js
