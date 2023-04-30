// シンボルテーブルを埋め込むためのメモリ領域
// tools/embed_symbols.py を使ってビルド時に上書きされる
.section .symbols

.align 4
.global __symbol_table, __symbol_table_end
__symbol_table:
.ascii "__SYMBOL_TABLE_START__"   // シンボルテーブルの開始を示すマーカー
.space 256 * 1024                 // 256KBあれば十分なはず
.ascii "__SYMBOL_TABLE_END__"     // シンボルテーブルの終了を示すマーカー
__symbol_table_end:
