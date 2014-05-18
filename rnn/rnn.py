# Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
# Copyright (C) 2010 Luca Barbieri <luca@luca-barbieri.com>
# Copyright (C) 2010 Francisco Jerez <currojerez@riseup.net>
# Copyright (C) 2010 Martin Peres <martin.peres@ensi-bourges.fr>
# Copyright (C) 2010 Marcin Slusarz <marcin.slusarz@gmail.com>
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

import sys

def catstr(a, b):
    if a is None:
        return b
    else:
        return a + '_' + b

class Author:
    def __init__(self, name, email, contributions, nicknames):
        self.name = name
        self.email = email
        self.contributions = contributions
        self.nicknames = nicknames

class Copyright:
    def __init__(self):
        self.firstyear = None
        self.license = None
        self.authors = []

class Database:
    def __init__(self):
        self.copyright = Copyright()
        self.enums = []
        self.bitsets = []
        self.domains = []
        self.groups = []
        self.spectypes = []
        self.files = set()
        self.estatus = 0

    def prep(self):
        for enum in self.enums:
            if not enum.inline:
                enum.prep(self)
        for bitset in self.bitsets:
            if not bitset.inline:
                bitset.prep(self)
        for domain in self.domains:
            domain.prep(self)
        for spectype in self.spectypes:
            spectype.prep(self)

    def findenum(self, name):
        for enum in self.enums:
            if enum.name == name:
                return enum

    def findbitset(self, name):
        for bitset in self.bitsets:
            if bitset.name == name:
                return bitset

    def finddomain(self, name):
        for domain in self.domains:
            if domain.name == name:
                return domain

    def findspectype(self, name):
        for spectype in self.spectypes:
            if spectype.name == name:
                return spectype

class Varset:
    def __init__(self, venum, variants):
        self.venum = venum
        self.variants = variants

    def copy(self):
        return Varset(self.venum, set(self.variants))


class Varinfo:
    def __init__(self, prefixstr=None, varsetstr=None, variantsstr=None):
        self.prefixstr = prefixstr
        self.varsetstr = varsetstr
        self.variantsstr = variantsstr
        self.dead = False
        self.prefenum = None
        self.prefix = None
        self.varsets = []

    def prep(self, db, what, parent):
        if parent:
            self.prefenum = parent.prefenum

        if self.prefixstr:
            if self.prefixstr == 'none':
                self.prefenum = 0
            else:
                self.prefenum = db.findenum(self.prefixstr) # XXX

        if parent:
            self.varsets += [ varset.copy() for varset in parent.varsets ]

        varset = self.prefenum
        if not varset and not self.varsetstr and parent:
            self.varsetstr = parent.varsetstr
        if self.varsetstr:
            varset = db.findenum(self.varsetstr)
        if self.variantsstr:
            if not varset:
                sys.stderr.write("{}: tried to use variants without active varset!\n".format(what))
                db.estatus = 1
                return

            for vs in self.varsets:
                if vs.venum == varset:
                    break
            else:
                vs = Varset(varset, set(range(len(varset.vals))))
                self.varsets.append(vs)

            curvars = set()
            nvars = len(varset.vals)
            subvars = self.variantsstr.split(' ')
            for subvar in subvars:
                if ':' in subvar:
                    pos = subvar.find(':')
                    first = subvar[:pos]
                    second = subvar[pos+1:]
                    idx1 = varset.findvidx(db, first) if first else 0
                    idx2 = varset.findvidx(db, second) if second else nvars
                elif '-' in subvar:
                    pos = subvar.find('-')
                    first = subvar[:pos]
                    second = subvar[pos+1:]
                    idx1 = varset.findvidx(db, first) if first else 0
                    idx2 = varset.findvidx(db, second) + 1 if second else nvars
                else:
                    idx1 = varset.findvidx(db, subvar)
                    idx2 = None if idx1 is None else idx1 + 1

                if idx1 is not None and idx2 is not None:
                    curvars |= set(range(idx1, idx2))

            vs.variants &= curvars
            if not vs.variants:
                self.dead = True
                return

        if self.prefenum:
            for vs in self.varsets:
                if vs.venum == self.prefenum:
                    self.prefix = self.prefenum.vals[min(vs.variants)].name
                    break
            else:
                self.prefix = self.prefenum.vals[0].name

    def copy(self):
        return Varinfo(self.prefixstr, self.varsetstr, self.variantsstr)


