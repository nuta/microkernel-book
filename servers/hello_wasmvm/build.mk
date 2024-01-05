objs-y          += main.o wasm.o
cflags-y        += -D__wasm_path__=\"$(build_dir)/app.wasm\"

# build wasm first
$(build_dir)/wasm.o: $(build_dir)/app.wasm

# Remove compiler flags specific to riscv and add ones for WASM
$(build_dir)/app.wasm: WASMCFLAGS += -Wl,--strip-all,--no-entry,--allow-undefined

$(build_dir)/app.wasm: $(dir)/app.c
	$(PROGRESS) CC app.wasm
	$(CC) $(WASMCFLAGS) -o $@ $^