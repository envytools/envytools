from docutils.parsers.rst import Directive, directives
from docutils import nodes
from sphinx import addnodes
from sphinx.roles import XRefRole
from sphinx.domains import Domain, ObjType, Index
from sphinx.util.nodes import make_refnode


class uplink_placeholder(nodes.General, nodes.Element):
    def __init__(self, name):
        self.name = name
        super().__init__()


class sub_placeholder(nodes.General, nodes.Element):
    def __init__(self, name):
        self.name = name
        super().__init__()


class Reg:
    iname = 'reg'
    def __init__(self, width, name, brief):
        self.width = width
        self.name = name
        self.brief = brief


class Space:
    iname = 'space'
    def __init__(self, width, name, size, brief):
        self.width = width
        self.name = name
        self.size = size
        self.brief = brief
        self.presubs = []


class ObjectDescription(Directive):
    has_content = True

    def after_content(self):
        return []

    def run(self):
        if ':' in self.name:
            self.domain, self.objtype = self.name.split(':', 1)
        else:
            self.domain, self.objtype = '', self.name
        self.env = self.state.document.settings.env
        self.indexnode = addnodes.index(entries=[])

        obj = self.make_obj()
        node = addnodes.desc()
        node.document = self.state.document
        node['domain'] = self.domain
        # 'desctype' is a backwards compatible attribute
        node['objtype'] = node['desctype'] = self.objtype
        node['noindex'] = noindex = ('noindex' in self.options)
        node.name = obj.name
        obj.docname = self.env.docname

        objects = self.env.domaindata['envy']['objects']

        signode = addnodes.desc_signature('', '')
        signode['first'] = True
        node.append(signode)
        self.make_signature(obj, signode)
        if not noindex and self.name not in objects:
            # only add target and index entry if this is the first
            # description of the object with this name in this desc block
            #self.add_target_and_index(self.name, sig, signode)
            nid = obj.iname + '-' + self.name
            signode['names'].append(nid)
            signode['ids'].append(nid)
            self.state.document.note_explicit_target(signode)

        for loc in self.locs:
            signode = addnodes.desc_signature('', '')
            signode['first'] = False
            node.append(signode)
            signode += addnodes.desc_name(loc, loc)

        node.append(uplink_placeholder(self.name))

        if self.name in objects:
            other = objects[self.name]
            self.state_machine.reporter.warning('duplicate object {}, other instance in {}'.format(self.name, self.env.doc2path(other.docname)))
        objects[self.name] = obj

        contentnode = addnodes.desc_content()
        node.append(contentnode)
        self.env.temp_data['object'] = self.name
        self.state.nested_parse(self.content, self.content_offset, contentnode)
        self.env.temp_data['object'] = None
        contentnode += self.after_content()
        return [self.indexnode, node]


class EnvyReg(ObjectDescription):
    required_arguments = 3
    final_argument_whitespace = True

    def make_obj(self):
        width, self.name, rest = self.arguments
        brief, *locs = rest.split('\n')
        self.locs = locs # XXX nuke me
        return Reg(int(width), self.name, brief)

    def make_signature(self, obj, signode):
        reg = "reg{} ".format(obj.width)
        signode += addnodes.desc_addname(reg, reg)
        signode += addnodes.desc_name(obj.name, obj.name)


class EnvySpace(ObjectDescription):
    required_arguments = 4
    final_argument_whitespace = True
    option_spec = {
        'root': directives.unchanged,
        'noindex': directives.flag,
    }

    def after_content(self):
        subnode = sub_placeholder(self.name)
        return [subnode]

    def make_obj(self):
        width, self.name, size, rest = self.arguments
        brief, *subs = rest.split('\n')
        obj = Space(int(width), self.name, int(size, 16), brief)
        for sub in subs:
            pos, name, ref, *rest = sub.split()
            if rest:
                variants, *rest = rest
                if variants == '*':
                    variants = None
            else:
                variants = None
            if rest:
                tags, = rest
            else:
                tags = None
            obj.presubs.append((pos, name, ref, variants))
        self.locs = []
        return obj

    def make_signature(self, obj, signode):
        space = "{}-bit space ".format(obj.width)
        signode += addnodes.desc_addname(space, space)
        signode += addnodes.desc_name(self.name, self.name)
        sz = " [{:#x}]".format(obj.size)
        signode += addnodes.desc_addname(sz, sz)


