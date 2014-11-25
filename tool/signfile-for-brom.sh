#!/bin/bash
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

if [ $# -ne 2 ]
then
       echo "Usage: $0 private-key.pem filename"
       exit -1
fi

TMPFILE1=$(mktemp)
TMPFILE2=$(mktemp)

python -c "
import hashlib

f = open('$2', 'rb')
b = f.read()
f.close()

d = hashlib.sha256(b).digest()
b = '\0\0'
for i in range(0, len(d), 2):
       b += d[i + 1] + d[i]
b += '\0' * 222

f = open('${TMPFILE1}', 'wb')
f.write(b)
f.close()
"

openssl rsautl -sign -inkey $1 -raw -in ${TMPFILE1} -out ${TMPFILE2} 
RET=$?

python -c "
f = open('${TMPFILE2}', 'rb')
d = f.read()
f.close()
b = ''
for i in range(0, len(d), 2):
        b += d[i + 1] + d[i]
f = open('$2.sign', 'wb')
f.write(b)
f.close()
"

rm -f ${TMPFILE1} ${TMPFILE2}
exit ${RET}
