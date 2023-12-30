objs-y          += main.o wasm.o
cflags-y        += -D__wasm_path__=\"$(build_dir)/app.wasm\"

# build wasm first
$(build_dir)/wasm.o: $(build_dir)/app.wasm

$(build_dir)/app.wasm: $(dir)/app.c
	$(PROGRESS) CC app.wasm
	$(CC) $(WASM_CFLAGS) -o $@ $^