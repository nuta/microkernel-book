"""

シンボルテーブルを実行ファイルに埋め込むスクリプト

"""

import argparse
import struct
import sys

SYMBOL_TABLE_MAGIC = bytes([0x53, 0x59, 0x4d, 0x4c])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("symbolsfile")
    parser.add_argument("infile")
    parser.add_argument("outfile")
    args = parser.parse_args()

    symbols = {}
    for line in open(args.symbolsfile, "r").readlines():
        cols = line.rstrip().split(" ", 2)
        try:
            addr = int(cols[0], 16)
        except ValueError:
            continue

        name = cols[2]
        if name.startswith("."):
            continue

        if name == "__symbol_table":
            start_off = addr
        elif name == "__symbol_table_end":
            end_off = addr
        else:
            type_= cols[1]
            if type_ not in ["T", "t"]:
                continue

            symbols[addr] = name

    image = open(args.infile, "rb").read()
    start_off = image.find(b"__SYMBOL_TABLE_START__")
    end_off = image.find(b"__SYMBOL_TABLE_END__") + len(b"__SYMBOL_TABLE_END__")
    if start_off is None:
        sys.exit(f"{args.infile}: BUG: failed to locate the symbol table")
    if end_off is None:
        sys.exit(f"{args.infile}: BUG: failed to locate the symbol table")
    if image.count(b"__SYMBOL_TABLE_START__") != 1:
        sys.exit(f"{args.infile}: BUG: multiple symbol tables")
    if image.count(b"__SYMBOL_TABLE_END__") != 1:
        sys.exit(f"{args.infile}: BUG: multiple symbol tables")

    symbols = sorted(symbols.items(), key=lambda s: s[0])

    # Build a symbol table.
    symbol_table = struct.pack("<4sI8x", SYMBOL_TABLE_MAGIC, len(symbols))
    for addr, name in symbols:
        symbol_table += struct.pack("<I60s", addr, bytes(name[:55], "ascii"))

    max_size = end_off - start_off
    if len(symbol_table) > max_size:
        sys.exit(f"{args.infile}: symbol table too big")

    symbol_table += struct.pack(f"{max_size - len(symbol_table)}x")

    # Embed the symbol table.
    new_image = image[:start_off] + symbol_table + image[start_off + len(symbol_table):]
    assert len(new_image) == len(image)
    open(args.outfile, "wb").write(new_image)


if __name__ == "__main__":
    main()
