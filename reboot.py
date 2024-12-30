#!/usr/bin/env python3

# Reboot to a specified address using /dev/xillybus_icap_in
# If it doesn't exist you aren't in the bootloader.

# After you run this program you need to rmmod xillybus_pcie
# to get the device to quiesce, at which point it reboots!

import argparse
from binascii import hexlify

def auto_int(x):
    return int(x,0)

parser = argparse.ArgumentParser(description='Reboot a Xillybus design')
parser.add_argument('address',type=auto_int,help='SPI address to reboot into')
args = vars(parser.parse_args())

print("address:", args['address'])
address = int(args['address'])

def revbyte(n):
    return int('{:08b}'.format(n)[::-1],2)

dev = open('/dev/xillybus_icap_in', 'wb')
# The data goes into xillybus_icap_in LSB FIRST
# In addition the ICAP ordering is STUPID:
# bit[15] is actually bit 8
# bit[7] is actually bit 0
# sooo dumb

# dummy word
din = bytearray(b'\xff\xff') # no need to swap or reverse, it already is
dev.write(din)

# sync word 0
# AA99 = 1010 1010 1001 1001 => reverse bit order IN EACH BYTE
# 5599   0101 0101 1001 1001 => write little endian
# 9955
din = bytearray(b'\xaa\x99'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# sync word 1
# 5566 = 0101 0101 0110 0110 => reverse bit order IN EACH BYTE
# AA66 = 1010 1010 0110 0110 => write little endian
# 66AA
din = bytearray(b'\x55\x66'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# type 1 write 1 word to GENERAL_1
# 3261 = 0011 0010 0110 0001 => reverse bit order IN EACH BYTE
# 4C86 = 0100 1100 1000 0110 => write little endian
# 864C
din = bytearray(b'\x32\x61'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# multiboot start address [15:0]
din = bytearray(2)
din[0] = revbyte((address & 0xFF))
din[1] = revbyte((address & 0xFF00)>>8)
dev.write(din)

# type 1 write 1 word to GENERAL2
din = bytearray(b'\x32\x81'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# opcode and multiboot start address[23:16]
din = bytearray(2)
din[0] = revbyte((address & 0xFF0000)>>16)
din[1] = revbyte(bytes(b'\x03')[0]) # read opcode
dev.write(din)

# type 1 write 1 word to GENERAL3
din = bytearray(b'\x32\xA1'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# fallback start address[15:0]
din = bytearray(b'\x00\x00') # no need to swap or reverse, it already is
dev.write(din)

# type 1 write 1 word to GENERAL4
din = bytearray(b'\x32\xC1'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# 0x3B00 = fallback start address[23:16] and opcode
din = bytearray(b'\x03\x00'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# type 1 write 1 word to CMD
din = bytearray(b'\x30\xA1'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# IPROG Command
din = bytearray(b'\x00\x0E'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

# Type 1 NO OP
din = bytearray(b'\x20\x00'[::-1])
din[0] = revbyte(din[0])
din[1] = revbyte(din[1])
dev.write(din)

dev.flush()
dev.close()
