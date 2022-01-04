# Demo of how to do threading in WASM

The WebAssembly threading story is still a bit messy.
This folder contains a minimal working example of
two threads sharing the task of calcuation the square of an input.
It uses modern techniques and should be useful going forward.

## Concept

Each thread will be given every other element and the function
`square` is defined to take an offset (into the input and output).
We will call square twice with two different offsets and that will
yield the full result.

`square(4, 8)` means start on the 4th byte and square every 8 bytes.
It should hopefully be clear how this can be run twice in parallel
with different first arguments (0 and 4) to get the full result.

## Generate `square.wasm`

```
g++ thread.cc -I../ -o thread
./thread
```

## Then run the server with proper CORS

This is a hacked python http.server that will 
serve up the proper isolation (COOP + COEP).
Read more here: 
[MDN](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/SharedArrayBuffer#security_requirements)

```
python3 -m server
```

## Now open the browser and look in the console

Navigate to `localhost:8000/test.html` and open the console.
We are looking at the result of `test.mjs` and `worker.js` interacting.