class Enum:
    def __init__(self, name, bare, inline, varinfo, vals, file):
        self.name = name
        self.bare = bare
        self.inline = inline
        self.varinfo = varinfo
        self.vals = vals
        self.fullname = None
        self.prepared = False
        self.file = file

    def prep(self, db):
        if self.prepared:
            raise RuntimeError("prepared")
        self.varinfo.prep(db, self.name, None)
        for val in self.vals:
            val.prep(db, None if self.bare else self.name, self.varinfo)
        self.fullname = catstr(self.varinfo.prefix, self.name)
        self.prepared = True

    def findvidx(self, db, name):
        for i, val in enumerate(self.vals):
            if val.name == name:
                return i
        sys.stderr.write("Cannot find variant {} in enum {}!\n".format(name, self.name))
        db.estatus = 1
        return None

class Value:
    def __init__(self, name, value, varinfo, file):
        self.name = name
        self.value = value
        self.varinfo = varinfo
        self.fullname = None
        self.file = file

    def prep(self, db, prefix, parvi):
        self.fullname = catstr(prefix, self.name)
        self.varinfo.prep(db, self.fullname, parvi)
        if self.varinfo.dead:
            return
        if self.varinfo.prefix:
            self.fullname = catstr(self.varinfo.prefix, self.fullname)

    def copy(self, file):
        return Value(self.name, self.value, self.varinfo.copy(), file)


class TypeHex:
    def __init__(self, shr=0, add=0, min=None, max=None, align=None):
        self.shr = shr
        self.add = add
        self.min = min
        self.max = max
        self.align = align

class TypeInt(TypeHex):
    def __init__(self, shr=0, add=0, min=None, max=None, align=None, signed=False):
        super().__init__(shr, add, min, max, align)
        self.signed = signed

class TypeBoolean:
    pass

class TypeFloat:
    pass

class TypeFixed:
    def __init__(self, min=None, max=None, radix=0, signed=False):
        self.min = min
        self.max = max
        self.radix = radix
        self.signed = signed

class TypeUnprep:
    def __init__(self, name, bitfields, vals, shr, add, min, max, align, radix):
        self.name = name
        self.bitfields = bitfields
        self.vals = vals
        self.shr = shr
        self.add = add
        self.min = min
        self.max = max
        self.align = align
        self.radix = radix

    def resolve(self, db, prefix, vi, width, file):
        if self.name:
            en = db.findenum(self.name)
            bs = db.findbitset(self.name)
            st = db.findspectype(self.name)
            if en:
                if en.inline:
                    vals = [ val.copy(file) for val in en.vals ]
                    for val in vals:
                        val.prep(db, prefix, vi)
                    return Enum(None, False, True, Varinfo(), vals, file)
                else:
                    return en
            elif bs:
                if bs.inline:
                    bitfields = [ bitfield.copy(file) for bitfield in bs.bitfields ]
                    for bitfield in bitfields:
                        bitfield.prep(db, prefix, vi)
                    return Bitset(None, False, True, Varinfo(), bitfields, file)
                else:
                    return bs
            elif st:
                return st
            elif self.name == 'hex':
                return TypeHex(self.shr, self.add, self.min, self.max, self.align)
            elif self.name == 'uint':
                return TypeInt(self.shr, self.add, self.min, self.max, self.align)
            elif self.name == 'int':
                return TypeInt(self.shr, self.add, self.min, self.max, self.align, signed=True)
            elif self.name == 'boolean':
                return TypeBoolean()
            elif self.name == 'float':
                return TypeFloat()
            elif self.name == 'fixed':
                return TypeFixed(self.min, self.max, self.radix, signed=True)
            elif self.name == 'ufixed':
                return TypeFixed(self.min, self.max, self.radix)
            else:
                sys.stderr.write("{}: unknown type {}\n".format(prefix, self.name))
                db.estatus = 1
                return TypeHex(self.shr, self.add, self.min, self.max)
        elif self.bitfields:
            for bitfield in self.bitfields:
                bitfield.prep(db, prefix, vi)
            return Bitset(None, False, True, Varinfo(), self.bitfields, file)
        elif self.vals:
            for val in self.vals:
                val.prep(db, prefix, vi)
            return Enum(None, False, True, Varinfo(), self.vals, file)
        elif width == 1:
            return TypeBoolean()
        else:
            return TypeHex(self.shr, self.add, self.min, self.max, self.align)

    def copy(self, file):
        return TypeUnprep(self.name,
            [bitfield.copy(file) for bitfield in self.bitfields],
            [val.copy(file) for val in self.vals],
            self.shr,
            self.add,
            self.min,
            self.max,
            self.align,
            self.radix)


