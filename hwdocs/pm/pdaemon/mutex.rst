.. _pdaemon-mutex:
.. _pdaemon-perf-mutex:
.. _pdaemon-io-mutex:
.. _pdaemon-io-token:

================
Hardware mutexes
================

The PDAEMON has hardware support for 16 busy-waiting mutexes accessed by up to
254 clients simultanously. The clients may be anything able to read and write
the PDAEMON registers - code running on host, on PDAEMON, or on any other falcon
engine with MMIO access powers.

The clients are identified by tokens. Tokens are 8-bit numbers in 0x01-0xfe
range. Tokens may be assigned to clients statically by software, or dynamically
by hardware. Only tokens 0x08-0xfe will be dynamically allocated by hardware
- software may use statically assigned tokens 0x01-0x07 even if dynamic tokens
are in use at the same time. The registers used for dynamic token allocation
are:

MMIO 0x488 / I[0x12200]: TOKEN_ALLOC
  Read-only, each to this register allocates a free token and gives it as the
  read result. If there are no free tokens, 0xff is returned.

MMIO 0x48c / I[0x12300]: TOKEN_FREE
  A write to this register will free a token, ie. return it back to the pool
  used by TOKEN_ALLOC. Only low 8 bits of the written value are used.
  Attempting to free a token outside of the dynamic allocation range
  [0x08-0xff] or a token already in the free queue will have no effect.
  Reading this register will show the last written value, invalid or not.

The free tokens are stored in a FIFO - the freed tokens will be used by
TOKEN_ALLOC in the order of freeing. After reset, the free token FIFO will
contain tokens 0x08-0xfe in ascending order.

The actual mutex locking and unlocking is done by the MUTEX_TOKEN registers:

MMIO 0x580+i*4 / I[0x16000+i*0x100], i<16: MUTEX_TOKEN[i]
  The 16 mutices. A value of 0 means unlocked, any other value means locked
  by the client holding the corresponding token. Only low 8 bits of the written
  value are used. A write of 0 will unlock the mutex and will always succeed.
  A write of 0x01-0xfe will succeed only if the mutex is currently unlocked.
  A write of 0xff is invalid and will always fail. A failed write has no
  effect.

The token allocation circuitry additionally exports four signals to PCOUNTER:

- TOKEN_ALL_USED: 1 iff all tokens are currently allocated [ie. a read from
  TOKEN_ALLOC would return 0xff]
- TOKEN_NONE_USED: 1 iff no tokens are currently allocated [ie. tokens
  0x08-0xfe are all in free tokens queue]
- TOKEN_FREE: pulses for 1 cycle whenever TOKEN_FREE is written, even if with
  invalid value
- TOKEN_ALLOC: pulses for 1 cycle whenever TOKEN_ALLOC is read, even if
  allocation fails
