objs-y += list.o vprintf.o backtrace.o symbol_table.o string.o ubsan.o error.o message.o
subdirs-y += $(ARCH)
cflags-y += -I$(dir)/include
