# ビルド設定 --------------------------------------------

# CPUアーキテクチャ名 (riscv32のみ有効)
ARCH ?= riscv32

# ビルド時の生成物を置くディレクトリ
BUILD_DIR ?= build

# ClangやLLD、llvm-objcopyなどのLLVMツールチェインのプレフィックス
LLVM_PREFIX ?=

# 自動起動するサーバのリスト
BOOT_SERVERS ?= fs tcpip shell virtio_blk virtio_net pong

# 起動時に自動実行するシェルコマンド (テストを自動化したいときに便利)
#
# 例: make AUTORUN="cat hello.txt; shutdown"
AUTORUN ?=

# make V=1: Makefileの実行ログを表示する
V ?=

# make RELEASE=1: デバッグ用の機能の無効化とコンパイラの最適化の強化 (リリースビルド)
RELEASE ?=

# make run CPUS=<CPU数>: QEMUの仮想CPU数を指定する
QEMUFLAGS += -smp $(if $(CPUS),$(CPUS),1)

# make run GUI=1: QEMUのウィンドウを表示する
ifeq ($(GUI),)
QEMUFLAGS += -nographic -serial mon:stdio
endif

# make run GDBSERVER=1: QEMUをデバッガつきで起動する (make gdb と組み合わせて使う)
ifneq ($(GDBSERVER),)
QEMUFLAGS += -S -gdb tcp::7777
endif

# make run QEMU_DEBUG=1: QEMUのデバッグメッセージをqemu-debug.logに出力する
ifneq ($(QEMU_DEBUG),)
QEMUFLAGS += -d unimp,guest_errors,int,cpu_reset -D qemu-debug.log
endif

# -----------------------------------------------------
# ビルドに使うコマンド・変数を定義する
ifeq ($(OS),Windows_NT)
    SHELL   := cmd
    top_dir := $(shell echo %cd%)
    PROGRESS ?= echo
    PYTHON3  ?= py
    RM       := $(PYTHON3) $(top_dir)/tools/coreutils.py rm
    CP       ?= $(PYTHON3) $(top_dir)/tools/coreutils.py cp
    MKDIR    ?= $(PYTHON3) $(top_dir)/tools/coreutils.py mkdir
    FIND     ?= $(PYTHON3) $(top_dir)/tools/coreutils.py find

    export PYTHONUTF8=1

    ifeq ($(LLVM_PREFIX),)
        # wingetでインストールされた次のLLVMツールチェーンを使う
        # https://winget.run/pkg/LLVM/LLVM
        LLVM_PREFIX = "C:/Program Files/llvm/bin/"
    endif
else
    top_dir := $(shell pwd)
    RM := rm

    ifeq ($(shell uname), Darwin)
        ifeq ($(shell uname -m), arm64)
	    # Apple Sillicon
            HOMEBREW_PREFIX ?= /opt/homebrew/
	else
	    # Intel CPU
            HOMEBREW_PREFIX ?= /usr/local/
        endif

        ifeq ($(LLVM_PREFIX),)
            LLVM_PREFIX = $(HOMEBREW_PREFIX)/opt/llvm/bin/
        endif
    endif
endif

# 暗黙のビルドルールなどを無効化する
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

# $(V) がセットされていなければ、コマンド実行表示を無効化する
ifeq ($(V),)
.SILENT:
endif

#
#  ビルドに必要なコマンド
#
CC        := $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
LD        := $(LLVM_PREFIX)ld.lld$(LLVM_SUFFIX)
OBJCOPY   := $(LLVM_PREFIX)llvm-objcopy$(LLVM_SUFFIX)
ADDR2LINE := $(LLVM_PREFIX)llvm-addr2line$(LLVM_SUFFIX)
NM        := $(LLVM_PREFIX)llvm-nm$(LLVM_SUFFIX)
GDB       ?= riscv64-unknown-elf-gdb
PROGRESS  ?= printf "  \\033[1;96m%8s\\033[0m  \\033[1;m%s\\033[0m\\n"
PYTHON3   ?= python3
CP        ?= cp
MKDIR     ?= mkdir
FIND      ?= find
ECHO      ?= echo

