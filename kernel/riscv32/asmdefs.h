// アセンブリファイルで使用する定義
//
// #defineや#ifdefといったプリプロセッサの命令 (#から始まるもの) しか使えないので注意。
// 構造体宣言のようなC言語特有のものはコンパイルエラーになる。
#pragma once

// struct arch_cpuvarのメンバ変数のオフセット
#define CPUVAR_SSCRATCH  0
#define CPUVAR_SP_TOP    4
#define CPUVAR_MSCRATCH0 8
#define CPUVAR_MSCRATCH1 12
#define CPUVAR_MTIMECMP  16
#define CPUVAR_MTIME     20
#define CPUVAR_INTERVAL  24