class Bitset:
    def __init__(self, name, bare, inline, varinfo, bitfields, file):
        self.name = name
        self.bare = bare
        self.inline = inline
        self.varinfo = varinfo
        self.bitfields = bitfields
        self.fullname = None
        self.file = file

    def prep(self, db):
        self.varinfo.prep(db, self.name, None)
        for bitfield in self.bitfields:
            bitfield.prep(db, None if self.bare else self.name, self.varinfo)
        self.fullname = catstr(self.varinfo.prefix, self.name)


class Bitfield:
    def __init__(self, name, low, high, varinfo, typeinfo, file):
        self.name = name
        self.low = low
        self.high = high
        self.mask = (1 << (high + 1)) - (1 << low)
        self.width = self.high - self.low + 1
        self.varinfo = varinfo
        self.typeinfo = typeinfo
        self.fullname = None
        self.file = file

    def prep(self, db, prefix, parvi):
        self.fullname = catstr(prefix, self.name)
        self.varinfo.prep(db, self.fullname, parvi)
        if self.varinfo.dead:
            return
        self.typeinfo = self.typeinfo.resolve(db, self.fullname, self.varinfo, self.width, self.file)
        if self.varinfo.prefix:
            self.fullname = catstr(self.varinfo.prefix, self.fullname)

    def copy(self, file):
        return Bitfield(self.name, self.low, self.high, self.varinfo.copy(), self.typeinfo.copy(file), file)


class Domain:
    def __init__(self, name, bare, width, size, varinfo, elems, file):
        self.name = name
        self.bare = bare
        self.width = width
        self.size = size
        self.varinfo = varinfo
        self.elems = elems
        self.fullname = None
        self.file = file

    def prep(self, db):
        self.varinfo.prep(db, self.name, None)
        for elem in self.elems:
            elem.prep(db, None if self.bare else self.name, self.varinfo, self.width)
        self.fullname = catstr(self.varinfo.prefix, self.name)

class Group:
    def __init__(self, name, elems):
        self.name = name
        self.elems = elems

class Reg:
    def __init__(self, name, width, access, offset, length, stride, varinfo, typeinfo, file):
        self.name = name
        self.width = width
        self.access = access
        self.offset = offset
        self.length = length
        self.stride = stride
        self.varinfo = varinfo
        self.typeinfo = typeinfo
        self.fullname = None
        self.file = file

    def prep(self, db, prefix, parvi, width):
        self.fullname = catstr(prefix, self.name)
        self.varinfo.prep(db, self.fullname or prefix, parvi)
        if self.varinfo.dead:
            return
        if self.length != 1 and self.stride is None:
            self.stride = self.width // width
        self.typeinfo = self.typeinfo.resolve(db, self.fullname, self.varinfo, self.width, self.file)
        if self.varinfo.prefix:
            self.fullname = catstr(self.varinfo.prefix, self.fullname)

    def copy(self, file):
        return Reg(self.name, self.width, self.access, self.offset, self.length, self.stride, self.varinfo.copy(), self.typeinfo.copy(file), file)



