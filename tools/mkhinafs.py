"""

HinaFSイメージを生成するスクリプト

"""

import argparse
from pathlib import Path
import struct

DISK_SIZE = 128 * 1024 * 1024
BLOCK_SIZE = 4096
NUM_BITMAP_BLOCKS = 4
NUM_HEADER_BLOCKS = 2 + NUM_BITMAP_BLOCKS
BLOCKS_PER_ENTRY = 1908
FS_TYPE_DIR = 0xdd
FS_TYPE_FILE = 0xff
FS_NAME_LEN = 256

def encode_path_name(name: str) -> str:
    assert len(name) < FS_NAME_LEN - 1

    try:
        return name.encode("ascii")
    except UnicodeEncodeError:
        raise Exception(f"file name must be an ASCII string: {name}")

def main():
    parser = argparse.ArgumentParser(description="Generates a bootfs.")
    parser.add_argument("image_file", help="The image file.")
    parser.add_argument("root_dir", help="The root directory to be embedded into the image.")
    args = parser.parse_args()

    blocks = []
    def add_file(path: Path) -> int:
        data = open(path, "rb").read()
        entry_block = struct.pack("B3x255sxIqq", FS_TYPE_FILE, encode_path_name(path.name), len(data), 0, 0)
        for _ in range(0, BLOCKS_PER_ENTRY):
            if len(data) == 0:
                data_block_index = 0xffff
            else:
                data_block_index = len(blocks) + NUM_HEADER_BLOCKS
                chunk = data[:BLOCK_SIZE]
                blocks.append(chunk + b"\x00" * (BLOCK_SIZE - len(chunk)))
                data = data[BLOCK_SIZE:]
            entry_block += struct.pack("H", data_block_index)

        entry_block_index = len(blocks) + NUM_HEADER_BLOCKS

        assert len(entry_block) == BLOCK_SIZE
        blocks.append(entry_block)
        return entry_block_index

    def add_dir(path: Path, root_dir: bool) -> int:
        if root_dir:
            name = "/"
        else:
            name = path.name

        entries = []
        for child_path in path.iterdir():
            if child_path.is_dir():
                entries.append(add_dir(child_path, root_dir=False)[1])
            if child_path.is_file():
                entries.append(add_file(child_path))
            else:
                raise Exception(f"unexpected file type: {child_path}")

        assert len(entries) < BLOCKS_PER_ENTRY # too many files in a directory

        dir_block = struct.pack("B3x255sxH2xqq", FS_TYPE_DIR, encode_path_name(name), len(entries), 0, 0)
        for i in range(0, BLOCKS_PER_ENTRY):
            if i < len(entries):
                entry_block_index = entries[i]
            else:
                entry_block_index = 0xffff
            dir_block += struct.pack("H", entry_block_index)

        assert len(dir_block) == BLOCK_SIZE
        if root_dir:
            dir_block_index = 1
        else:
            dir_block_index = len(blocks) + NUM_HEADER_BLOCKS
            blocks.append(dir_block)

        return (dir_block_index, dir_block)

    _, root_dir_block = add_dir(Path(args.root_dir), root_dir=True)

    # struct hinafs_header {
    #     uint32_t magic;
    #     uint32_t num_data_blocks;
    #     uint8_t padding[4088];
    # };
    fs_header = struct.pack("II4088x", 0xf2005346, 0)

    bitmap_blocks = [0x00] * NUM_BITMAP_BLOCKS * BLOCK_SIZE
    for i in range(0, len(blocks)):
        bitmap_blocks[i // 8] |= 1 << (i % 8)

    # struct hinafs {
    #     struct hinafs_header header;
    #     uint8_t bitmap_blocks[num_bitmap_blocks][BLOCK_SIZE];
    #     uint8_t blocks[num_data_blocks][BLOCK_SIZE];
    # }
    bitmap_blocks_bytes = bytes(bitmap_blocks)
    data_blocks_bytes = b"".join(blocks)

    assert len(blocks)
    assert len(fs_header) == BLOCK_SIZE
    assert len(bitmap_blocks_bytes) == NUM_BITMAP_BLOCKS * BLOCK_SIZE
    assert len(data_blocks_bytes) == len(blocks) * BLOCK_SIZE

    image = fs_header + root_dir_block + bitmap_blocks_bytes + data_blocks_bytes

    free_space_len = DISK_SIZE - len(image)
    zeroed_4096_bytes = b"\x00" * 4096
    with open(args.image_file, "wb") as f:
        f.write(image)
        while True:
            if free_space_len < 4096:
                f.write(b"\x00" * free_space_len)
                break
            f.write(zeroed_4096_bytes)
            free_space_len -= 4096

if __name__ == "__main__":
    main()
