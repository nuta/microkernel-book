// 最初のユーザータスクのELFイメージを、カーネルに埋め込むためのファイル。
// .boot_elfセクションに埋め込まれ、リンカースクリプトでどこに配置するかを指定している。
.section .boot_elf
.incbin BOOT_ELF_PATH