class Stripe:
    def __init__(self, name, offset, length, stride, full, elems, varinfo, file):
        self.name = name
        self.offset = offset
        self.length = length
        self.stride = stride
        self.full = full
        self.elems = elems
        self.varinfo = varinfo
        self.fullname = None
        self.file = file

    def prep(self, db, prefix, parvi, width):
        if self.name:
            self.fullname = catstr(prefix, self.name)
            parname = self.fullname
        else:
            parname = prefix
        self.varinfo.prep(db, self.fullname or prefix, parvi)
        if self.varinfo.dead:
            return
        if self.length != 1 and self.stride is None:
            sys.stderr.write("{} has non-1 length, but no stride!\n".format(self.fullname))
            db.estatus = 1
        for elem in self.elems:
            elem.prep(db, parname, self.varinfo, width)
        if self.varinfo.prefix and self.name:
            self.fullname = catstr(self.varinfo.prefix, self.fullname)

    def copy(self, file):
        return Stripe(self.name, self.offset, self.length, self.stride, self.full,
            [elem.copy(file) for elem in self.elems],
            self.varinfo.copy(), file)


class UseGroup:
    def __init__(self, name, file):
        self.name = name
        self.file = file

    def prep(self, db, prefix, parvi, width):
        for group in db.groups:
            if group.name == self.name:
                self.elems = [ elem.copy(self.file) for elem in group.elems ]
                break
        else:
            sys.stderr.write("group {} not found!\n".format(self.name))
            db.estatus = 1
            self.elems = []

        self.__class__ = Stripe
        self.name = None
        self.offset = 0
        self.stride = None
        self.length = 1
        self.full = False
        self.varinfo = Varinfo()
        self.fullname = None
        self.prep(db, prefix, parvi, width)

    def copy(self, file):
        return UseGroup(self.name, file)


class SpecType:
    def __init__(self, name, typeinfo, file):
        self.name = name
        self.typeinfo = typeinfo
        self.width = 32 # XXX doesn't exactly make sense...
        self.file = file

    def prep(self, db):
        self.typeinfo = self.typeinfo.resolve(db, self.name, Varinfo(), self.width, self.file)

def T(x):
    return '{http://nouveau.freedesktop.org/}' + x

def getattrs(node, attrs, db, file):
    res = [ node.attrib.get(attr, None) for attr in attrs ]
    for attr in node.attrib:
        if attr not in attrs:
            sys.stderr.write("{}: wrong attribute \"{}\" for {}\n".format(file, attr, node.tag))
            db.estatus = 1
    return res

def tobool(x):
    if x in ['yes', '1']:
        return True
    elif x in [None, 'no', '0']:
        return False
    raise TypeError("invalid boolean value")

def parsevalue(db, file, node):
    name, value, varset, variants = getattrs(node, ['name', 'value', 'varset', 'variants'], db, file)
    if value is not None:
        value = int(value, 0)
    for child in node:
        if child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in value: <{}>\n".format(file, child.tag))
            db.estatus = 1
    if not name:
        sys.stderr.write("{}: nameless value\n".format(file))
        db.estatus = 1
        return None
    else:
        return Value(name, value, Varinfo(None, varset, variants), file)

def parsespectype(db, file, node):
    name, type, shr, add, min, max, align, radix = getattrs(
        node, ['name', 'type', 'shr', 'add', 'min', 'max', 'align', 'radix'], db, file)
    shr = 0 if shr is None else int(shr, 0)
    add = 0 if add is None else int(add, 0)
    min = None if min is None else int(min, 0)
    max = None if max is None else int(max, 0)
    align = None if align is None else int(align, 0)
    radix = None if radix is None else int(radix, 0)
    bitfields = []
    vals = []
    for child in node:
        if child.tag == T('value'):
            vals.append(parsevalue(db, file, child))
        elif child.tag == T('bitfield'):
            bitfields.append(parsebitfield(db, file, child))
        elif child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in spectype: <{}>\n".format(file, child.tag))
            db.estatus = 1
    if not name:
        sys.stderr.write("{}: nameless spectype\n".format(file))
        db.estatus = 1
        return
    for spectype in db.spectypes:
        if spectype.name == name:
            sys.stderr.write("{}: duplicated spectype name {}\n".format(file, name))
            db.estatus = 1
            return
    db.spectypes.append(SpecType(name, TypeUnprep(type, bitfields, vals, shr, add, min, max, align, radix), file))

