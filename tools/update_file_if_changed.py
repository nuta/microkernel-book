"""

GNU makeではファイルの内容が変わっていなくても更新日時が変わってしまうと、
makeが再ビルドが必要と判断してしまう。

このスクリプトを使うことで、ファイルの内容が変わっていない場合には、
更新日時を変更しないようにすることができる。

"""

import argparse
import os

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("outfile")
    parser.add_argument("text")
    args = parser.parse_args()

    try:
        with open(args.outfile, "r") as f:
            if f.read() == args.text:
                return
    except IOError:
        pass

    if os.path.exists(args.outfile):
       print("update detected in build settings; rebuilding from scratch...")
    with open(args.outfile, "w") as f:
        f.write(args.text)

if __name__ == "__main__":
    main()
