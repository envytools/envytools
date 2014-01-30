from docutils.parsers.rst import directives
from sphinx import addnodes

from sphinx.domains import Domain
from sphinx.directives import ObjectDescription

class EnvyReg(ObjectDescription):
    required_arguments = 2
    optional_arguments = 1
    final_argument_whitespace = True

    def get_signatures(self):
        width, gname, rest = self.arguments
        width = int(width)
        brief, *locs = rest.split('\n')
        return [(width, gname, brief)] + locs

    def handle_signature(self, sig, signode):
        if isinstance(sig, tuple):
            width, gname, brief = sig
            reg = "reg{} ".format(width)
            signode += addnodes.desc_addname(reg, reg)
            signode += addnodes.desc_name(gname, gname)
            return sig
        else:
            signode += addnodes.desc_name(sig, sig)
            return sig


class EnvySpace(ObjectDescription):
    required_arguments = 3
    optional_arguments = 1
    final_argument_whitespace = True
    option_spec = {
        'root': directives.unchanged,
        'noindex': directives.flag,
    }

    def get_signatures(self):
        width, gname, size, rest = self.arguments
        width = int(width)
        size = int(size, 16)
        brief, *locs = rest.split('\n')
        return [(width, gname, size, brief)] + locs

    def handle_signature(self, sig, signode):
        if isinstance(sig, tuple):
            width, gname, size, brief = sig
            space = "{}-bit space ".format(width)
            signode += addnodes.desc_addname(space, space)
            signode += addnodes.desc_name(gname, gname)
            sz = " [{:#x}]".format(size)
            signode += addnodes.desc_addname(sz, sz)
            return sig
        else:
            signode += addnodes.desc_name(sig, sig)
            return sig


class EnvyDomain(Domain):
    name = 'envy'
    label = 'envytools'
    object_types = {
    }
    directives = {
        'reg': EnvyReg,
        'space': EnvySpace,
    }
    roles = {
    }
    initial_data = {
    }
    data_version = 0


def setup(app):
    app.add_domain(EnvyDomain)
