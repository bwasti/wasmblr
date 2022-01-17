all:
	emcc mm.cc -I../ -s EXPORTED_FUNCTIONS="['_jit_mm', '_jit_mm_len', '_free']" -O3 -s ASSERTIONS=1 -s SINGLE_FILE=1 -s MODULARIZE -s 'EXPORT_NAME="createMyModule"' -o mm.js
