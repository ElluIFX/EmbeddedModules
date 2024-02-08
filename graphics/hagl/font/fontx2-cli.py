import argparse

from fontx2 import FontX2


def main():
    parser = argparse.ArgumentParser(description="Convert FontX2")

    parser.add_argument("--from_binary", help="FontX2 filename (binary version).")
    parser.add_argument("--from_json", help="FontX2 filename (json version).")

    parser.add_argument("--new", help="FontX2 new json filename.")
    parser.add_argument(
        "--start_char",
        help="ONLY USED FOR NEW FONT. Starting char. (default: 32)",
        type=int,
        default=32,
    )
    parser.add_argument(
        "--stop_char",
        help="ONLY USED FOR NEW FONT. Stopping char. (default: 127)",
        type=int,
        default=127,
    )
    parser.add_argument(
        "--width",
        help="ONLY USED FOR NEW FONT. Font width. (default: 16)",
        type=int,
        default=16,
    )
    parser.add_argument(
        "--height",
        help="ONLY USED FOR NEW FONT. Font height. (default: 16)",
        type=int,
        default=16,
    )

    parser.add_argument("--crop_sx", help="Crop sx", type=int, default=0)
    parser.add_argument("--crop_dx", help="Crop dx", type=int, default=0)
    parser.add_argument("--crop_top", help="Crop top", type=int, default=0)
    parser.add_argument("--crop_bottom", help="Crop bottom", type=int, default=0)

    parser.add_argument("--to_binary", help="FontX2 filename (binary version).")
    parser.add_argument("--to_json", help="FontX2 filename (json version).")
    parser.add_argument("--to_jpg", help="Dump font in jpeg format.")

    args = parser.parse_args()

    font_in = FontX2()
    font_out = FontX2()

    if args.from_binary != None:
        font_in.from_binary(args.from_binary)
    elif args.from_json != None:
        json_file = open(args.from_json, "r+")
        font_in.from_json(json_file.read())
        json_file.close()
    elif args.new != None:
        # todo check args
        font_in.new(args.start_char, args.stop_char, args.width, args.height)
    else:
        print("Missing import (from_binary/from_json) or new argument.")
        exit(1)
    print(
        f"Imported {font_in.chars_number} chars with size {font_in.width}x{font_in.height}."
    )

    font_in.crop(font_out, args.crop_sx, args.crop_dx, args.crop_top, args.crop_bottom)

    if args.to_binary != None:
        font_out.to_binary(args.to_binary)
    if args.to_json != None:
        json_file = open(args.to_json, "w")
        json_file.write(font_out.to_json())
        json_file.close()
    if args.to_jpg != None:
        font_out.to_picture(args.to_jpg)


if __name__ == "__main__":
    main()
