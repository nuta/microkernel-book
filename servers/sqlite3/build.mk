objs-y += main.o sqlite3.o
# SQLiteのコンパイルオプション:
#
#  -DSQLITE_OS_OTHER=1: UNIXでもWindowsでもない独自の環境であることを示す。
#  -DSQLITE_THREADSAFE=0: ミューテックスなどのスレッド周りの機能を無効にする。
#  -DSQLITE_OMIT_FLOATING_POINT=1: 浮動小数点演算を無効にする。
#  -DSQLITE_DEFAULT_LOCKING_MODE=1: EXCLUSIVEモードでロックをとる。予期せずデータベースを閉じてしまうっぽい。
cflags-y += -DSQLITE_OS_OTHER=1 -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_FLOATING_POINT=1 -DSQLITE_DEFAULT_LOCKING_MODE=1
cflags-y += -I$(top_dir)/servers/sqlite3

# 浮動小数点演算のソフトウェア実装 (libgcc.a) を用意する。
# ここでは、macOSのHomebrewでインストールしたriscv-gnu-toolchainのものを使う。
objs-y += libgcc.o
libgcc_archive := /opt/homebrew/opt/riscv-gnu-toolchain/lib/gcc/riscv64-unknown-elf/12.2.0/rv32imac/ilp32/libgcc.a
$(BUILD_DIR)/servers/sqlite3/libgcc.o:
	$(PROGRESS) LD $(@)
	mkdir -p $(@D)
	$(LD) -r --whole-archive -o $@ $(libgcc_archive)

# SQLiteのテスト用データベースを作成する。
test.sqlite3:
	$(PROGRESS) SQLITE3 $(@)
	sqlite3 test.sqlite3 < servers/sqlite3/test.sql

# SQLiteのテスト用データベースをELFファイルに埋め込む。
objs-y += test_database.o
$(BUILD_DIR)/servers/sqlite3/test_database.o: test.sqlite3
	$(PROGRESS) OBJCOPY $(@)
	mkdir -p $(@D)
	$(OBJCOPY) -Ibinary -Oelf32-littleriscv test.sqlite3 $@

# PDCLibのソースコードを追加する。
# 今のところ、実際に使われているのはstrcspn()とstrrchr()のみ使われている。
objs-y += pdclib/functions/string/strcspn.o
objs-y += pdclib/functions/string/strrchr.o
cflags-y += -I$(top_dir)/servers/sqlite3/pdclib/include
cflags-y += -I$(top_dir)/servers/sqlite3/pdclib_plat
