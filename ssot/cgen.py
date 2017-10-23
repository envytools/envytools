from ssot.gentype import GenFieldBits, GenFieldInt, GenFieldRef

class CPartEnum:
    def __init__(self, gen_type):
        self.gen_type = gen_type

    def needed_headers(self):
        return set()

    def write_h(self, file):
        file.write('enum {}_index {{\n'.format(self.gen_type.cls_lowname))
        for inst in self.gen_type.instances:
            file.write('\t{},\n'.format(inst.capsname))
        file.write('\tNUM_{},\n'.format(self.gen_type.cls_capsname))
        file.write('\t{0}_UNKNOWN = NUM_{0},\n'.format(self.gen_type.cls_capsname))
        file.write('};\n')
        file.write('\n')

    def write_c(self, file):
        pass


class StructName:
    pass


class CPartStruct:
    def __init__(self, gen_type, fields):
        self.gen_type = gen_type
        self.fields = fields

    def needed_headers(self):
        return {'<stdint.h>', '<stdbool.h>'}

    def write_h(self, file):
        file.write('extern const struct {} {{\n'.format(self.gen_type.cls_lowname))
        for field in self.fields:
            if field is StructName:
                file.write('\tconst char *name;\n')
            elif isinstance(field, GenFieldBits):
                if field.size <= 32:
                    file.write('\tuint32_t {};\n'.format(field.name))
                elif field.size <= 64:
                    file.write('\tuint64_t {};\n'.format(field.name))
                else:
                    raise NotImplementedError
                file.write('\tbool {}_valid;\n'.format(field.name))
            elif isinstance(field, GenFieldInt):
                file.write('\tint {};\n'.format(field.name))
                file.write('\tbool {}_valid;\n'.format(field.name))
            elif isinstance(field, GenFieldRef):
                file.write('\tenum {}_index {};\n'.format(field.target_type.cls_lowname, field.name))
            else:
                print(field)
                raise NotImplementedError
        file.write('}} {}_list[];\n'.format(self.gen_type.cls_lowname))
        file.write('\n')

    def write_c(self, file):
        file.write('const struct {} {}_list[] = {{\n'.format(self.gen_type.cls_lowname, self.gen_type.cls_lowname))
        for inst in self.gen_type.instances:
            file.write('\t{ ')
            for field in self.fields:
                if field is StructName:
                    file.write('"{}", '.format(inst.name))
                elif isinstance(field, GenFieldBits):
                    val = getattr(inst, field.name)
                    if val is None:
                        file.write('0, false, ')
                    elif val is ...:
                        file.write('-1, false, ')
                    else:
                        file.write('{:#x}, true, '.format(val))
                elif isinstance(field, GenFieldInt):
                    val = getattr(inst, field.name)
                    if val is None:
                        file.write('0, false, ')
                    elif val is ...:
                        file.write('-1, false, ')
                    else:
                        file.write('{}, true, '.format(val))
                elif isinstance(field, GenFieldRef):
                    val = getattr(inst, field.name)
                    if val is None:
                        file.write('-1, ')
                    elif val is ...:
                        file.write('{}_UNKNOWN, '.format(field.target_type.cls_capsname))
                    else:
                        file.write('{}, '.format(val.capsname))
                else:
                    print(field)
                    raise NotImplementedError
            file.write('},\n')
        file.write('};\n')
        file.write('\n')


class CGenerator:
    def __init__(self, name, deps, parts):
        self.parts = parts
        self.deps = deps
        self.name = name

    def write_h(self, file):
        guard = 'CGEN_{}_H'.format(self.name.replace('/', '_').upper())
        file.write('/* This file is auto-generated -- do not edit. */\n')
        file.write('#ifndef {}\n'.format(guard))
        file.write('#define {}\n'.format(guard))
        needed_headers = set()
        for part in self.parts:
            needed_headers |= part.needed_headers()
        for header in sorted(needed_headers):
            file.write('#include {}\n'.format(header))
        for dep in self.deps:
            file.write('#include "cgen/{}.h"\n'.format(dep.name))
        file.write('\n')
        for part in self.parts:
            part.write_h(file)
        file.write('#endif\n')

    def write_c(self, file):
        file.write('/* This file is auto-generated -- do not edit. */\n')
        file.write('#include "cgen/{}.h"\n'.format(self.name))
        file.write('\n')
        for part in self.parts:
            part.write_c(file)
