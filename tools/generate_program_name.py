import argparse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-o",
        metavar="OUTFILE",
        dest="out_file",
        required=True,
        help="The output file path.",
    )
    parser.add_argument("name")
    args = parser.parse_args()

    text = 'const char *__program_name(void) { return "' + args.name + '"; }'
    with open(args.out_file, "w") as f:
        f.write(text)


if __name__ == "__main__":
    main()
