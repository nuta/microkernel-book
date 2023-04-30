objs-y += printf.o syscall.o malloc.o init.o ipc.o task.o driver.o dmabuf.o
subdirs-y += $(ARCH) virtio
global-cflags-y += -I$(top_dir)/libs/user/arch/$(ARCH)
