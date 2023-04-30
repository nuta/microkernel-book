objs-y += main.o shellcode.bin.o

$(BUILD_DIR)/servers/crack/main.o: $(BUILD_DIR)/servers/crack/shellcode.bin.o

$(BUILD_DIR)/servers/crack/shellcode.bin.o: servers/crack/shellcode.S
	$(MKDIR) -p $(@D)
	$(CC) --target=riscv32 -c -o $@.tmp1 $^
	$(OBJCOPY) -Obinary $@.tmp1 $@.tmp2
	cd $(BUILD_DIR)/servers/crack && \
		$(CP) shellcode.bin.o.tmp2 shellcode.bin && \
		$(OBJCOPY) -Ibinary -Oelf32-littleriscv shellcode.bin shellcode.bin.o
