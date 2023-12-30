objs-y += boot.o setup.o task.o vm.o mp.o switch.o handler.o trap.o usercopy.o debug.o uart.o plic.o libgcc.a

# To build WAMR, you will need libgcc.a, so use the one from the riscv-gnu-toolchain(https://github.com/riscv-collab/riscv-gnu-toolchain). 
# The pre-built version is available at kernel/riscv32/libgcc.a. 
# If you want to build it from the source code, please follow the instructions in the README. 
# Use the following command for configuration.
# $ ./configure --prefix=/opt/riscv32 --with-arch=rv32imac --with-abi=ilp32 --with-target-cflags=-mno-relax

$(build_dir)/libgcc.a: $(dir)/libgcc.a
	$(PROGRESS) CP $@
	$(CP) $< $@
