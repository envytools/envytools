.. _pdaemon-crc:
.. _pdaemon-io-crc:

===============
CRC computation
===============

The PDAEMON has a very simple CRC accelerator. Specifically, it can perform
the CRC accumulation operation on 32-bit chunks using the standard CRC-32
polynomial of 0xedb88320. The current CRC residual is stored in the CRC_STATE
register:

MMIO 0x494 / I[0x12500]: CRC_STATE
  The current CRC residual. Read/write.

And the data to add to the CRC is written to the CRC_DATA register:

MMIO 0x490 / I[0x12400]: CRC_DATA
    When written, appends the 32-bit LE value to the running CRC residual in
    CRC_STATE. When read, returns the last value written. Write operation::

        CRC_STATE ^= value;
        for (i = 0; i < 32; i++) {
            if (CRC_STATE & 1) {
                CRC_STATE >>= 1;
                CRC_STATE ^= 0xedb88320;
            } else {
                CRC_STATE >>= 1;
            }
        }

To compute a CRC:

1. Write the initial CRC residue to CRC_STATE
2. Write all data to CRC_DATA, in 32-bit chunks
3. Read CRC_STATE, xor its value with the final constant, use that as the CRC.

If the data block to CRC has size that is not a multiple of 32 bits, the extra
bits at the end [or the beginning] have to be handled manually.
