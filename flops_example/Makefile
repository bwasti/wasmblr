all:
	emcc jit.cc -I../ -s EXPORTED_FUNCTIONS="['_jit_mac', '_jit_mac_len', '_free']" -O3 -s SINGLE_FILE=1 -s MODULARIZE=1 -s 'EXPORT_NAME="createMyModule"' -o jit.js

