objs-y          += main.o wasm.o
extra-deps-y    +=	$(build_dir)/app.wasm
cflags-y        += -D__wasm_path__=\"$(build_dir)/app.wasm\"

wasm-cflags := --target=wasm32 -O0          \
                -Wl,--strip-all,--no-entry  \
                -nostdlib                   \
                -Wl,--allow-undefined		\
				-I$(top_dir)

# build wasm first
$(build_dir)/wasm.o: $(build_dir)/app.wasm

$(build_dir)/app.wasm: $(dir)/app.c
	$(PROGRESS) CC app.wasm
	$(CC) $(wasm-cflags) -o $@ $^