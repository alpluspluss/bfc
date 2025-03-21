import argparse
from capstone import *

def disasm(file_path, arch, mode, base_address):
    with open(file_path, "rb") as f:
        binary_data = f.read()
        
    if arch == "arm64":
        md = Cs(CS_ARCH_ARM64, mode)
    elif arch == "x86":
        md = Cs(CS_ARCH_X86, mode)
    else:
        raise ValueError("Unsupported architecture. Use 'arm64' or 'x86'.")

    for insn in md.disasm(binary_data, base_address):
        print(f"0x{insn.address:x}:\t{insn.mnemonic}\t{insn.op_str}")

def main():
    parser = argparse.ArgumentParser(description="Disassemble a raw binary file")
    parser.add_argument("file", help="Path to the .bin file")
    parser.add_argument("-a", "--arch", choices=["arm64", "x86"], default="arm64", help="Architecture (default: arm64)")
    parser.add_argument("-m", "--mode", choices=["little", "big"], default="little", help="Endianness (default: little)")
    parser.add_argument("-b", "--base", type=int, default=0x1000, help="Base address for disassembly (default: 0x1000)")

    args = parser.parse_args()

    mode = CS_MODE_LITTLE_ENDIAN if args.mode == "little" else CS_MODE_BIG_ENDIAN
    disasm(args.file, args.arch, mode, args.base)

if __name__ == "__main__":
    main()
