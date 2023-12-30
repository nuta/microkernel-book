srcs := $(addprefix $(dir)/, $(srcs-y))
$(executable): $(srcs)
	$(PROGRESS) CC $@
	$(CC) $(WASM_CFLAGS) -o $@ $^