# 『自作OSで学ぶマイクロカーネルの設計と実装』サポートサイト

このリポジトリは、[『自作OSで学ぶマイクロカーネルの設計と実装』(秀和システム)](https://www.hanmoto.com/bd/isbn/9784798068718) のサポートサイト兼、教育用マイクロカーネルOS「HinaOS」のソースコードの配布場所です。

- [質問・バグ報告など (GitHub Discussions)](https://github.com/nuta/microkernel-book/discussions)
- [HinaOS: ビルド・実行方法](#開発環境構築)
- [HinaOS: 内部構造の概要](ARCHITECTURE.md)
- [HinaOS: 実験テーマのアイデア集](IDEAS.md)
- [HinaOS: デバッグガイド](DEBUG.md)

<a href="https://www.hanmoto.com/bd/isbn/9784798068718">
<img alt="本のカバー画像" src="https://gist.githubusercontent.com/nuta/e45864405fbdc8618af4b08de534e42c/raw/bd3df82e7039902818c8fc0d394b69250cc78fc9/cover.jpg" width="300">
</a>

## HinaOSとは

HinaOS (ひなおーえす) はマイクロカーネルベースの教育用オペレーティングシステムです。次の特徴・機能を持っています。

- 32ビット RISC-V ([QEMU virtマシン](https://www.qemu.org/docs/master/system/riscv/virt.html)) に対応
- マルチプロセッサ対応マイクロカーネル
- 独自ファイルシステム (HinaFS)
- TCP/IP プロトコルスタック
- virtio-blk デバイスドライバ (仮想ストレージデバイス)
- virtio-net デバイスドライバ (仮想ネットワークデバイス)
- コマンドラインシェル

## 開発環境構築

Ubuntu 22.04 (Jammy) の場合は次のコマンドで必要なパッケージをインストールできます。

```
$ sudo apt update
$ sudo apt install llvm clang lld python3-pip qemu-system gdb-multiarch git
$ pip3 install --user -r tools/requirements.txt
```

macOS 13 (Ventura) の場合は次のコマンドで必要なパッケージをインストールできます。

```
$ brew install llvm python3 qemu riscv-software-src/riscv/riscv-tools
$ pip3 install --user -r tools/requirements.txt
```

> **Note**
>
> Windowsでもwingetを利用して上記のパッケージをインストールすることでネイティブな開発環境を構築できます。ただし、環境構築がかえって面倒であるため、[WSL2](https://learn.microsoft.com/ja-jp/windows/wsl/install)上でUbuntuを利用するのがお勧めです。

## ソースコードのダウンロード

`git`コマンドでソースコードをダウンロードできます。

```
$ git clone https://github.com/nuta/microkernel-book
```

## ビルド・実行

次のコマンドでビルドできます。`-j8` は8並列ビルドを意味します。CPUコア数程度を指定すると良いでしょう。`V=1`や`RELEASE=1`はビルドオプションで、組み合わせて使うことができます。

```
$ make -j8              # OSをビルドする
$ make -j8 V=1          # OSをビルドする (詳細なビルドログを出力)
$ make -j8 RELEASE=1    # OSをビルドする (コンパイラのより高度な最適化を有効化)
$ make -j8 run          # OSをビルドしてQEMUで実行する (シングルコア)
$ make -j8 run CPUS=4   # OSをビルドしてQEMUで実行する (4コア)
```

## デバッグ

[HinaOSのデバッグ](./DEBUG.md) を参照してください。

## ライセンス

HinaOSは [MITライセンス](./LICENSE.md) で公開されています。

## プルリクエスト受諾方針

HinaOSは教育目的のOSということで、シンプルさと分かりやすさを保つために新機能を追加するプルリクエストは受け付けていません。誤字の修正やバグフィックスのような変更のみ受け付けています。ただし、各ドキュメント (`ARCHITECTURE.md` など) の改善は大歓迎です :D
