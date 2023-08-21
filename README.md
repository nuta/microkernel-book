# SQLite3 on HinaOS

このブランチは、著名なRDBMSである[SQLite](https://www.sqlite.org/index.html)をHinaOS上に移植したものです。

## 試し方

```
$ make run BOOT_SERVERS=sqlite3 LTO=1
```

次のように表示されれば成功です。

```
[sqlite3] opening database...
[sqlite3] executing query: SELECT * FROM users;
[sqlite3] id = 1
[sqlite3] name = John Smith
[sqlite3] email = john@example.com
[sqlite3] id = 2
[sqlite3] name = Lorem Ipsum
[sqlite3] email = lorem@example.com
[sqlite3] done!
```

## 同梱物

このブランチでは、新たに `servers/sqlite3` というディレクトリを追加し、その中にSQLiteと他に必要なファイルを置いています。

```
 .gitignore                                         |      1 +
 Makefile                                           |      5 +
 servers/sqlite3/build.mk                           |     25 +
 servers/sqlite3/libgcc.o                           |    Bin 0 -> 2397584 bytes
 servers/sqlite3/main.c                             |    201 +
 servers/sqlite3/shell.c                            |  28032 ++
 servers/sqlite3/sqlite3.c                          | 247629 ++++++++++++++++++
 servers/sqlite3/sqlite3.h                          |  13068 +
 servers/sqlite3/stdarg.h                           |      5 +
 servers/sqlite3/stddef.h                           |      6 +
 servers/sqlite3/test.sql                           |      8 +
 servers/sqlite3/pdclib                             | (omitted)
 318 files changed, 329456 insertions(+)
```

- `libgcc.o`: 浮動小数点演算を行うために必要なライブラリ。
- `sqlite3.c`: SQLiteのソースコード。
- `pdclib`: 標準Cライブラリの実装。
- `stdarg.h`, `stddef.h`: PDCLibが提供していない標準Cライブラリのヘッダファイル。
- `test.sql`: SQLiteのテスト用データベースを作成するためのSQLファイル。

## 落とし穴

移植するにあたって、いくつかの落とし穴があります。

- `sqlite.c`を単にコンパイルすると大きすぎるため、利用しない部分を削ぎ落とす必要があります。これを勝手にやってくれるのが、リンク時最適化 (LTO) と呼ばれる技術です。本ブランチでは、`LTO=1`というオプションをMakefileに追加しています。
-　SQLiteは標準ライブラリのヘッダファイル (`stdio.h` など) を内部で参照しています。そのため、PDCLibやnewlibなどの標準Cライブラリを移植する必要があります。ただし、データベースファイルの読み書きなどのOSの機能は別途登録するため、実際に使われるのは標準Cライブラリのごくごく一部です。
- `servers/sqlite3/build.mk` に書いてある通り、いくつかSQLiteのビルドオプションを加える必要があります。
- SQLiteは浮動小数点演算を行いますが、ビルドの設定上コンパイラは「浮動小数点演算のソフトウェア実装がリンクされる」と仮定し、演算に対応する関数 (例: `__addsf3` 関数) 呼出しを出力します。そこで、その関数たちの実装である `libgcc.o` ([GCC low-level runtime library](https://gcc.gnu.org/onlinedocs/gccint/Libgcc.html)) というファイルを用意しています。

## 残っている課題

- 書き込み処理 (例: `INSERT`) は未実装です。
- `servers/sqlite3/main.c` では、実行ファイルに埋め込まれたデータベースファイルをメモリ上で読み書きしています。ここの部分をvirtio-blkドライバを呼び出すようにすると「SQLite3 as a ファイルシステム」が実現できます。
