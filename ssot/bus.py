from ssot.gentype import (
    GenType, GenFieldRef, GenFieldBits, GenFieldInt, GenFieldString,
)

class BusInterface(GenType):
    class_prefix = "Bus"
    slug_prefix = 'bus'

class BusPci(metaclass=BusInterface):
    """PCI or AGP bus -- includes on-chip AGP for Celsius/Kelvin IGPs."""

class BusPcie(metaclass=BusInterface):
    """PCI-Express bus."""

class BusIgp(metaclass=BusInterface):
    """An on-chip custom bus interface for Curie/Tesla IGPs."""

class BusFlexIO(metaclass=BusInterface):
    """FlexIO interface (for RSX)."""

class BusTegra(metaclass=BusInterface):
    """An on-chip custom bus interface for Tegra IGPs."""


from ssot.cgen import CGenerator, CPartEnum

cgen = CGenerator('bus', [], [
    CPartEnum(BusInterface),
])
