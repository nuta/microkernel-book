objs-y    += main.o printk.o memory.o task.o interrupt.o ipc.o syscall.o bootelf.o \
             hinavm.o wasmvm.o

cflags-y  += -Ilibs/kernel/wasm/include
libs-y    += kernel
subdirs-y += riscv32

$(build_dir)/bootelf.o: $(boot_elf)