# ライブラリの生成ルール
#
# libs/<ライブラリ名>配下に build.mk という名前のファイルを作ると、
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
#   - subdirs-y: サブディレクトリのリスト。定義されているとビルドシステムそれらの
#                サブディレクトリに入っているbuild.mkを読み込む。

# サブディレクトリ (subdir-y) を辿って必要なオブジェクトファイルを列挙する
build_dir := $(BUILD_DIR)/$(dir)
objs := $(addprefix $(build_dir)/, $(objs-y))
dir-saved := $(dir)
$(foreach subdir, $(subdirs-y),                                 \
	$(eval dir := $(dir-saved)/$(subdir))                   \
	$(eval build_dir := $(BUILD_DIR)/$(dir))                \
	$(eval objs-y :=)                                       \
	$(eval include $(dir)/build.mk)                         \
	$(eval objs += $(addprefix $(build_dir)/, $(objs-y)))   \
)

# ライブラリファイルの生成ルール (build/libs/<ライブラリ名>.o)
#
# リンカーの-rオプションを使って単一のオブジェクトファイルにまとめる。
$(objs): CFLAGS := $(CFLAGS) $(cflags-y)
$(output): OBJS := $(objs)
$(output): $(objs)
	$(PROGRESS) LD $(@)
	$(MKDIR) -p $(@D)
	$(LD) -r -o $(@) $(OBJS)