# ビルド対象のサーバとライブラリの名前リスト (例: "fs tcpip shell ...")
all_servers := $(notdir $(patsubst %/build.mk, %, $(wildcard servers/*/build.mk)))
all_libs := $(notdir $(patsubst %/build.mk, %, $(wildcard libs/*/build.mk)))

# リンカーとCコンパイラのオプション
LDFLAGS :=
CFLAGS :=
CFLAGS += -g3 -std=c11 -ffreestanding -fno-builtin -nostdlib -nostdinc
CFLAGS += -fno-omit-frame-pointer -fno-optimize-sibling-calls -fstack-size-section
CFLAGS += -Wall -Wextra
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -Werror=int-conversion
CFLAGS += -Werror=incompatible-pointer-types
CFLAGS += -Werror=shift-count-overflow
CFLAGS += -Werror=switch
CFLAGS += -Werror=return-type
CFLAGS += -Werror=pointer-integer-compare
CFLAGS += -Werror=tautological-constant-out-of-range-compare
CFLAGS += -Werror=visibility
CFLAGS += -Wno-unused-parameter
# トップディレクトリをインクルードパスに追加する
CFLAGS += -I$(top_dir)
# 各ライブラリの "libs/<ライブラリ名>/<CPUアーキテクチャ名>" をインクルードパスに追加する
CFLAGS += $(foreach lib,$(all_libs),-Ilibs/$(lib)/$(ARCH))

# RISC-V特有のコンパイラオプション
ifeq ($(ARCH),riscv32)
CFLAGS += --target=riscv32 -mno-relax
endif

ifeq ($(RELEASE),)
# デバッグビルド
CFLAGS += -O1 -fsanitize=undefined -DDEBUG_BUILD
else
# リリースビルド
CFLAGS += -O3 -DRELEASE_BUILD
endif

# エミュレータ (QEMU) の起動コマンド
QEMU ?= $(QEMU_PREFIX)qemu-system-riscv32
QEMUFLAGS += --no-reboot -m 128 -machine virt,aclint=on -bios none
QEMUFLAGS += -global virtio-mmio.force-legacy=true
QEMUFLAGS += -drive file=$(hinafs_img),if=none,format=raw,id=drive0
QEMUFLAGS += -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0
QEMUFLAGS += -device virtio-net-device,netdev=net0,bus=virtio-mmio-bus.1
QEMUFLAGS += -object filter-dump,id=fiter0,netdev=net0,file=virtio-net.pcap
QEMUFLAGS += -netdev user,id=net0

hinaos_elf     := $(BUILD_DIR)/hinaos.elf
boot_elf       := $(BUILD_DIR)/servers/vm.elf
bootfs_bin     := $(BUILD_DIR)/bootfs.bin
hinafs_img     := $(BUILD_DIR)/hinafs.img

# HinaOSをビルドするコマンド
.PHONY: build
build: $(hinaos_elf) $(hinafs_img)
	$(PROGRESS) GEN $(BUILD_DIR)/compile_commands.json
	$(PYTHON3) ./tools/merge_compile_commands_json.py -o $(BUILD_DIR)/compile_commands.json $(BUILD_DIR)
	$(PYTHON3) ./tools/print_build_info.py --kernel-elf $(hinaos_elf) --bootfs-bin $(bootfs_bin) --hinafs-img $(hinafs_img)

# ビルド時に生成されたファイルたちを消すコマンド
.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR)

# QEMU (エミュレータ) でHinaOSを試すコマンド
.PHONY: run
run: build
	$(PROGRESS) QEMU $(hinaos_elf)
	$(QEMU) $(QEMUFLAGS) -kernel $(hinaos_elf)

