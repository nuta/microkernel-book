import argparse
import os

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--kernel-elf", required=True)
    parser.add_argument("--bootfs-bin", required=True)
    parser.add_argument("--hinafs-img", required=True)
    args = parser.parse_args()

    print()
    print(f"Kernel Executable: {args.kernel_elf} ({os.stat(args.kernel_elf).st_size // 1024} KiB)")
    print(f"BootFS Image:      {args.bootfs_bin} ({os.stat(args.bootfs_bin).st_size // 1024} KiB)")
    print(f"HinaFS Image:      {args.hinafs_img} ({os.stat(args.hinafs_img).st_size // 1024} KiB)")

if __name__ == "__main__":
    main()
