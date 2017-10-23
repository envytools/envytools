def name_to_words(name):
    res = []
    state = 'empty'
    pos = 0
    for i, c in enumerate(name):
        if state == 'empty':
            if not c.isupper():
                raise ValueError('gentype name must start with uppercase letter')
            state = 'upper'
        elif state == 'upper':
            if c.islower():
                if pos != i - 1:
                    res.append(name[pos:i - 1])
                    pos = i - 1
                state = 'lower'
            elif c == '_':
                res.append(name[pos:i])
                pos = i + 1
                state = 'empty'
            elif c.isdigit():
                state = 'upper-digit'
        elif state == 'upper-digit':
            if c.islower():
                state = 'lower'
            elif c == '_':
                res.append(name[pos:i])
                pos = i + 1
                state = 'empty'
            elif c.isdigit():
                state = 'upper-digit'
            else:
                state = 'upper'
        elif state == 'lower':
            if c.isupper():
                res.append(name[pos:i])
                pos = i
                state = 'upper'
            elif c == '_':
                res.append(name[pos:i])
                pos = i + 1
                state = 'empty'
    if pos != len(name):
        res.append(name[pos:])
    return res


class GenType:
    class_prefix = ""

    def __init__(self, name, bases, ns):
        assert name.startswith(self.class_prefix)
        self.name = name[len(self.class_prefix):]
        words = name_to_words(name)
        self.slug = '-'.join(word.lower() for word in words)
        self.lowname = '_'.join(word.lower() for word in words)
        self.capsname = '_'.join(word.upper() for word in words)
        self.brief = self.name
        self.own_attrs = ns
        assert len(bases) <= 1
        if bases:
            self.attrs = dict(bases[0].attrs)
        else:
            self.attrs = {
                k: None
                for k in self.fields
            }
        self.doc = None
        for k, v in ns.items():
            if k == '__doc__':
                self.doc = v
            elif k.startswith('__') and k.endswith('__'):
                pass
            elif k == 'brief':
                self.brief = v
            elif k in self.fields:
                self.fields[k].validate(v)
                self.attrs[k] = v
            else:
                raise AttributeError('gentype {} has no attribute {}'.format(type(self).__name__, k))
        for k, v in self.fields.items():
            if self.attrs[k] is None and not v.optional:
                raise ValueError('attribute {} is not filled'.format(k))
        self.instances.append(self)

    def __init_subclass__(cls, **kwargs):
        cls.instances = []
        cls.fields = {
            k: v
            for k, v in cls.__dict__.items()
            if isinstance(v, GenFieldBase)
        }
        words = name_to_words(cls.__name__)
        cls.cls_slug = '-'.join(word.lower() for word in words)
        cls.cls_lowname = '_'.join(word.lower() for word in words)
        cls.cls_capsname = '_'.join(word.upper() for word in words)
        super().__init_subclass__(*kwargs)

    def __repr__(self):
        return self.class_prefix + self.name

    def __str__(self):
        return self.name


# Fields.

class GenFieldBase:
    def __init__(self, optional=False):
        self.optional = optional

    def __set_name__(self, owner, name):
        self.owner = owner
        self.name = name

    def __get__(self, instance, owner):
        if instance is None:
            return self
        return instance.attrs[self.name]

    def validate(self, val):
        if val is ...:
            return
        if val is None and self.optional:
            return
        self.validate_inner(val)

class GenFieldRef(GenFieldBase):
    def __init__(self, target_type, *args, **kwargs):
        self.target_type = target_type
        super().__init__(*args, **kwargs)

    def validate_inner(self, val):
        if not isinstance(val, self.target_type):
            raise TypeError('field {} must be of type {}'.format(self.name, self.target_type.__name__))

class GenFieldBits(GenFieldBase):
    def __init__(self, size, *args, **kwargs):
        self.size = size
        super().__init__(*args, **kwargs)

    def validate_inner(self, val):
        if not isinstance(val, int):
            raise TypeError('field {} must be an integer'.format(self.name))
        if val not in range(1 << self.size):
            raise TypeError('field {} must fit in {} bits'.format(self.name, self.size))

class GenFieldInt(GenFieldBase):
    def validate_inner(self, val):
        if not isinstance(val, int):
            raise TypeError('field {} must be an integer'.format(self.name))

class GenFieldString(GenFieldBase):
    def validate_inner(self, val):
        if not isinstance(val, str):
            raise TypeError('field {} must be a string'.format(self.name))

class GenFieldBool(GenFieldBase):
    def validate_inner(self, val):
        if not isinstance(val, bool):
            raise TypeError('field {} must be a bool'.format(self.name))
