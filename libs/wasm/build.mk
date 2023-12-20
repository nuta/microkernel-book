WAMR_ROOT           := $(top_dir)/libs/third_party/wasm-micro-runtime
WAMR_CORE_ROOT      := $(WAMR_ROOT)/core
WAMR_SHARED_ROOT    := $(WAMR_CORE_ROOT)/shared
WAMR_IWASM_ROOT     := $(WAMR_CORE_ROOT)/iwasm

objs-y += platform.o

# see line 5793 of wasm_runtime_common.c
cflags-y += -Wno-error=int-conversion

# defines
cflags-y +=     -DWASM_ENABLE_INTERP=1          \
                -DBH_MALLOC=wasm_runtime_malloc \
                -DBH_FREE=wasm_runtime_free

# include directories
cflags-y +=     -I$(WAMR_IWASM_ROOT)/include                \
                -I$(WAMR_SHARED_ROOT)/platform/include      \
                -I$(top_dir)/$(dir)/include                 \
                -I$(WAMR_IWASM_ROOT)/interpreter            \
                -I$(WAMR_SHARED_ROOT)/mem-alloc             \
                -I$(WAMR_IWASM_ROOT)/common                 \
                -I$(WAMR_SHARED_ROOT)/utils

# WAMR object files
objs-y +=       wasm_interp_classic.o   \
                wasm_loader.o           \
                wasm_runtime.o          \
                mem_alloc.o             \
                ems_alloc.o             \
                ems_hmu.o               \
                ems_kfc.o               \
                wasm_application.o      \
                wasm_blocking_op.o      \
                wasm_c_api.o            \
                wasm_exec_env.o         \
                wasm_memory.o           \
                wasm_native.o           \
                wasm_runtime_common.o   \
                bh_assert.o             \
                bh_bitmap.o             \
                bh_common.o             \
                bh_hashmap.o            \
                bh_list.o               \
                bh_log.o                \
                bh_queue.o              \
                bh_vector.o             \
                runtime_timer.o         \
                math.o                  \
                invokeNative_riscv.o

# WAMR source code paths
VPATH +=    $(WAMR_IWASM_ROOT)/interpreter/             \
            $(WAMR_SHARED_ROOT)/mem-alloc               \
            $(WAMR_SHARED_ROOT)/mem-alloc/ems           \
            $(WAMR_IWASM_ROOT)/common                   \
            $(WAMR_IWASM_ROOT)/common/arch              \
            $(WAMR_SHARED_ROOT)/utils                   \
            $(WAMR_SHARED_ROOT)/utils/uncommon          \
            $(WAMR_SHARED_ROOT)/platform/common/math    \
            $(WAMR_IWASM_ROOT)/common/arch

# build rules
$(build_dir)/%.o: %.S
	$(PROGRESS) CC $@
	$(CC) $(CFLAGS) $(extra-cflags)-c -o $@ $^

$(build_dir)/%.o: %.c
	$(PROGRESS) CC $@
	$(CC) $(CFLAGS) $(extra-cflags) -c -o $@ $^