def parseenum(db, file, node):
    name, bare, inline, prefixstr, varsetstr, variantsstr = getattrs(node, ['name', 'bare', 'inline', 'prefix', 'varset', 'variants'], db, file)
    bare = tobool(bare)
    inline = tobool(inline)
    if not name:
        sys.stderr.write("{}: nameless enum\n".format(file))
        db.estatus = 1
        return
    for enum in db.enums:
        if enum.name == name:
            if (enum.varinfo.prefixstr != prefixstr or
                    enum.varinfo.varsetstr != varsetstr or
                    enum.varinfo.variantsstr != variantsstr or
                    enum.inline != inline or enum.bare != bare):
                sys.stderr.write("{}: merge fail for enum {}\n".format(file, name))
                db.estatus = 1
            break
    else:
        enum = Enum(name, bare, inline, Varinfo(prefixstr, varsetstr, variantsstr), [], file)
        db.enums.append(enum)
    for child in node:
        if child.tag == T('value'):
            val = parsevalue(db, file, child)
            if val:
                enum.vals.append(val)
        elif child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in enum: <{}>\n".format(file, child.tag))
            db.estatus = 1

def parsebitfield(db, file, node):
    name, high, low, pos, varset, variants, type, shr, add, min, max, align, radix = getattrs(
        node, ['name', 'high', 'low', 'pos', 'varset', 'variants', 'type', 'shr', 'add', 'min', 'max', 'align', 'radix'], db, file)
    if pos is not None and high is None and low is None:
        high = low = int(pos)
    elif high is not None and low is not None and pos is None:
        high = int(high)
        low = int(low)
    else:
        sys.stderr.write("{}: bitfield {} has invalid placement attr\n".format(file, name))
        db.estatus = 1
    shr = 0 if shr is None else int(shr, 0)
    add = 0 if add is None else int(add, 0)
    min = None if min is None else int(min, 0)
    max = None if max is None else int(max, 0)
    align = None if align is None else int(align, 0)
    radix = None if radix is None else int(radix, 0)
    bitfields = []
    vals = []
    for child in node:
        if child.tag == T('value'):
            vals.append(parsevalue(db, file, child))
        elif child.tag == T('bitfield'):
            bitfields.append(parsebitfield(db, file, child))
        elif child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in bitfield: <{}>\n".format(file, child.tag))
            db.estatus = 1
    if not name:
        sys.stderr.write("{}: nameless bitfield\n".format(file))
        db.estatus = 1
        return None
    if high < low:
        sys.stderr.write("{}: bitfield has wrong placement\n".format(file))
        db.estatus = 1
        return None
    return Bitfield(name, low, high, Varinfo(None, varset, variants), TypeUnprep(type, bitfields, vals, shr, add, min, max, align, radix), file)

def parsebitset(db, file, node):
    name, bare, inline, prefixstr, varsetstr, variantsstr = getattrs(node, ['name', 'bare', 'inline', 'prefix', 'varset', 'variants'], db, file)
    bare = tobool(bare)
    inline = tobool(inline)
    if not name:
        sys.stderr.write("{}: nameless bitset\n".format(file))
        db.estatus = 1
        return
    for bitset in db.bitsets:
        if bitset.name == name:
            if (bitset.varinfo.prefixstr != prefixstr or
                    bitset.varinfo.varsetstr != varsetstr or
                    bitset.varinfo.variantsstr != variantsstr or
                    bitset.inline != inline or bitset.bare != bare):
                sys.stderr.write("{}: merge fail for bitset {}\n".format(file, name))
                db.estatus = 1
            break
    else:
        bitset = Bitset(name, bare, inline, Varinfo(prefixstr, varsetstr, variantsstr), [], file)
        db.bitsets.append(bitset)
    for child in node:
        if child.tag == T('bitfield'):
            bf = parsebitfield(db, file, child)
            if bf:
                bitset.bitfields.append(bf)
        elif child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in enum: <{}>\n".format(file, child.tag))
            db.estatus = 1

def parseusegroup(db, file, node):
    name, = getattrs(node, ['name'], db, file)
    if not name:
        sys.stderr.write("{}: nameless use-group\n".format(file))
        db.estatus = 1
        return
    return UseGroup(name, file)

