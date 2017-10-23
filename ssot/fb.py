from ssot.gentype import (
    GenType, GenFieldBits, GenFieldInt, GenFieldString,
    GenFieldBool
)

class FbGen(GenType):
    class_prefix = 'Fb'
    slug_prefix = 'fb'
    vram_addr_bits = GenFieldInt()
    tile_regions = GenFieldInt(optional=True)
    has_comp = GenFieldBool()
    is_igp = GenFieldBool()
    # Size of the compression tag RAM, in bits.
    comp_ram_size = GenFieldInt(optional=True)

# NV1 insane unified memory+display controller -- 4MB max.
class FbNV1(metaclass=FbGen):
    vram_addr_bits = 22
    has_comp = False
    is_igp = False

# NV3 new memory controller -- still 4MB max.
class FbNV3(metaclass=FbGen):
    vram_addr_bits = 22
    has_comp = False
    is_igp = False

# Bumped to max 8MB.
class FbNV3T(FbNV3):
    vram_addr_bits = 23

# Bumped to max 16MB.
class FbNV4(FbNV3T):
    vram_addr_bits = 24

# Bumped to max 32MB.
class FbNV5(FbNV4):
    vram_addr_bits = 25

# NVA memory controller -- nothing known.
class FbNVA(FbNV5):
    # XXX
    vram_addr_bits = ...
    is_igp = True

# Bumped to max 128MB, introduced tiled regions.
class FbNV10(FbNV5):
    vram_addr_bits = 27
    tile_regions = 8

# NV1A memory controller -- actually resides on a different PCI device on
# the northbridge.
class FbNV1A(metaclass=FbGen):
    # XXX
    vram_addr_bits = ...
    tile_regions = 8
    is_igp = True
    has_comp = False

class FbNV1F(FbNV1A):
    # XXX
    pass

class FbNV20(FbNV10):
    vram_addr_bits = 30
    has_comp = True
    comp_ram_size = 0x8000

class FbNV2A(FbNV20):
    is_igp = True
    comp_ram_size = ...

class FbNV25(FbNV20):
    comp_ram_size = 0xc000

class FbNV30(FbNV25):
    comp_ram_size = 0x2e400

class FbNV31(FbNV30):
    comp_ram_size = 0x40000

class FbNV35(FbNV30):
    comp_ram_size = 0x5c800

class FbNV36(FbNV35):
    comp_ram_size = 0xba000

class FbNV40(FbNV36):
    comp_ram_size = 0x2e400

class FbNV41(FbNV40):
    tile_regions = 12

class FbNV43(FbNV41):
    comp_ram_size = 0x5c800

class FbG70(FbNV43):
    tile_regions = 15
    comp_ram_size = 0x48000

class FbG73(FbG70):
    tile_regions = 15
    comp_ram_size = 0x5cc00

class FbNV44(FbNV10):
    tile_regions = 12

class FbG72(FbNV44):
    tile_regions = 15

class FbC51(FbNV44):
    is_igp = True

class FbMCP61(FbC51):
    tile_regions = 15

class FbG80(FbG73):
    vram_addr_bits = 32
    tile_regions = None
    comp_ram_size = ...

class FbMCP77(FbG80):
    is_igp = True

class FbGF100(metaclass=FbGen):
    vram_addr_bits = 33
    is_igp = False
    has_comp = True

class FbGK20A(FbGF100):
    is_igp = True


from ssot.cgen import CGenerator, CPartEnum

cgen = CGenerator('fb', [], [
    CPartEnum(FbGen),
])
