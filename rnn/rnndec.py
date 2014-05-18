# Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
# Copyright (C) 2010 Francisco Jerez <currojerez@riseup.net>
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

import rnn
import fp
import sys

class Context:
    def __init__(self, db, colors):
        self.db = db
        self.colors = colors
        self.vars = [] # XXX dict?

    def varadd(self, varset, variant):
        en = self.db.findenum(varset)
        if en is None:
            raise ValueError("Enum {} doesn't exist in database!".format(varset))
        for idx, val in enumerate(en.vals):
            if variant == val.name:
                self.vars.append((en, idx))
                break
        else:
            raise ValueError("Variant {} doesn't exist in enum {}!".format(variant, varset))

    def varmatch(self, vi):
        if vi.dead:
            return False
        for vs in vi.varsets:
            for en, idx in self.vars:
                if en == vs.venum:
                    if idx not in vs.variants:
                        return False
                    break
            else:
                sys.stderr.write("I don't know which {} variant to use!\n".format(vs.venum.name))
        return True

    def decodeval(self, ti, value, width):
        if isinstance(ti, rnn.Enum):
            for val in ti.vals:
                if self.varmatch(val.varinfo) and val.value == value:
                    return self.colors('val', val.name)
        elif isinstance(ti, rnn.Bitset):
            mask = 0
            res = []
            for bitfield in ti.bitfields:
                if not self.varmatch(bitfield.varinfo):
                    continue
                sval = (value & bitfield.mask) >> bitfield.low
                mask |= bitfield.mask
                if isinstance(bitfield.typeinfo, rnn.TypeBoolean):
                    if sval == 0:
                        continue
                    elif sval == 1:
                        res.append(self.colors('mod', bitfield.name))
                        continue
                subval = self.decodeval(bitfield.typeinfo, sval, bitfield.width)
                res.append("{} = {}".format(self.colors('rname', bitfield.name), subval))
            if value & ~mask:
                res.append(self.colors('err', "{:#x}".format(value & ~mask)))
            if not res:
                res.append(self.colors('num', "0"))
            return "{{ {} }}".format(" | ".join(res))
        elif isinstance(ti, rnn.SpecType):
            return self.decodeval(ti.typeinfo, value, width)
        elif isinstance(ti, rnn.TypeInt):
            if ti.signed and value & 1 << (width - 1):
                value |= -1 << width
            return self.colors('num', str((value << ti.shr) + ti.add))
        elif isinstance(ti, rnn.TypeHex):
            return self.colors('num', "{:#x}".format((value << ti.shr) + ti.add))
        elif isinstance(ti, rnn.TypeBoolean):
            if value == 0:
                return self.colors('eval', "FALSE")
            elif value == 1:
                return self.colors('eval', "TRUE")
        elif isinstance(ti, rnn.TypeFixed):
            if ti.signed and value & 1 << (width - 1):
                value |= -1 << width
            value = value / (1 << ti.radix) # XXX
            return self.colors('num', str(value))
        elif isinstance(ti, TypeFloat):
            if width == 64:
                return self.colors('num', str(fp.float64(value)))
            elif width == 32:
                return self.colors('num', str(fp.float32(value)))
            elif width == 16:
                return self.colors('num', str(fp.float16(value)))
        return self.colors('err', "{:#x}".format(value))

    def _trymatch(self, elems, addr, write, dwidth, indices):
        for elem in elems:
            if not self.varmatch(elem.varinfo):
                continue
            if isinstance(elem, rnn.Reg):
                if addr < elem.offset:
                    continue
                if elem.stride:
                    idx = (addr - elem.offset) // elem.stride
                    offset = (addr - elem.offset) % elem.stride
                    eindices = indices + [idx] 
                else:
                    idx = 0
                    offset = addr - elem.offset
                    eindices = indices
                if offset >= elem.width // dwidth:
                    continue
                if elem.length and idx >= elem.length:
                    continue
                name = self.colors('rname', elem.name)
                for idx in eindices:
                    name += "[{}]".format(self.colors('num', "{:#x}".format(idx)))
                if offset:
                    name += "+ {}".format(self.colors('err', "{:#x}".format(offset)))
                return name, elem.typeinfo, elem.width
            elif isinstance(elem, rnn.Stripe):
                if not elem.full:
                    if elem.stride:
                        m = (addr - elem.offset) // elem.stride + 1
                        if elem.length:
                            m = min(elem.length, m)
                        r = range(0, m)
                    else:
                        r = 0,
                    for idx in r:
                        if addr < elem.offset + (elem.stride or 0) * idx:
                            break;
                        offset = addr - elem.offset + (elem.stride or 0) * idx
                        if elem.length != 1:
                            eindices = indices + [idx] 
                        else:
                            eindices = indices
                        res = self._trymatch(elem.elems, offset, write, dwidth, eindices if not elem.name else [])
                        if res is None:
                            continue;
                        if not elem.name:
                            return res
                        name = self.colors('rname', elem.name)
                        for idx in eindices:
                            name += "[{}]".format(self.colors('num', "{:#x}".format(idx)))
                        subname, typeinfo, width = res
                        return "{}.{}".format(name, subname), typeinfo, width
                else:
                    if addr < elem.offset:
                        continue
                    idx = (addr - elem.offset) // elem.stride
                    offset = (addr - elem.offset) % elem.stride
                    if elem.length and idx >= elem.length:
                        continue
                    name = self.colors('rname', elem.name)
                    if elem.length != 1:
                        eindices = indices + [idx] 
                    else:
                        eindices = indices
                    for idx in eindices:
                        name += "[{}]".format(self.colors('num', "{:#x}".format(idx)))
                    res = self._trymatch(elem.elems, offset, write, dwidth, [])
                    if res is not None:
                        subname, typeinfo, width = res
                        return "{}.{}".format(name, subname), typeinfo, width
                    else:
                        name += "+ {}".format(self.colors('err', "{:#x}".format(offset)))
                        return name, None, None
        return None

    def decodeaddr(self, domain, addr, write):
        res = self._trymatch(domain.elems, addr, write, domain.width, [])
        if res is None:
            return self.colors('err', "{:#x}".format(addr)), None, None
        return res
