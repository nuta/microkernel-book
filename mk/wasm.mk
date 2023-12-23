WASM_CFLAGS := --target=wasm32 -O0          \
                -Wl,--strip-all,--no-entry  \
				-fno-builtin				\
                -nostdlib                   \
				-nostdinc					\
                -Wl,--allow-undefined

WASM_CFLAGS += $(cflags-y)

srcs := $(addprefix $(dir)/, $(srcs-y))
$(executable): $(srcs)
	$(PROGRESS) CC $@
	$(CC) $(WASM_CFLAGS) -o $@ $^