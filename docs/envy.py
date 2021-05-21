from docutils.parsers.rst import Directive, directives
from docutils import nodes
from docutils.statemachine import ViewList
from sphinx import addnodes
from sphinx.roles import XRefRole
from sphinx.domains import Domain, ObjType, Index
from sphinx.util.nodes import make_refnode
from sphinx.util.docstrings import prepare_docstring
from ssot.gentype import GenFieldBits
from ssot.gpu import Gpu, GpuGen


# New stuff

class ColumnName:
    def header(self):
        return 'Name'

    def data(self, obj):
        txt = obj.name
        return nodes.Text(txt, txt)


class ColumnField:
    def __init__(self, head, field):
        self.head = head
        self.field = field

    def header(self):
        return self.head

    def data(self, obj):
        data = obj.attrs[self.field.name]
        if data is None:
            txt = '-'
        elif data is ...:
            txt = '?'
        elif isinstance(self.field, GenFieldBits):
            txt = format(data, '#0{}x'.format((self.field.size + 3) // 4 + 2))
        else:
            txt = str(data)
        return nodes.Text(txt, txt)


class ColumnRef:
    def __init__(self, head, field):
        self.head = head
        self.field = field

    def header(self):
        return self.head

    def data(self, obj):
        data = obj.attrs[self.field.name]
        if data is None:
            return nodes.Text('-', '-')
        elif data is ...:
            return nodes.Text('?', '?')
        else:
            ref = addnodes.pending_xref(
                '', refdomain='envy', reftype='genobj', reftarget=data.slug,
            )
            ref += nodes.Text(data.name, data.name)
            return ref


class ColumnPciid:
    def header(self):
        return 'PCI device IDs'

    def data(self, obj):
        if obj.pciid is None:
            txt = '-'
        elif obj.pciid is ...:
            txt = '?'
        else:
            lo = obj.pciid
            hi = lo + (1 << obj.pciid_varbits) - 1
            if lo == hi:
                txt = format(lo, '#06x')
            else:
                txt = '{:#06x}-{:#06x}'.format(lo, hi)
        return nodes.Text(txt, txt)


class ColumnBiosVersion:
    def header(self):
        return 'BIOS version prefix'

    def data(self, obj):
        if obj.bios_major is None or obj.bios_chip is None:
            txt = '-'
        elif obj.bios_chip is ... or obj.bios_chip is ...:
            txt = '?'
        else:
            txt = '{:02x}.{:02x}'.format(obj.bios_major, obj.bios_chip)
        return nodes.Text(txt, txt)


def gen_table(columns, data):
        table = nodes.table()
        tgroup = nodes.tgroup(cols=len(columns))
        table += tgroup
        for column in columns:
            tgroup += nodes.colspec(colwidth=1)
        thead = nodes.thead()
        tgroup += thead
        headrow = nodes.row()
        for column in columns:
            entry = nodes.entry()
            para = nodes.paragraph()
            entry += para
            header = column.header()
            para += nodes.Text(header, header)
            headrow += entry
        thead += headrow
        tbody = nodes.tbody()
        tgroup += tbody
        for obj in data:
            row = nodes.row()
            for column in columns:
                entry = nodes.entry()
                para = nodes.paragraph()
                entry += para
                para += column.data(obj)
                row += entry
            tbody += row
        return [table]


class EnvyGpuTable(Directive):
    def run(self):
        return gen_table([
            ColumnName(),
            ColumnField('GPU id', Gpu.id),
            ColumnRef('GPU generation', Gpu.gen),
            ColumnField('Release date [approximate]', Gpu.date),
            ColumnField('Bus interface', Gpu.bus),
            ColumnField('PCI vendor id', Gpu.vendorid),
            ColumnPciid(),
            ColumnField('HDA PCI device id', Gpu.hda_pciid),
            ColumnField('USB PCI device id', Gpu.usb_pciid),
            ColumnField('UCSI PCI device id', Gpu.ucsi_pciid),
            ColumnBiosVersion(),
            ColumnField('FB type', Gpu.fb),
            ColumnField('# of FB partitions', Gpu.fbpart_count),
            ColumnField('# of MCs per FB partition', Gpu.mc_fbpart_count),
            ColumnField('# of SUBPs per FB partition', Gpu.subp_fbpart_count),
            ColumnField('# of XF units', Gpu.xf_count),
            ColumnField('# of GPCs', Gpu.gpc_count),
            ColumnField('# of TPCs [per GPC for Fermi+]', Gpu.tpc_count),
            ColumnField('# of SMs per TPC', Gpu.sm_tpc_count),
            ColumnField('# of PPCs per GPC', Gpu.ppc_count),
            ColumnField('# of CEs', Gpu.ce_count),
        ], Gpu.instances)


class GenObjPlaceholder:
    def __init__(self, docname, brief):
        self.docname = docname
        self.brief = brief


class EnvyGenType(Directive):
    required_arguments = 1

    def make_signature(self, obj, signode):
        signode += addnodes.desc_addname(self.objtype + ' ', self.objtype + ' ')
        signode += addnodes.desc_name(obj.name, obj.name)

    def run(self):
        if ':' in self.name:
            self.domain, self.objtype = self.name.split(':', 1)
        else:
            self.domain, self.objtype = '', self.name
        self.env = self.state.document.settings.env

        obj_name, = self.arguments
        for obj in self.gen_type.instances:
            if obj.name == obj_name:
                break
        else:
            raise ValueError('unknown instance {}'.format(obj_name))

        node = addnodes.desc()
        node.document = self.state.document
        node['domain'] = self.domain
        node['objtype'] = node['desctype'] = self.objtype
        node['noindex'] = False
        node.name = obj.slug
        pobj = GenObjPlaceholder(self.env.docname, obj.brief)

        genobjs = self.env.domaindata['envy']['genobjs']

        signode = addnodes.desc_signature('', '')
        signode['first'] = True
        node.append(signode)
        self.make_signature(obj, signode)
        signode['names'].append(obj.name)
        signode['ids'].append(obj.slug)
        self.state.document.note_explicit_target(signode)

        if obj.slug in genobjs:
            other = genobjs[obj.slug]
            self.state_machine.reporter.warning('duplicate object {}, other instance in {}'.format(obj.slug, self.env.doc2path(other.docname)))
        genobjs[obj.slug] = pobj

        contentnode = addnodes.desc_content()
        node.append(contentnode)
        vl = ViewList()
        doc = prepare_docstring(obj.doc or '')
        for line in doc:
            vl.append(line, obj_name)
        self.state.nested_parse(vl, 0, contentnode)

        return [
            node
        ]


class EnvyGpuGen(EnvyGenType):
    gen_type = GpuGen


# Old stuff


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

    def get_size(self, parent):
        if self.width % parent.width:
            app.warn('space {} of width {} has child {} of width {}'.format(parent.name, parent.width, self.name, self.width))
        return self.width // parent.width


class Space:
    iname = 'space'
    def __init__(self, width, name, size, brief):
        self.width = width
        self.name = name
        self.size = size
        self.brief = brief
        self.presubs = []

    def get_size(self, parent):
        if self.width != parent.width:
            app.warn('space {} of width {} has subspace {} of width {}'.format(parent.name, parent.width, self.name, self.width))
        return self.size


class Index:
    def __init__(self, name=None, num=None, stride=None):
        self.name = name
        self.num = num
        self.stride = stride

    def __str__(self):
        if isinstance(self.num, int):
            ret = hex(self.num)
        else:
            ret = self.num
        if self.stride is not None:
            ret = ret + '/' + hex(self.stride)
        if self.name is not None:
            ret = self.name + ':' + ret
        return ret


class Pos:
    def __init__(self, offset=0, indices=()):
        self.offset = offset
        self.indices = list(indices)

    def __str__(self):
        return hex(self.offset) + ''.join('[{}]'.format(index) for index in self.indices)

    def format_offset(self):
        res = hex(self.offset)
        for idx in self.indices:
            res += ' +{}*{:#x}'.format(idx.name, idx.stride)
        return res

    def format_square(self):
        res = ''
        for idx in self.indices:
            res += '[{}]'.format(idx.name)
        return res

    def format_post(self):
        chunks = []
        for idx in self.indices:
            chunks.append('{}<{}'.format(idx.name, idx.num))
        return chunks


def parse_index(s):
    rest, _, stride = s.partition('/')
    if stride == '':
        stride = None
    else:
        stride = int(stride, 0)
    name, _, num = rest.rpartition(':')
    if name == '':
        name = None
    try:
        num = int(num, 0)
    except ValueError:
        pass
    return Index(name, num, stride)


def parse_pos(s):
    offset, lp, rest = s.partition('[')
    rest = lp + rest
    offset = int(offset, 0)
    indices = []
    while rest:
        if rest[0] != '[':
            raise ValueError('index must be followed by another index or end of string')
        pos = 0
        lev = 0
        for c in rest:
            if c == '[':
                lev += 1
            elif c == ']':
                lev -= 1
            pos += 1
            if lev == 0:
                break
        else:
            raise ValueError('unbalanced []')
        indices.append(parse_index(rest[1:pos-1]))
        rest = rest[pos:]
    return Pos(offset, indices)


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
            try:
                pos, name, ref, *rest = sub.split()
                pos = parse_pos(pos)
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
            except ValueError as e:
                self.state_machine.reporter.warning('failed to parse sub {}: {}'.format(sub, e))
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
                        indices = []
                        size = obj.get_size(sp)
                        names = ['i', 'j', 'k', 'l']
                        num_names_needed = 0
                        for idx in reversed(pos.indices):
                            if idx.name is None:
                                num_names_needed += 1
                            if idx.stride is None:
                                idx = Index(idx.name, idx.num, size)
                            indices.append(idx)
                            size = idx.num * idx.stride
                        indices.reverse()
                        if num_names_needed > len(names):
                            names = ['i{}'.format(x) for x in range(num_names_needed)]
                        ni = 0
                        for i in range(len(indices)):
                            idx = indices[i]
                            if idx.name is None:
                                indices[i] = Index(names[ni], idx.num, idx.stride)
                                ni += 1
                        pos = Pos(pos.offset, indices)
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
            text = ' {}: {}{}'.format(pos.format_offset(), name, pos.format_square())
            post = pos.format_post()
            if post:
                text += ' ({})'.format(', '.join(post))
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
        if obj.subs:
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
                ptext = pos.format_offset()
                post = pos.format_post()
                if post:
                    ptext += ' ({})'.format(', '.join(post))
                row += wrap_text_entry(ptext)
                if add_variant:
                    row += wrap_text_entry('all' if variants is None else variants)
                row += wrap_text_entry(name + pos.format_square())
                entry = nodes.entry()
                para = nodes.paragraph()
                entry += para
                para += make_refnode(app.builder, fromdocname, child.docname, child.iname + '-' + child.name, nodes.Text(child.brief, child.brief), child.brief)
                row += entry
                tbody += row
            holder.replace_self([table])
        else:
            holder.replace_self([])


class EnvyDomain(Domain):
    name = 'envy'
    label = 'envytools'
    object_types = {
    }
    directives = {
        'gpu-gen': EnvyGpuGen,
        'gpu-table': EnvyGpuTable,
        'reg': EnvyReg,
        'space': EnvySpace,
    }
    roles = {
        'obj': XRefRole(),
        'genobj': XRefRole(),
    }
    initial_data = {
        'objects' : {},  # name -> envydesc
        'genobjs' : {},  # name -> (document, brief)
    }
    data_version = 0

    def clear_doc(self, docname):
        for name, node in list(self.data['objects'].items()):
            if node.docname == docname:
                del self.data['objects'][name]
        for name, obj in list(self.data['genobjs'].items()):
            if obj.docname == docname:
                del self.data['genobjs'][name]

    def resolve_xref(self, env, fromdocname, builder, type, target, node, contnode):
        if type == 'obj':
            obj = self.data['objects'].get(target)
            if obj is not None:
                return make_refnode(builder, fromdocname, obj.docname, obj.iname + '-' + obj.name, contnode, obj.brief)
        else:
            obj = self.data['genobjs'].get(target)
            if obj is not None:
                return make_refnode(builder, fromdocname, obj.docname, target, contnode, obj.brief)


def setup(app):
    app.add_domain(EnvyDomain)
    app.connect('env-updated', envy_connect)
    app.connect('doctree-resolved', envy_resolve)
