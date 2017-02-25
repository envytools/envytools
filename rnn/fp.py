import struct

# see https://en.wikipedia.org/wiki/Half-precision_floating-point_format
def float16i(val):
    val &= 0xffff
    sign = (val & 0x8000) << 16
    frac = val & 0x3ff
    expn = (val >> 10) & 0x1f

    if expn == 0:
        if frac:
            # denormalized number
            shift = 11 - frac.bit_length()
            frac <<= shift
            frac &= 0x3ff
            expn = 1 - shift
        else:
            # +/- zero
            return sign
    elif expn == 0x1f:
        # Inf/NaN
        return sign | 0x7f800000 | (frac << 13)

    return sign | ((expn + 127 - 15) << 23) | (frac << 13)

def float16(val):
    return float32(float16i(val))

def float32(val):
    return struct.unpack('<f', struct.pack('<i', val))[0]

def float64(val):
    return struct.unpack('<d', struct.pack('<q', val))[0]
