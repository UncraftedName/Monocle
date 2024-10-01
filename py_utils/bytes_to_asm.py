import iced_x86
import pyperclip


byte_hex_str = input("enter byte string w/ spaces: ")
inp_bytes = bytearray.fromhex(byte_hex_str)

decoder = iced_x86.Decoder(32, inp_bytes)
formatter = iced_x86.Formatter(iced_x86.FormatterSyntax.MASM)
formatter.uppercase_mnemonics = True
formatter.uppercase_registers = True
formatter.space_between_memory_add_operators = True
formatter.space_after_operand_separator = True

cb = ""
for instr in decoder:
    f_str = formatter.format(instr)
    print(f_str)
    cb += "    " + f_str + "\n"

pyperclip.copy(cb)

print("\nCopied to clipboard!")
