# Copyright (C) 2013 Marcin Kościelnicki <koriakin@0x04.net>
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
# VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

class Colors(dict):
    def __call__(self, which, text):
        if which not in self:
            return text
        col = self[which]
        if isinstance(col, str):
            return col + text
        else:
            start, end = col
            return start + text + end

null = Colors()
term = Colors(
    iname=('\x1b[0;32m', '\x1b[0m'),
    rname=('\x1b[0;32m', '\x1b[0m'),
    mod=('\x1b[0;36m', '\x1b[0m'),
    sym=('\x1b[0;36m', '\x1b[0m'),
    reg=('\x1b[0;31m', '\x1b[0m'),
    regsp=('\x1b[0;35m', '\x1b[0m'),
    num=('\x1b[0;33m', '\x1b[0m'),
    mem=('\x1b[0;35m', '\x1b[0m'),
    btarg=('\x1b[0;35m', '\x1b[0m'),
    ctarg=('\x1b[0;1;37m', '\x1b[0m'),
    bctarg=('\x1b[0;1;35m', '\x1b[0m'),
    eval=('\x1b[0;35m', '\x1b[0m'),
    comm=('\x1b[0;34m', '\x1b[0m'),
    err=('\x1b[0;1;31m', '\x1b[0m'),
)

schemes = {
    'null' : null,
    'term' : term,
}
