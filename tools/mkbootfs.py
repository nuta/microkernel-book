"""

BootFSイメージを生成するスクリプト

"""

import argparse
from glob import glob
from pathlib import Path
import struct
import sys

VERSION = 1
PAGE_SIZE = 4096
FILE_ENTRY_SIZE = 64
BOOTFS_MAX_SIZE = 32 * 1024 * 1024


def align_up(value, align):
    return (value + align - 1) & ~(align - 1)


def main():
    parser = argparse.ArgumentParser(description="Generates a bootfs.")
    parser.add_argument("-o", dest="output", help="The output file.")
    parser.add_argument("dir", help="The root directory.")
    args = parser.parse_args()

    # Write the boot binary and the file system header.
    files = glob(args.dir + "/*")
    image = struct.pack("HHHH", VERSION, 8, len(files), 0)

    # Append files.
    file_contents = bytes()
    file_off = align_up(len(image) + FILE_ENTRY_SIZE * len(files), PAGE_SIZE)
    for path in files:
        name = str(Path(path).stem)
        if len(name) >= 56:
            sys.exit(f"too long file name: {name}")

        data = open(path, "rb").read()
        file_contents += data
        image += struct.pack("56sII", name.encode("ascii"), file_off, len(data))

        padding = align_up(len(file_contents), PAGE_SIZE) - len(file_contents)
        file_contents += struct.pack(f"{padding}x")
        file_off += len(data) + padding

    padding = align_up(len(image), PAGE_SIZE) - len(image)
    image += struct.pack(f"{padding}x")
    image += file_contents

    if len(image) > BOOTFS_MAX_SIZE:
        sys.exit(f"bootfs.bin is too big ({len(image) / 1024}KB)")

    with open(args.output, "wb") as f:
        f.write(image)


if __name__ == "__main__":
    main()
