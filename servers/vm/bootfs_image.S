// BootFSを埋め込むためのファイル
.section .rodata
.global __bootfs
__bootfs:
.incbin BOOTFS_PATH
