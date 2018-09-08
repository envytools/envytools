===========================================
envydis, envyas and envysched documentation
===========================================

.. contents::

Using envydis, envyas and envysched
===================================

``envydis`` reads from standard input and prints the disassembly to standard
output. By default, input is parsed as sequence space- or comma-separated
hexadecimal numbers representing the bytes to disassemble.

``envyas`` reads assembly from standard input and outputs to the filename
specified by -o <filename>.

``envysched`` reads assembly from standard input and outputs scheduled
assembly to stdout.

The options are:

Input format
------------

.. option:: -w

  (``envydis`` only) Instead of sequence of hexadecimal bytes, treat input as
  sequence of hexadecimal 32-bit words

.. option:: -W

  (``envydis`` only) Instead of sequence of hexadecimal bytes, treat input as
  sequence of hexadecimal 64-bit words

.. option:: -i

  (``envydis`` only) Treat input as pure binary

Input subranging
----------------

.. option:: -b <base>

  (``envydis`` only) Assume the start of input to be at address <base> in code
  segment

.. option:: -d <discard>

  (``envydis`` only) Discard that many bytes of input before starting to read
  code

.. option:: -l <limit>

  (``envydis`` only) Don't disassemble more than <limit> bytes

Variant selection
-----------------

.. option:: -m <machine>

  Select the ISA to disassemble/assemble. One of:

  - ``[****]`` g80: tesla CUDA/shader ISA
  - ``[*** ]`` gf100: fermi CUDA/shader ISA
  - ``[**  ]`` gk110: kepler GK110 CUDA/shader ISA
  - ``[*** ]`` gm107: maxwell CUDA/shader ISA
  - ``[**  ]`` ctx: nv40 and g80 PGRAPH context-switching microcode
  - ``[*** ]`` falcon: falcon microcode, used to power various engines on G98+ cards
  - ``[****]`` hwsq: PBUS hardware sequencer microcode
  - ``[****]`` xtensa: xtensa variant as used by video processor 2 [g84-gen]
  - ``[*** ]`` vuc: video processor 2/3 master/mocomp microcode
  - ``[****]`` macro: gf100 PGRAPH macro method ISA
  - ``[**  ]`` vp1: video processor 1 [nv41-gen] code
  - ``[****]`` vcomp: PVCOMP video compositor microcode

  Where the quality level is:

  - ``[    ]``: Bare beginnings
  - ``[*   ]``: Knows a few instructions
  - ``[**  ]``: Knows enough instructions to write some simple code
  - ``[*** ]``: Knows most instructions, enough to write advanced code
  - ``[****]``: Knows all instructions, or very close to.

  Currently only gm107 is supported with ``envysched``.

.. option:: -V <variant>

  Select variant of the ISA.

  For g80:

  - g80: The original G80 [aka compute capability 1.0]
  - g84: G84, G86, G92, G94, G96, G98 [aka compute capability 1.1]
  - g200: G200 [aka compute capability 1.3]
  - mcp77: MCP77, MCP79 [aka compute capability 1.2]
  - gt215: GT215, GT216, GT218, MCP89 [aka compute capability 1.2 + d3d10.1]

  For gf100:

  - gf100: GF100:GK104 cards
  - gk104: GK104+ cards

  For ctx:

  - nv40: NV40:G80 cards
  - g80: G80:G200 cards
  - g200: G200:GF100 cards

  For hwsq:

  - nv17: NV17:NV41 cards
  - nv41: NV41:G80 cards
  - g80: G80:GF100 cards

  For falcon:

  - fuc0: falcon version 0 [G98, MCP77, MCP79]
  - fuc3: falcon version 3 [GT215 and up]
  - fuc4: falcon version 4 [GF119 and up, selected engines only]
  - fuc5: falcon version 5 [GK208 and up, selected engines only]
  - fuc6: falcon version 6 [GP102 and up, selected engines only]

  For vuc:

  - vp2: VP2 video processor [G84:G98, G200]
  - vp3: VP3 video processor [G98, MCP77, MCP79]
  - vp4: VP4 video processor [GT215:GF119]

.. option:: -F <feature>

  Enable optional ISA feature. Most of these are auto-selected by :option:`-V`,
  but can also be specified manually. Can be used multiple times to enable
  several features.

  For g80:

  - sm11: SM1.1 new opcodes [selected by g84, g200, mcp77, gt215]
  - sm12: SM1.2 new opcodes [selected by g200, mcp77, gt215]
  - fp64: 64-bit floating point [selected by g200]
  - d3d10_1: Direct3D 10.1 new features [selected by gt215]

  For gf100:

  - gf100op: GF100:GK104 exclusive opcodes [selected by gf100]
  - gk104op: GK104+ exclusive opcodes [selected by gk104]

  For ctx:

  - nv40op: NV40:G80 exclusive opcodes [selected by nv40]
  - g80op: G80:GF100 exclusive opcodes [selected by g80, g200]
  - callret: call/ret opcodes [selected by g200]

  For hwsq:

  - nv17f: NV17:G80 flags [selected by nv17, nv41]
  - nv41f: NV41:G80 flags [selected by nv41]
  - nv41op: NV41 new opcodes [selected by nv41, g80]

  For falcon:

  - fuc0op: falcon version 0 exclusive opcodes [selected by fuc0]
  - fuc3op: falcon version 3+ exclusive opcodes [selected by fuc3, fuc4, fuc5, fuc6]
  - fuc4op: falcon version 4+ exclusive opcodes [selected by fuc4, fuc5, fuc6]
  - fuc5op: falcon version 5+ exclusive opcodes [selected by fuc5, fuc6]
  - fuc6op: falcon version 6+ exclusive opcodes [selected by fuc6]
  - pc24: 24-bit PC opcodes [selected by fuc4]
  - crypt: Cryptographic coprocessor opcodes [has to be manually selected]

  For vuc:

  - vp2op: VP2 exclusive opcodes [selected by vp2]
  - vp3op: VP3+ exclusive opcodes [selected by vp3, vp4]
  - vp4op: VP4 exclusive opcodes [selected by vp4]

.. option:: -O <mode>

  Select processor mode.

  For g80:

  - vp: Vertex program
  - gp: Geometry program
  - fp: Fragment program
  - cp: Compute program

.. option:: -S <stride>

  (``envydis`` and ``envyas`` only) Override stride length for ISA and variant (relevant in binary mode only).

.. option:: -M <mapfile>

  (``envydis`` only) Load map file.

.. option:: -u <value>

  (``envydis`` only) Set map file label value.

Output
------

.. option:: -o <filename>

  (``envyas`` only) Output to filename


Output format
-------------

.. option:: -n

  (``envydis`` only) Disable output coloring

.. option:: -q

  (``envydis`` only) Disable printing address + opcodes.

.. option:: -a

  (``envyas`` only) Decorate output with human-readable section names and labels

.. option:: -w

  (``envyas`` only) Output as a sequence of hexadecimal 32-bit words instead of
  bytes

.. option:: -W

  (``envyas`` only) Output as a sequence of hexadecimal 64-bit words instead of
  bytes

.. option:: -i

  (``envyas`` only) Output as pure binary

Scheduling
----------

.. option:: -a

  (``envysched`` only) Schedule all code as if between .beginsched/.endsched.

.. option:: -s

  (``envysched`` only) Ignore the sched instructions between
  .beginsched/.endsched instead of erroring.

.. option:: -p

  (``envysched`` only) Use the target-specific placeholder scheduling
  information. Slow, but reliable.

  This is ``sched (st 0x0) (st 0x0) (st 0x0)`` on gm107.
