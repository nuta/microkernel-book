build_dir := $(BUILD_DIR)/$(dir)
objs := $(addprefix $(build_dir)/, $(objs-y))
dir-saved = $(dir)
$(foreach subdir, $(subdirs-y),                           	\
	$(eval dir := $(dir-saved)/$(subdir))                   \
	$(eval build_dir := $(BUILD_DIR)/$(dir))                \
	$(eval objs-y :=)                                       \
	$(eval include $(dir)/build.mk)                         \
	$(eval objs += $(addprefix $(build_dir)/, $(objs-y)))   \
)

objs := \
	$(objs) \
	$(foreach lib, $(libs-y), $(BUILD_DIR)/libs/$(lib).o)

$(objs): WASMCFLAGS := $(WASMCFLAGS) $(cflags-y)

$(executable): WASMLDFLAGS := $(WASMLDFLAGS) $(ldflags-y)
$(executable): OBJS := $(objs)
$(executable): $(objs) $(extra-deps-y)
	$(PROGRESS) WASMLD $(@)
	$(MKDIR) -p $(@D)
	$(WASMLD) $(WASMLDFLAGS) -Map $(@:.wasm=.map) -o $(@) $(OBJS)