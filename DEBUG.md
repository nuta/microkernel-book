# HinaOSのデバッグ

## printfデバッグ

一番手っ取り早いのが変数の値をprintfで出力する方法です。HinaOSではprintfを直接呼び出す代わりに次のようなマクロを利用します。各マクロは引数にprintfのフォーマット文字列とその引数を取ります。

```c
#define TRACE(fmt, ...) // トレースメッセージ
#define DBG(fmt, ...)   // デバッグメッセージ
#define INFO(fmt, ...)  // 普通のメッセージ
#define WARN(fmt, ...)  // 警告メッセージ
#define ERROR(fmt, ...) // エラーメッセージ
#define OOPS(fmt, ...)  // 警告メッセージ + スタックトレース
#define PANIC(fmt, ...) // エラーメッセージ + スタックトレース + 強制終了
```

### 利用例

```c
#include <libs/common/print.h>

int i = 123;
char *s = "abc";
INFO("hello world: %d, %s", i, s); // hello world: 123, abc
```

## タスクダンプ

<kbd>Ctrl</kbd>+<kbd>P</kbd>を押下すると、HinaOSカーネルが各タスクの状態を次のように表示します。処理がどこかで止まっている場合は、この結果を元に原因を特定することができます。メッセージパッシングが同期的な処理であるため、とりわけ「各タスクがどのタスクに対してメッセージの送信待ち・受信待ちしているのか」が重要です。

```
[kernel] WARN: active tasks:
[kernel] WARN:   #1: vm: BLOCKED (open receive)
[kernel] WARN:   #2: virtio_blk: BLOCKED (open receive)
[kernel] WARN:   #3: tcpip: BLOCKED (open receive)
[kernel] WARN:   #4: virtio_net: BLOCKED (open receive)
[kernel] WARN:   #5: shell: BLOCKED (send, serial_read, or exited)
[kernel] WARN:   #6: fs: BLOCKED (open receive)
```

## デバッガ (GDB) を利用したデバッグ

QEMUにはGDBを利用したデバッグ機能が組み込まれています。この機能を活用することで、HinaOSが具体的にどのような処理を行っているのかを追うことができます。

GDBを利用するデバッグには (1) QEMUを実行するターミナルと (2) GDBを実行するターミナルの2つが必要です。QEMUを実行するターミナルでは、次のように `GDBSERVER=1` を指定して起動します。

```
make run GDBSERVER=1
```

QEMUはGDBが接続されるまでHinaOSを起動しません。GDBを実行するターミナルでは、次のように `make gdb` を実行します。OSごとにGDBのパスが異なるため、OSに合わせて `GDB` 変数を指定してください。なお、Homebrewでインストールした場合は指定する必要はありません。

```
# macOS Homebrew (riscv-software-src/riscv/riscv-tools パッケージ)
make gdb

# Ubuntu (gdb-multiarch パッケージ)
make gdb GDB=gdb-multiarch

# Windows MSYS2 (mingw-w64-clang-x86_64-gdb-multiarch パッケージ)
make gdb GDB="c:\msys64\clang64\bin\gdb-multiarch.exe"
```

GDBが起動すると、次のようなメッセージが表示されます。

```
0x00001000 in ?? ()
=> 0x00001000:  97 02 00 00     auipc   t0,0x0
(gdb)
```

`(gdb)` が表示されたら、GDBのコマンドを入力できます。GDBのコマンドには次の様なものがあります。

- `q`: プログラムを終了する。
- `c`: プログラムを続行する。
- `s`: ステップ実行をする。
- `p 変数名`: 変数の値を表示する。
- `b シンボル名`: ブレークポイントを設定する。シンボル名は下記の注意事項を参照。
- `bt`: スタックトレースを表示する。
- `info registers`: レジスタの値を表示する。

> **Note**
>
> HinaOSではカーネルとユーザプログラムの両方をデバッグすることができます。ただしシンボル名の重複を避けるために、各シンボル名には`kernel.`または`<サーバ名>.`の接頭辞が付いています。例えば、`kernel.main`はカーネルの`main`関数を表します。

詳しくはGDBのマニュアルを参照してください。基本的には次の様な流れで利用します。

1. `b`コマンドで確認したい関数の先頭にブレークポイントを設定する。
2. `c`コマンドでHinaOSを起動する。
3. ブレークポイントで停止すると、GDBのプロンプトが表示される。
4. `bt`コマンドでスタックトレースを表示したり、`s`コマンドでステップ実行していくことで動作の流れを確認したり、`p` コマンドで変数の値を確認したりする。
5. `q`コマンドでGDBを終了する。

以下に、実際にGDBを利用したデバッグの例を示します。この例ではVMサーバの`bootfs_read`関数にブレークポイントを設定し、それがどこから呼ばれるのか (スタックトレース) 確認しています。

```
0x00001000 in ?? ()
=> 0x00001000:  97 02 00 00     auipc   t0,0x0
(gdb) b vm.bootfs_read
Breakpoint 1 at 0x2e001fe0: file servers/vm/bootfs.c, line 11.
(gdb) c
Continuing.

Breakpoint 1, bootfs_read (file=0x2e00d7d4, off=0, buf=0x2e6a3060, len=4096) at servers/vm/bootfs.c:11
11          void *p = (void *) (((uaddr_t) __bootfs) + file->offset + off);
(gdb) bt
#0  bootfs_read (file=0x2e00d7d4, off=0, buf=0x2e6a3060, len=4096)
    at servers/vm/bootfs.c:11
#1  0x2e000cb6 in task_spawn (file=0x2e00d7d4) at servers/vm/task.c:34
#2  0x2e00075a in spawn_servers () at servers/vm/main.c:26
#3  main () at servers/vm/main.c:50
#4  0x2e008692 in vm[] () at libs/user/riscv32/start.S:10
Backtrace stopped: frame did not save the PC
(gdb)
```

### QEMUにprintfデバッグを追加する

HinaOSではなくQEMU自体にprintfを入れたり、QEMU自体にデバッガをアタッチして挙動を観察するのも有効なデバッグ手段です。例えば、virtioデバイスがうまく動いていない場合、virtioデバイスのエミュレーション部分にprintfをたくさん入れて、どこまで処理が進んでいるのか、どこで中断されているのかを確認することができます。

なお、QEMUにはトレース機能 ([ドキュメント](https://qemu-project.gitlab.io/qemu/devel/tracing.html)) が備わっていますが、自身で知りたい情報をprintfで出力する方が正直楽です。
