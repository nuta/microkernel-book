import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-o",
        metavar="OUTFILE",
        dest="outfile",
        required=True,
        help="The output file path.",
    )
    parser.add_argument("dwarf_files", nargs="+")
    args = parser.parse_args()

    text = "# tools/generate_gdbinit.py によって生成されたGDBの初期化スクリプト\n"
    text += "# make gdb コマンドで利用される\n"
    text += "set confirm off\n"
    text += "set history save on\n"
    text += "set print pretty on\n"
    text += "set disassemble-next-line auto\n"
    text += "set architecture riscv:rv32\n"
    text += "set riscv use-compressed-breakpoints yes\n"
    for dwarf_file in args.dwarf_files:
        text += f"add-symbol-file {dwarf_file}\n"
    text += "target remote 127.0.0.1:7777\n"

    with open(args.outfile, "w", encoding="utf-8") as f:
        f.write(text)

if __name__ == "__main__":
    main()
