objs-y    += main.o printk.o memory.o task.o interrupt.o ipc.o syscall.o bootelf.o \
             hinavm.o wasmvm.o libgcc.a
# add "libs/wasm/include" to include path
cflags-y  += -Ilibs/wasm/include
libs-y    += wasm
subdirs-y += riscv32

$(build_dir)/bootelf.o: $(boot_elf)

# WAMR builds require libgcc.a, so use the one from riscv-gnu-toolchain
# To build from source, execute the following commands
# $ git clone https://github.com/riscv-collab/riscv-gnu-toolchain
# $ cd riscv-gnu-toolchain
# $ ./configure --prefix=/opt/riscv32 --with-arch=rv32imac --with-abi=ilp32 --with-target-cflags=-mno-relax
# $　sudo make -j$(nproc)
libgcc_archive := /opt/riscv32/lib/gcc/riscv32-unknown-elf/13.2.0/libgcc.a
$(build_dir)/libgcc.a:
	$(PROGRESS) CP $(@)
	$(MKDIR) -p $(@D)
	$(CP) $(libgcc_archive) $@