from sphinx import addnodes

from sphinx.domains import Domain
from sphinx.directives import ObjectDescription

class EnvyReg(ObjectDescription):
    required_arguments = 2
    optional_arguments = 1
    final_argument_whitespace = True

    def get_signatures(self):
        width, gname, rest = self.arguments
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

class EnvyDomain(Domain):
    name = 'envy'
    label = 'envytools'
    object_types = {
    }
    directives = {
        'reg': EnvyReg,
    }
    roles = {
    }
    initial_data = {
    }
    data_version = 0


def setup(app):
    app.add_domain(EnvyDomain)