# GDBでQEMUに接続するコマンド (make run GDBSERVER=1 と組み合わせて使う)
.PHONY: gdb
gdb:
	$(PROGRESS) GEN $(BUILD_DIR)/gdbinit
	$(PYTHON3) ./tools/generate_gdbinit.py -o $(BUILD_DIR)/gdbinit $(hinaos_elf).gdb $(wildcard $(BUILD_DIR)/servers/*.elf.gdb)
	$(PROGRESS) GDB $(BUILD_DIR)/gdbinit
	$(GDB) -q -ex "source $(BUILD_DIR)/gdbinit"

# 自動テスト
export QEMUFLAGS
.PHONY: test
test:
	$(PYTHON3)                                                             \
		-m pytest tests.py -p no:cacheprovider                         \
		--qemu "$(QEMU)" --make "$(MAKE)"                              \
		$(if $(FLAKE_RUNS),--flake-finder --flake-runs=$(FLAKE_RUNS))  \
		$(if $(RELEASE),--release,)

# トラブルシューティングに役立つ情報を表示するコマンド
.PHONY: doctor
doctor:
	$(ECHO) "OS:      $(shell $(PYTHON3) -m platform)"
	$(ECHO) "PYTHON3: $(shell $(PYTHON3) --version)"
	$(ECHO) "CC:      $(shell $(CC) --version)"
	$(ECHO) "LD:      $(shell $(LD) --version)"
	$(ECHO) "QEMU:    $(shell $(QEMU) --version)"
	$(ECHO) "OBJCOPY: $(shell $(OBJCOPY) --version)"
	$(ECHO) "MAKE:    $(shell $(MAKE) --version)"

# カーネルの実行ファイルの生成ルール (build/hinaos.elf)
executable   := $(hinaos_elf)
name         := kernel
dir          := kernel
build_dir    := $(BUILD_DIR)/kernel
objs-y       :=
cflags-y     := -DKERNEL -DBOOT_ELF_PATH='"$(boot_elf)"' -Ikernel/$(ARCH)/include
ldflags-y    := -T$(BUILD_DIR)/kernel/kernel.ld
libs-y       := common
extra-deps-y := $(BUILD_DIR)/kernel/kernel.ld
include kernel/build.mk
include mk/executable.mk

# ライブラリの生成ルール (build/libs/<ライブラリ名>.o)
$(foreach lib, $(all_libs),                                       \
	$(eval dir := libs/$(lib))                                \
	$(eval build_dir := $(BUILD_DIR)/$(dir))                  \
	$(eval output := $(BUILD_DIR)/libs/$(lib).o)              \
	$(eval objs-y :=)                                         \
	$(eval cflags-y :=)                                       \
	$(eval ldflags-y :=)                                      \
	$(eval subdirs-y :=)                                      \
	$(eval include $(dir)/build.mk)                           \
	$(eval include $(top_dir)/mk/lib.mk)                      \
)

# サーバの実行ファイルの生成ルール (build/servers/<サーバ名>.elf)
$(foreach server, $(all_servers),                                       \
	$(eval dir := servers/$(server))                                \
	$(eval build_dir := $(BUILD_DIR)/$(dir))                        \
	$(eval executable := $(BUILD_DIR)/servers/$(server).elf)        \
	$(eval name := $(server))                                       \
	$(eval objs-y :=)                                               \
	$(eval libs-y := common user)                                   \
	$(eval cflags-y :=)                                             \
	$(eval ldflags-y := -T$(BUILD_DIR)/servers/$(server)/user.ld)   \
	$(eval subdirs-y :=)                                            \
	$(eval extra-deps-y := $(BUILD_DIR)/servers/$(server)/user.ld)  \
	$(eval include $(dir)/build.mk)                                 \
	$(eval include $(top_dir)/mk/executable.mk)                     \
)

# Cファイルのコンパイル規則
$(BUILD_DIR)/%.o: %.c Makefile $(BUILD_DIR)/consts.mk libs/common/ipcstub.h
	$(PROGRESS) CC $<
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

# ビルドディレクトリ下にある (自動生成される) Cファイルのコンパイル規則
$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c Makefile $(BUILD_DIR)/consts.mk libs/common/ipcstub.h
	$(PROGRESS) CC $<
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

# アセンブリファイルのコンパイル規則
$(BUILD_DIR)/%.o: %.S Makefile
	$(PROGRESS) CC $<
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

# ビルドディレクトリ下にある (自動生成される) アセンブリファイルのコンパイル規則
$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.S Makefile
	$(PROGRESS) CC $<
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

# bootfsイメージ
server_elf_files := $(foreach server, $(filter-out vm, $(all_servers)), $(BUILD_DIR)/bootfs/$(server).elf)
$(bootfs_bin): $(server_elf_files) tools/mkbootfs.py
	$(PROGRESS) MKBOOTFS $@
	$(MKDIR) -p $(@D)
	$(PYTHON3) ./tools/mkbootfs.py -o $@ $(BUILD_DIR)/bootfs

# bootfsの中身を生成するためのルール
$(BUILD_DIR)/bootfs/%.elf: $(BUILD_DIR)/servers/%.elf
	$(MKDIR) -p $(@D)
	$(CP) $< $@

# hinafsイメージ
$(hinafs_img): ./tools/mkhinafs.py $(wildcard fs/*)
	$(PROGRESS) MKFS $@
	$(MKDIR) -p $(@D)
	$(PYTHON3) ./tools/mkhinafs.py $(@) fs

# メッセージ定義ファイル
libs/common/ipcstub.h: messages.idl tools/generate_ipcstub.py
	$(PROGRESS) IPCSTUB $@
	$(MKDIR) -p $(@D)
	$(PYTHON3) ./tools/generate_ipcstub.py -o $@ messages.idl

# カーネルのリンカスクリプト
$(BUILD_DIR)/kernel/kernel.ld: kernel/$(ARCH)/kernel.ld.template
	$(PROGRESS) CPP $@
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -E -x c -o $@ $<

# ユーザープログラムのリンカスクリプトで使われる定数
$(BUILD_DIR)/user_ld_params.h: tools/generate_user_ld_params.py $(BUILD_DIR)/consts.mk
	$(PROGRESS) GEN $@
	$(MKDIR) -p $(@D)
	$(PYTHON3) ./tools/generate_user_ld_params.py -o $@ $(all_servers)

# ユーザープログラムのリンカスクリプト
$(BUILD_DIR)/servers/%/user.ld: libs/user/$(ARCH)/user.ld.template $(BUILD_DIR)/user_ld_params.h
	$(PROGRESS) CPP $@
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS)  -E -x c -o $@ $< \
		-DSERVER_$(patsubst $(BUILD_DIR)/servers/%/user.ld,%,$@) \
		-include $(BUILD_DIR)/user_ld_params.h

# makeのコマンドライン引数や環境変数から指定できるビルド設定が変更された場合に、すべてのファイル
# を再コンパイルするためのギミック。
build_vars := ARCH BUILD_DIR BOOT_SERVERS AUTORUN RELEASE all_servers
$(BUILD_DIR)/consts.mk: FORCE
	$(PROGRESS) UPDATE $@
	$(MKDIR) -p $(@D)
	$(PYTHON3) ./tools/update_file_if_changed.py $@ \
		"$(foreach var,$(build_vars),$(var)=$(value $(var));)"
FORCE:

# 前回ビルド時にCコンパイラが生成したヘッダーの依存関係を読み込む。初回ビルドでは何もしない。
ifneq ($(wildcard $(BUILD_DIR)),)
    dep_files := $(shell $(FIND) $(BUILD_DIR) -name '*.deps')
    ifneq ($(dep_files),)
        -include $(dep_files)
        %.deps: %.c
    endif
endif
