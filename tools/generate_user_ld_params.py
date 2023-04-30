import argparse

# user.ldのベース仮想アドレス。ここからユーザープログラムのメモリ領域がマップされる。
#
# 512 MiB目から利用するのは、NULLポインタ参照をきちんと検出できるようにするためなのと、
# MMIO領域 (UART, virtio, PLIC, ACLINT) の物理アドレスがそのまま同じ仮想アドレスに
# 対応するようにカーネルがマップしているため。
USER_BASE_ADDR = 0x20000000

# ユーザープログラムのメモリ領域 (.text, .data, .rodata, .bss, スタック, ヒープ) のサイズ。
USER_SIZE = 32 * 1024 * 1024 # 32 MiB

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-o",
        metavar="OUTFILE",
        dest="out_file",
        required=True,
        help="The output file path.",
    )
    parser.add_argument("servers", nargs="+", help="The list of servers.")
    args = parser.parse_args()

    text = "// generate_user_ld_params.py によって生成されたファイル\n"
    text += "#pragma once\n"
    text += "\n"

    base_addr = USER_BASE_ADDR
    for server in sorted(args.servers):
        text += f"#ifdef SERVER_{server.lower()}\n"
        text += f"#define USER_BASE_ADDR {hex(base_addr)}\n"
        text += f"#define USER_SIZE      {hex(USER_SIZE)}\n"
        text += "#endif\n\n"
        base_addr += USER_SIZE

    with open(args.out_file, "w", encoding="utf-8") as f:
        f.write(text)


if __name__ == "__main__":
    main()
