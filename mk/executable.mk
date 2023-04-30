# 実行ファイル (カーネルと各サーバ) の生成ルール
#
# kernelまたはservers/<サーバ名>配下に build.mk という名前のファイルを作ると、
# ビルドシステムがそれを認識して必要なビルドルールを構築する。
#
# build.mk内では次の変数を使って宣言的にビルドルールを定義する。原則として、
# 末尾に追記する (+=) ことで定義するのがおすすめ。上書き (:=) するとデフォルト値を
# 失ってしまう。
#
#   - objs-y:    生成するオブジェクトファイルのリスト。
#                C/アセンブリファイル共に拡張子を.oにする必要があり、
#                hello.cとhello.Sのような同名のファイルは扱えない。
#   - cflags-y:  Cコンパイラのオプション
#   - ldflags-y: リンカーのオプション
#   - libs-y:    リンクするライブラリ名のリスト。
#                カーネルはデフォルトでcommon、サーバはデフォルトでcommonとuserがセットされている。
#   - subdirs-y: サブディレクトリのリスト。定義されているとビルドシステムそれらの
#                サブディレクトリに入っているbuild.mkを読み込む。

# サブディレクトリ (subdir-y) を辿って必要なオブジェクトファイルを列挙する
build_dir := $(BUILD_DIR)/$(dir)
objs := $(addprefix $(build_dir)/, $(objs-y))
dir-saved = $(dir)
$(foreach subdir, $(subdirs-y),                                 \
	$(eval dir := $(dir-saved)/$(subdir))                   \
	$(eval build_dir := $(BUILD_DIR)/$(dir))                \
	$(eval objs-y :=)                                       \
	$(eval include $(dir)/build.mk)                         \
	$(eval objs += $(addprefix $(build_dir)/, $(objs-y)))   \
)

objs := \
	$(objs) \
	$(foreach lib, $(libs-y), $(BUILD_DIR)/libs/$(lib).o) \
	$(BUILD_DIR)/program_names/$(name).o

# 各オブジェクトファイルのコンパイルルール
$(objs): CFLAGS := $(CFLAGS) $(cflags-y)

# 実行ファイルの生成ルール (build/hinaos.elf または build/servers/<サーバ名>.elf)
#
# 1. リンカーでオブジェクトファイルをくっつけて実行ファイルを生成する (*.elf.tmp)
# 2. 実行ファイルからシンボルテーブルを抽出する (*.symbols)
# 3. スタックトレース用のシンボルテーブルを埋め込む (*.elf.debug)
# 4. GDB用にシンボル名に「プログラム名.」を付加した別のELFファイルを生成する (*.elf.gdb)
# 5. bootfsの容量削減のために、.elf.debugファイルからデバッグ情報を削除する (*.elf)
$(executable): LDFLAGS := $(LDFLAGS) $(ldflags-y)
$(executable): OBJS := $(objs)
$(executable): NAME := $(name)
$(executable): $(objs) $(extra-deps-y) tools/embed_symbols.py
	$(PROGRESS) LD $(@)
	$(MKDIR) -p $(@D)
	$(LD) $(LDFLAGS) -Map $(@:.elf=.map) -o $(@).tmp $(OBJS)

	$(PROGRESS) NM $(@)
	$(NM) --radix=x --defined-only $(@).tmp > $(@:.elf=.symbols)

	$(PROGRESS) SYMBOLS $(@:.elf=.symbols)
	$(PYTHON3) ./tools/embed_symbols.py $(@:.elf=.symbols) $(@).tmp $(@).debug

	$(PROGRESS) GEN $(@).gdb
	$(OBJCOPY) --only-keep-debug --prefix-symbols="$(NAME)." $(@).debug $(@).gdb

	$(PROGRESS) STRIP $(@)
	$(OBJCOPY) --strip-all $(@).debug $(@)

	$(RM) $(@).tmp

# プログラム名を返す関数を生成する。INFOやWARNマクロ内で利用する。
$(BUILD_DIR)/program_names/$(name).c: NAME := $(name)
$(BUILD_DIR)/program_names/$(name).c: tools/generate_program_name.py
	$(MKDIR) -p $(@D)
	$(PYTHON3) tools/generate_program_name.py -o $(@) $(NAME)
