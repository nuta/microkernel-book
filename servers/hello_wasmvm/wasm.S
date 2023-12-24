    .section .data
    .global wasm_start
wasm_start:
     .incbin __wasm_path__
wasm_end:
    .global wasm_size 
wasm_size:
    .int wasm_end - wasm_start