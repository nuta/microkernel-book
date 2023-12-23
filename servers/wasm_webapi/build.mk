WAMR_IWASM_ROOT := $(top_dir)/libs/third_party/wasm-micro-runtime/core/iwasm
libs-y          += wasm
objs-y          += main.o libgcc.a wasm.o
extra-deps-y    +=	$(build_dir)/app.wasm
cflags-y        +=  -I$(WAMR_IWASM_ROOT)/include                \
                    -I$(top_dir)/$(dir)/include                 \
                    -D__wasm_path__=\"$(build_dir)/app.wasm\"

# WAMR builds require libgcc.a, so use the one from riscv-gnu-toolchain
# To build from source, execute the following commands
# $ git clone https://github.com/riscv-collab/riscv-gnu-toolchain
# $ cd riscv-gnu-toolchain
# $./configure --prefix=/opt/riscv32 --with-arch=rv32imac --with-abi=ilp32 --with-target-cflags=-mno-relax
# $sudo make -j$(nproc)
libgcc_archive := /opt/riscv32/lib/gcc/riscv32-unknown-elf/13.2.0/libgcc.a
$(build_dir)/libgcc.a:
	$(PROGRESS) CP $(@)
	$(MKDIR) -p $(@D)
	$(CP) $(libgcc_archive) $@

wasm-cflags := --target=wasm32 -O0          \
                -Wl,--strip-all,--no-entry  \
                -fno-builtin 				\
                -nostdlib                   \
				-nostdinc 					\
                -Wl,--allow-undefined		\
				-I$(top_dir)

# Build wasm first
$(build_dir)/wasm.o: $(build_dir)/app.wasm

$(build_dir)/app.wasm: $(dir)/app.c
	$(PROGRESS) CC app.wasm
	$(CC) $(wasm-cflags) -o $@ $^