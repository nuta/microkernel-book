"""

UNIX系OSの基本的なコマンドを実装したスクリプト (Windows用)

Windows上では、GNU makeはUNIX系のシェルではなくコマンドプロンプト (cmd.exe) 上でコマンドを実行するため、
ところどころ互換性がない部分がある。特に、パスの区切りがスラッシュ (macOS/Linux) かバックスラッシュ (Windows) か
の違いをMakefile側で吸収するのが難しい。そこでPythonに任せる。

"""
import argparse
import fnmatch
import os
import shutil
import sys

def do_cp(args):
    for src in args.src:
        shutil.copy(src, args.dst)

def do_rm(args):
    if args.r:
        shutil.rmtree(args.path, ignore_errors=args.f)
    else:
        try:
            os.unlink(args.path)
        except Exception as e:
            if not args.f:
                sys.exit(f"rm: failed to remove '{args.path}': {e}")

def do_mkdir(args):
    if args.p:
        os.makedirs(args.path, exist_ok=True)
    else:
        os.mkdir(args.path)

def do_find(args):
    name = None
    for k, v in zip(args.exprs[::2], args.exprs[1::2]):
        if k not in ["-name"]:
            sys.exit(f"find: unknown expression {k}")
        if v is None:
            sys.exit(f"find: the value for {k} is missing")
        if k == "-name":
            name = v

    for root, dirs, files in os.walk(args.path):
        for file in files:
            if name:
                show = fnmatch.fnmatch(file, name)
            else:
                show = True
            if show:
                print(os.path.join(root, file))

def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    mkdir_command = subparsers.add_parser("cp")
    mkdir_command.add_argument("src", nargs="+")
    mkdir_command.add_argument("dst")
    mkdir_command.set_defaults(handler=do_cp)

    rm_command = subparsers.add_parser("rm")
    rm_command.add_argument("-r", action="store_true")
    rm_command.add_argument("-f", action="store_true")
    rm_command.add_argument("path")
    rm_command.set_defaults(handler=do_rm)

    mkdir_command = subparsers.add_parser("mkdir")
    mkdir_command.add_argument("-p", action="store_true")
    mkdir_command.add_argument("path")
    mkdir_command.set_defaults(handler=do_mkdir)

    find_command = subparsers.add_parser("find")
    find_command.add_argument("path")
    find_command.add_argument("exprs", nargs=argparse.REMAINDER)
    find_command.set_defaults(handler=do_find)

    args = parser.parse_args()
    if hasattr(args, "handler"):
        args.handler(args)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
