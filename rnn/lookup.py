#!/usr/bin/env python3

# Copyright (C) 2010 Marcin Kościelnicki <koriakin@0x04.net>
# Copyright (C) 2011 Martin Peres <martin.peres@ensi-bourges.fr>
# Copyright (C) 2011 Witold Waligóra <witold.waligora@gmail.com>
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

import argparse
import sys
import rnn
import rnndec
import colors

def aint(x):
    return int(x, 0)

parser = argparse.ArgumentParser(description="rnn database lookup.")
parser.add_argument('-f', '--file', default='root.xml')
parser.add_argument('-a', '--chipset')
parser.add_argument('-c', '--nocolor', action='store_const', const=colors.null, dest='colors', default=colors.term)
select = parser.add_mutually_exclusive_group()
select.add_argument('-d', '--domain', default='NV_MMIO')
select.add_argument('-e', '--enum')
select.add_argument('-b', '--bitset')
parser.add_argument('-w', '--write', action='store_true', default=False)
parser.add_argument('-v', '--variant', nargs=2, action='append', default=[])
parser.add_argument('address', type=aint, help="Address to be looked up")
parser.add_argument('value', type=aint, nargs='?', help="Value to be looked up")
args = parser.parse_args()

db = rnn.Database()
rnn.parsefile(db, args.file)
db.prep()

vc = rnndec.Context(db, args.colors)
if args.chipset is not None:
    vc.varadd('chipset', args.chipset)
for varset, variant in args.variant:
    vc.varadd(varset, variant)

if args.enum:
    en = db.findenum(args.enum)
    if not en:
        sys.stderr.write("Not an enum: '{}'\n".format(args.enum))
        sys.exit(1)
    print(vc.decodeval(en, args.address, None))
elif args.bitset:
    bs = db.findbitset(args.bitset)
    if not bs:
        sys.stderr.write("Not a bitset: '{}'\n".format(args.bitset))
        sys.exit(1)
    print(vc.decodeval(bs, args.address, None))
else:
    dom = db.finddomain(args.domain)
    if not dom:
        sys.stderr.write("Not a domain: '{}'\n".format(args.domain))
        sys.exit(1)
    name, typeinfo, width = vc.decodeaddr(dom, args.address, args.write)
    if typeinfo is None:
        print(name)
    else:
        print(name, '=>', vc.decodeval(typeinfo, args.value, width))