def envy_connect(app, env):
    objects = env.domaindata['envy']['objects']

    # clear uplinks
    for obj in objects.values():
        obj.uplinks = []

    # resolve space sub refs
    for sp in objects.values():
        if isinstance(sp, Space):
            sp.subs = []
            for pos, name, ref, variants in sp.presubs:
                try:
                    obj = objects[ref]
                except KeyError:
                    app.warn('space {} refers to nonexistent object {}'.format(sp.name, ref))
                else:
                    if not isinstance(obj, (Reg, Space)):
                        app.warn('space {} refers to object {} of type {}'.format(sp.name, ref, type(obj)))
                    else:
                        sp.subs.append((pos, name, obj, variants))
                        obj.uplinks.append((sp, pos, name, variants))

def wrap_text_entry(txt):
    entry = nodes.entry()
    para = nodes.paragraph()
    entry += para
    para += nodes.Text(txt, txt)
    return entry

def envy_resolve(app, doctree, fromdocname):
    objects = app.env.domaindata['envy']['objects']

    # add uplink info
    for holder in doctree.traverse(uplink_placeholder):
        obj = objects[holder.name]
        links = []
        for sp, pos, name, variants in obj.uplinks:
            signode = addnodes.desc_signature('', '')
            signode['first'] = False
            signode += make_refnode(app.builder, fromdocname, sp.docname, sp.iname + '-' + sp.name, addnodes.desc_addname(sp.name, sp.name), sp.name)
            text = ' {}: {}'.format(pos, name)
            signode += addnodes.desc_name(text, text)
            if variants is not None:
                text = ' [{}]'.format(variants)
                signode += addnodes.desc_annotation(text, text)
            links.append(signode)
        holder.replace_self(links)

    # add subnode list
    for holder in doctree.traverse(sub_placeholder):
        obj = objects[holder.name]
        add_variant = False
        for pos, name, child, variants in obj.subs:
            if variants is not None:
                add_variant = True
        table = nodes.table()
        headers = [(1, 'Address'), (1, 'Name'), (10, 'Description')]
        if add_variant:
            headers.insert(1, (1, 'Variants'))
        tgroup = nodes.tgroup(cols=len(headers))
        table += tgroup
        for colwidth, header in headers:
            tgroup += nodes.colspec(colwidth=colwidth)
        thead = nodes.thead()
        tgroup += thead
        headrow = nodes.row()
        for colwidth, header in headers:
            entry = nodes.entry()
            para = nodes.paragraph()
            entry += para
            para += nodes.Text(header, header)
            headrow += entry
        thead += headrow
        tbody = nodes.tbody()
        tgroup += tbody
        for pos, name, child, variants in obj.subs:
            row = nodes.row()
            row += wrap_text_entry(pos)
            if add_variant:
                row += wrap_text_entry('all' if variants is None else variants)
            row += wrap_text_entry(name)
            entry = nodes.entry()
            para = nodes.paragraph()
            entry += para
            para += make_refnode(app.builder, fromdocname, child.docname, child.iname + '-' + child.name, nodes.Text(child.brief, child.brief), child.brief)
            row += entry
            tbody += row
        holder.replace_self([table])


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
        'obj': XRefRole(),
    }
    initial_data = {
        'objects' : {}  # name -> envydesc
    }
    data_version = 0

    def clear_doc(self, docname):
        for name, node in list(self.data['objects'].items()):
            if node.docname == docname:
                del self.data['objects'][name]

    def resolve_xref(self, env, fromdocname, builder, type, target, node, contnode):
        obj = self.data['objects'].get(target)
        if obj is not None:
            return make_refnode(builder, fromdocname, obj.docname, obj.iname + '-' + obj.name, contnode, obj.brief)


def setup(app):
    app.add_domain(EnvyDomain)
    app.connect('env-updated', envy_connect)
    app.connect('doctree-resolved', envy_resolve)