def parsestripe(db, file, node, full):
    name, offset, length, stride, prefix, varset, variants = getattrs(
            node, ['name', 'offset', 'length', 'stride', 'prefix', 'varset', 'variants'], db, file)
    offset = 0 if offset is None else int(offset, 0)
    length = 1 if length is None else int(length, 0)
    stride = None if stride is None else int(stride, 0)
    elems = []
    for child in node:
        if child.tag in delemtags:
            elems.append(delemtags[child.tag](db, file, child))
        elif child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in stripe: <{}>\n".format(file, child.tag))
            db.estatus = 1
    # Sanity checking */
    if full and not stride:
        sys.stderr.write("{}: Array {}'s stride is undefined. Aborting.\n".format(file, name))
        raise ValueError()
    return Stripe(name, offset, length, stride, full, elems, Varinfo(prefix, varset, variants), file)

def parsereg(db, file, node, width):
    name, offset, length, stride, varset, variants, access, type, shr, add, min, max, align, radix = getattrs(
            node, ['name', 'offset', 'length', 'stride', 'varset', 'variants', 'access', 'type', 'shr', 'add', 'min', 'max', 'align', 'radix'], db, file)
    offset = int(offset, 0)
    length = 1 if length is None else int(length, 0)
    stride = None if stride is None else int(stride, 0)
    shr = 0 if shr is None else int(shr, 0)
    add = 0 if add is None else int(add, 0)
    min = None if min is None else int(min, 0)
    max = None if max is None else int(max, 0)
    align = None if align is None else int(align, 0)
    radix = None if radix is None else int(radix, 0)
    if access is None:
        access = 'rw'
    if access not in ['r', 'w', 'rw']:
        sys.stderr.write("{}: wrong access type \"{}\" for register\n".format(file, access))
        db.estatus = 1
    bitfields = []
    vals = []
    for child in node:
        if child.tag == T('value'):
            vals.append(parsevalue(db, file, child))
        elif child.tag == T('bitfield'):
            bitfields.append(parsebitfield(db, file, child))
        elif child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in reg: <{}>\n".format(file, child.tag))
            db.estatus = 1
    if not name:
        sys.stderr.write("{}: nameless register\n".format(file))
        db.estatus = 1
        return None
    return Reg(name, width, access, offset, length, stride, Varinfo(None, varset, variants), TypeUnprep(type, bitfields, vals, shr, add, min, max, align, radix), file)

def parsegroup(db, file, node):
    name, = getattrs(node, ['name'], db, file)
    if not name:
        sys.stderr.write("{}: nameless group\n".format(file))
        db.estatus = 1
        return
    for group in db.groups:
        if group.name == name:
            break
    else:
        group = Group(name, [])
        db.groups.append(group)
    for child in node:
        if child.tag in delemtags:
            group.elems.append(delemtags[child.tag](db, file, child))
        elif child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in group: <{}>\n".format(file, child.tag))
            db.estatus = 1


def parsedomain(db, file, node):
    name, bare, size, width, prefixstr, varsetstr, variantsstr = getattrs(node, ['name', 'bare', 'size', 'width', 'prefix', 'varset', 'variants'], db, file)
    bare = tobool(bare)
    size = None if size is None else int(size, 0)
    width = 8 if width is None else int(width)
    if not name:
        sys.stderr.write("{}: nameless domain\n".format(file))
        db.estatus = 1
        return
    for domain in db.domains:
        if domain.name == name:
            if (domain.varinfo.prefixstr != prefixstr or
                    domain.varinfo.varsetstr != varsetstr or
                    domain.varinfo.variantsstr != variantsstr or
                    domain.width != width or
                    domain.bare != bare or
                    (size and domain.size and size != domain.size)):
                sys.stderr.write("{}: merge fail for domain {}\n".format(file, name))
                db.estatus = 1
            if size:
                domain.size = size
            break
    else:
        domain = Domain(name, bare, width, size, Varinfo(prefixstr, varsetstr, variantsstr), [], file)
        db.domains.append(domain)
    for child in node:
        if child.tag in delemtags:
            domain.elems.append(delemtags[child.tag](db, file, child))
        elif child.tag in toptags:
            toptags[child.tag](db, file, child)
        elif child.tag not in doctags:
            sys.stderr.write("{}: wrong tag in domain: <{}>\n".format(file, child.tag))
            db.estatus = 1


