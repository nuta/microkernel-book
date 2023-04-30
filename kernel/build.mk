objs-y += main.o printk.o memory.o task.o interrupt.o ipc.o syscall.o bootelf.o \
          hinavm.o
subdirs-y += riscv32

$(build_dir)/bootelf.o: $(boot_elf)
