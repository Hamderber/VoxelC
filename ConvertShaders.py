#!/usr/bin/env python3
import sys, os, re

def to_camel_case(name: str) -> str:
    parts = re.split(r'[\W_]+', name)
    return parts[0].lower() + ''.join(word.capitalize() for word in parts[1:] if word)

def main():
    if len(sys.argv) < 2:
        print("Usage: ./ConvertShaders.py shader.spv")
        sys.exit(1)

    infile = sys.argv[1]
    if not os.path.isfile(infile):
        print(f"Error: file '{infile}' not found.")
        sys.exit(1)

    base = os.path.splitext(os.path.basename(infile))[0]
    varname = to_camel_case(base)
    outfile = os.path.splitext(infile)[0] + ".h"

    with open(infile, "r") as f:
        contents = f.read().strip()

    # Remove outer braces if present
    if contents.startswith("{") and contents.endswith("}"):
        contents = contents[1:-1].strip()

    # Write so that the original is overwritten on recompile
    with open(outfile, "w", newline="\n") as f:
        f.write(f"// Auto-generated from {os.path.basename(infile)}\n")
        f.write("#pragma once\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"const uint32_t {varname}Code[] = {{\n{contents}\n}};\n")
        f.write(f"const size_t {varname}CodeSize = sizeof({varname}Code);\n")

    print(f"Generated: {outfile}")

if __name__ == "__main__":
    main()
