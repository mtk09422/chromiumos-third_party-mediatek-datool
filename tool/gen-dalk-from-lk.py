#
# Copyright (c) 2014 MediaTek Inc.
# Author: Tristan Shieh <tristan.shieh@mediatek.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

#!/usr/bin/env python

import os
import struct
import sys

def read(path):
        with open(path, "rb") as f:
                return f.read()

def write(path, data):
        with open(path, "wb") as f:
                f.write(data)

def padding(data, size, pattern = '\0'):
        return data + pattern * (size - len(data))

LK_MEM_ADDRs = {'8135': 0x81e00000,
                '6595': 0x41e00000,
                '8173': 0x41e00000}

BOOTARG_OFFSET = 0x80

boot_args = { 
        '8135': struct.pack("26I",
        0x504c504c, 0x00000063, 0x00000000, 0x11009000,
        0x000e1000, 0x00500a01, 0x00000001, 0x34000000,
        0x10240d40, 0x02101010, 0x000a8200, 0x00000000,
        0x00000000, 0x00000000, 0x00000231, 0x00000000,
        0x00000000, 0x00000000, 0x822041c1, 0x51200655,
        0x92124805, 0x18420000, 0x3a00a284, 0xc0444890,
        0x1980a991, 0x04000099), 

        '6595': struct.pack("26I",
        0x504c504c, 0x00000063, 0x00000000, 0x11002000,
        0x000e1000, 0xEBFE0101, 0x00000001, 0x80000000,
        0x00000000, 0xE59304C0, 0xE3500000, 0x00000000,
        0x00000000, 0x00000000, 0x00000231, 0x00000000,
        0x00000000, 0x00000000, 0x822041c1, 0x51200655,
        0x92124805, 0x18420000, 0x40079a84, 0xE1A09000,
        0x00000000, 0x00000000),

        '8173': struct.pack("44I",
        0x504C504C, 0x00000063, 0x00000000, 0x11002000,
        0x000E1000, 0x00000301, 0x00000001, 0x7F800000,
        0x00000000, 0x00000000, 0x00000000, 0x00000001,
        0x00000000, 0x00000000, 0x000014E7, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x40079A84, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x001997C0, 0x00000000,
        0x00000007, 0x00000005, 0x0012C000, 0x00004000,
        0xBF800000, 0x00000000, 0x00800000, 0x00000000)}


def main(argv):
        lk = read(argv[2])

        boot_arg = boot_args[argv[1]]
        LK_MEM_ADDR = LK_MEM_ADDRs[argv[1]]

        lk_wrapper = struct.pack("21I",
					#LK_WRAPPER:
         0xe1a0000f, 			#   0:	mov     r0, pc
         0xe2400008, 			#   4:	sub     r0, r0, #8
         0xe59f1030, 			#   8:	ldr     r1, [pc, #48]   ; 40 <COPY+0x1c>
         0xe0800001, 			#   c:	add     r0, r0, r1
         0xe59f102c, 			#  10:	ldr     r1, [pc, #44]   ; 44 <COPY+0x20>
         0xe59f202c, 			#  14:	ldr     r2, [pc, #44]   ; 48 <COPY+0x24>
         0xe0812002, 			#  18:	add     r2, r1, r2
         0xe59f3028, 			#  1c:	ldr     r3, [pc, #40]   ; 4c <COPY+0x28>
         0xe0822003, 			#  20:	add     r2, r2, r3
					#COPY:
         0xe1510002, 			#  24:	cmp     r1, r2
         0x34903004, 			#  28:	ldrcc   r3, [r0], #4
         0x34813004, 			#  2c:	strcc   r3, [r1], #4
         0x3afffffb, 			#  30:	bcc     24 <COPY>
         0xe59f4008, 			#  34:	ldr     r4, [pc, #8]    ; 44 <COPY+0x20>
         0xe59f5008, 			#  38:	ldr     r5, [pc, #8]    ; 48 <COPY+0x24>
         0xe59ff00c, 			#  3c:	ldr     pc, [pc, #12]   ; 50 <COPY+0x2c>
         BOOTARG_OFFSET,		#  40:	BOOTARG_OFFSET  .word   0x11111111
         LK_MEM_ADDR - len(boot_arg),	#  44:	BOOTARG_ADDR    .word   0x22222222
         len(boot_arg),			#  48:	BOOTARG_SIZE    .word   0x33333333
         len(lk),			#  4c:	LK_SIZE         .word   0x44444444
         LK_MEM_ADDR			#  50:	LK_ADDR         .word   0x55555555
        )

        o = padding(lk_wrapper, BOOTARG_OFFSET, '\0') + boot_arg + lk
        write(argv[3], o)

if __name__ == "__main__":
        main(sys.argv)