def parseauthor(db, file, node):
    name, email = getattrs(node, ['name', 'email'], db, file)
    nicknames = []
    for child in node:
        if child.tag == T('nick'):
            nickname, = getattrs(child, ['name'], db, file)
            if nickname is None:
                sys.stderr.write("{}: missing \"name\" attribute for nick\n".format(file))
                db.estatus = 1
            else:
                nicknames.append(nickname)
        else:
            sys.stderr.write("{}: wrong tag in author: <{}>\n".format(file, child.tag))
            db.estatus = 1
    return Author(name, email, node.text, nicknames)


def parsecopyright(db, file, node):
    year, = getattrs(node, ['year'], db, file)
    if year:
        firstyear = int(year)
        if db.copyright.firstyear is None or firstyear < db.copyright.firstyear:
            db.copyright.firstyear = firstyear

    for child in node:
        if child.tag == T('license'):
            if db.copyright.license is not None and db.copyright.license != node.text:
                sys.stderr.write("fatal error: multiple different licenses specified!\n")
                os.abort() # TODO: do something better here, but headergen, xml2html, etc. should not produce anything in this case
            else:
                db.copyright.license = node.text
        elif child.tag == T('author'):
            db.copyright.authors.append(parseauthor(db, file, child))
        else:
            sys.stderr.write("{}: wrong tag in copyright: <{}>\n".format(file, child.tag))
            db.estatus = 1


def parseimport(db, file, node):
    subfile, = getattrs(node, ['file'], db, file)
    if not subfile:
        sys.stderr.write("{}:{}: missing \"file\" attribute for import\n".format(file, node.line))
        db.estatus = 1
    else:
        parsefile(db, subfile)

toptags = {
    T('enum'): parseenum,
    T('bitset'): parsebitset,
    T('group'): parsegroup,
    T('domain'): parsedomain,
    T('spectype'): parsespectype,
    T('import'): parseimport,
    T('copyright'): parsecopyright,
}

delemtags = {
    T('reg8'): lambda db, file, node: parsereg(db, file, node, 8),
    T('reg16'): lambda db, file, node: parsereg(db, file, node, 16),
    T('reg32'): lambda db, file, node: parsereg(db, file, node, 32),
    T('reg64'): lambda db, file, node: parsereg(db, file, node, 64),
    T('stripe'): lambda db, file, node: parsestripe(db, file, node, False),
    T('array'): lambda db, file, node: parsestripe(db, file, node, True),
    T('use-group'): parseusegroup,
}

doctags = (T('doc'), T('brief'))

# XXX
RNN_DEF_PATH = "/home/mwk/envytools/rnndb:/usr/local/share/rnndb"

import os
import xml.etree.ElementTree

def find_in_paths(name, paths):
    for path in paths.split(':'):
        if path:
            fullname = os.path.join(path, name)
            try:
                file = open(fullname)
                return file, fullname
            except FileNotFoundError:
                pass
    raise FileNotFoundError(name)

def parsefile(db, fname):
    if fname in db.files:
        return

    rnn_path = os.environ.get("RNN_PATH", RNN_DEF_PATH)

    try:
        file, fullname = find_in_paths(fname, rnn_path)
        file.close()
    except FileNotFoundError:
        sys.stderr.write("{}: couldn't find database file. Please set the env var RNN_PATH.\n".format(fname))
        db.estatus = 1
        return

    db.files.add(fname)
    try:
        tree = xml.etree.ElementTree.parse(fullname)
    except xml.etree.ElementTree.ParseError:
        sys.stderr.write("{}: couldn't open database file. Please set the env var RNN_PATH.\n".format(fname))
        db.estatus = 1
        return
    root = tree.getroot()
    if root.tag != T('database'):
        sys.stderr.write("{}: wrong top-level tag <{}>\n".format(fname, root.tag))
        db.estatus = 1
    else:
        for elem in root:
            if elem.tag in toptags:
                toptags[elem.tag](db, fname, elem)
            elif elem.tag not in doctags:
                sys.stderr.write("{}: wrong tag in database: <{}>\n".format(fname, elem.tag))
                db.estatus = 1
