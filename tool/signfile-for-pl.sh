#!/bin/bash

#
# Copyright (C) 2015 MediaTek Inc. All rights reserved.
# Tristan Shieh <tristan.shieh@mediatek.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

if [ $# -ne 2 ]
then
       echo "Usage: $0 private-key.pem filename"
       exit -1
fi

TMPFILE1=$(mktemp)
TMPFILE2=$(mktemp)

python -c "
import hashlib
import struct
import os

file_len = os.path.getsize('$2')
b = struct.pack('16I', \
         0x53535353, 0x54535543, 0x00000000, 0x00000000, \
         0x00000000, 0x00000000, 0x00000000, 0x00000000, \
         0x00000000, 0x00000001, file_len, 0x00000040, \
         0x00000000, file_len, file_len+0x40, 0x00000094, )

f = open('$2', 'rb')
b += f.read()
f.close()

d = hashlib.sha1(b).digest()

f = open('${TMPFILE1}', 'wb')
f.write(d)
f.close()
"

openssl rsautl -sign -inkey $1 -in ${TMPFILE1} -out ${TMPFILE2}
RET=$?

python -c "
import struct
import os

file_len = os.path.getsize('$2')
b = struct.pack('16I', \
         0x53535353, 0x54535543, 0x00000000, 0x00000000, \
         0x00000000, 0x00000000, 0x00000000, 0x00000000, \
         0x00000000, 0x00000001, file_len, 0x00000040, \
         0x00000000, file_len, file_len+0x40, 0x00000094, )

f = open('${TMPFILE2}', 'rb')
b += f.read()
f.close()

f = open('${TMPFILE1}', 'rb')
b += f.read(20)
f.close()

b += '\0' * 44

f = open('$2.sign', 'wb')
f.write(b)
f.close()
"

rm -f ${TMPFILE1} ${TMPFILE2}
exit ${RET}
