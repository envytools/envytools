.. _gpu:

=========
GPU chips
=========

.. contents::


Introduction
============

Each nvidia GPU has several identifying numbers that can be used to determine
supported features, the engines it contains, and the register set. The most
important of these numbers is an 8-bit number known as the "GPU id".
If two cards have the same GPU id, their GPUs support identical features,
engines, and registers, with very minor exceptions. Such cards can however
still differ in the external devices they contain: output connectors,
encoders, capture chips, temperature sensors, fan controllers, installed
memory, supported clocks, etc. You can get the GPU id of a card by reading
from its :ref:`PMC area <pmc-id>`.

The GPU id is usually written as NVxx, where xx is the id written as
uppercase hexadecimal number. Note that, while cards before NV10 used another
format for their ID register and don't have the GPU id stored directly,
they are usually considered as NV1-NV5 anyway.

Nvidia uses "GPU code names" in their materials. They started out
identical to the GPU id, but diverged midway through the NV40 series
and started using a different numbering. However, for the most part nvidia
code names correspond 1 to 1 with the GPU ids.

The GPU id has a mostly one-to-many relationship with pci device ids. Note that
the last few bits [0-6 depending on GPU] of PCI device id are
changeable through straps [see :ref:`pstraps`]. When pci ids of a GPU are
listed in this file, the following shorthands are used:

1234
    PCI device id 0x1234
1234*
    PCI device ids 0x1234-0x1237, choosable by straps
123X
    PCI device ids 0x1230-0x123X, choosable by straps
124X+
    PCI device ids 0x1240-0x125X, choosable by straps
124X*
    PCI device ids 0x1240-0x127X, choosable by straps


The GPU families
================

The GPUs can roughly be grouped into a dozen or so families: NV1, NV3/RIVA,
NV4/TNT, Celsius, Kelvin, Rankine, Curie, Tesla, Fermi, Kepler, Maxwell, Pascal,
Volta and Turing.

This aligns with big revisions of PGRAPH, the drawing engine of the card. While
most functionality was introduced in sync with PGRAPH revisions, some other
functionality [notably video decoding hardware] gets added in GPUs late in a GPU
family and sometimes doesn't even get to the first GPU in the next GPU family.
For example, NV11 expanded upon the previous NV15 chipset by adding dual-head
support, while NV20 added new PGRAPH revision with shaders, but didn't have
dual-head - the first GPU to feature both was NV25.

Also note that a bigger GPU id doesn't always mean a newer card / card
with more features: there were quite a few places where the numbering actually
went backwards. For example, NV11 came out later than NV15 and added several
features.

Nvidia's card release cycle always has the most powerful high-end GPU
first, subsequently filling in the lower-end positions with new cut-down
GPUs. This means that newer cards in a single sub-family get progressively
smaller, but also more featureful - the first GPUs to introduce minor
changes like DX10.1 support or new video decoding are usually the low-end
ones.

Whenever a range of GPUs is mentioned in the documentation, it's written as
"NVxx:NVyy". This is left-inclusive, right-noninclusive range of GPU ids
as sorted in the following list. For example, G200:GT218 means GPUs G200,
MCP77, MCP79, GT215, GT216. NV20:NV30 effectively means all NV20 family GPUs.

The full known GPU list, sorted roughly according to introduced features,
is:

- NV1 family: NV1
- NV3 (aka RIVA) family: NV3, NV3T
- NV4 (aka TNT)  family: NV4, NV5
- Celsius family: NV10, NV15, NV1A, NV11, NV17, NV1F, NV18
- Kelvin family: NV20, NV2A, NV25, NV28
- Rankine family: NV30, NV35, NV31, NV36, NV34
- Curie family:

  - NV40 subfamily: NV40, NV45, NV41, NV42, NV43, NV44, NV44A
  - G70 subfamily: G70, G71, G73, G72
  - the IGPs: C51, MCP61, MCP67, MCP68, MCP73
  - the special snowflake: RSX

- Tesla family:

  - G80 subfamily: G80
  - G84 subfamily: G84, G86, G92, G94, G96, G98
  - G200 subfamily: G200, MCP77, MCP79
  - GT215 subfamily: GT215, GT216, GT218, MCP89

- Fermi family:

  - GF100 subfamily: GF100, GF104, GF106, GF114, GF116, GF108, GF110
  - GF119 subfamily: GF119, GF117

- Kepler family: GK104, GK107, GK106, GK110, GK110B, GK208, GK208B, GK20A, GK210
- Maxwell family: GM107, GM108, GM200, GM204, GM206, GM20B
- Pascal family: GP100, GP102, GP104, GP106, GP107, GP108
- Volta family: GV100
- Turing family: TU102, TU104, TU106


NV1 family: NV1
---------------

.. gpu-gen:: NV1


NV3 (RIVA) family: NV3, NV3T
----------------------------

.. gpu-gen:: NV3


NV4 (TNT) family: NV4, NV5
--------------------------

.. gpu-gen:: NV4


Celsius family: NV10, NV15, NV1A, NV11, NV17, NV1F, NV18
--------------------------------------------------------

.. gpu-gen:: Celsius

===== ==== ========= ======= ========== ========
pciid GPU  pixel     texture date       notes
           pipelines units
           and ROPs
===== ==== ========= ======= ========== ========
0100* NV10 4         4       11.10.1999 the first GeForce card [GeForce 256]
0150* NV15 4         8       26.04.2000 the high-end card of GeForce 2 lineup [GeForce 2 Ti, ...]
01a0* NV1A 2         4       04.06.2001 the IGP of GeForce 2 lineup [nForce]
0110* NV11 2         4       28.06.2000 the low-end card of GeForce 2 lineup [GeForce 2 MX]
017X  NV17 2         4       06.02.2002 the low-end card of GeForce 4 lineup [GeForce 4 MX]
01fX  NV1F 2         4       01.10.2002 the IGP of GeForce 4 lineup [nForce 2]
018X  NV18 2         4       25.09.2002 like NV17, but with added AGP x8 support
===== ==== ========= ======= ========== ========

NV1A and NV1F are IGPs and lack VRAM, memory controller, mediaport, and ROM
interface. They use the internal interfaces of the northbridge to access
an area of system memory set aside as fake VRAM and BIOS image.


Kelvin family: NV20, NV2A, NV25, NV28
-------------------------------------

.. gpu-gen:: Kelvin


The GPUs are:

===== ==== ======= ========= ======= ========== ========
pciid GPU  vertex  pixel     texture date       notes
           shaders pipelines units
                   and ROPs
===== ==== ======= ========= ======= ========== ========
0200* NV20 1       4         8       27.02.2001 the only GPU of GeForce 3 lineup [GeForce 3 Ti, ...]
02a0* NV2A 2       4         8       15.11.2001 the XBOX IGP [XGPU]
025X  NV25 2       4         8       06.02.2002 the high-end GPU of GeForce 4 lineup [GeForce 4 Ti]
028X  NV28 2       4         8       20.01.2003 like NV25, but with added AGP x8 support
===== ==== ======= ========= ======= ========== ========

NV2A is a GPU designed exclusively for the original xbox, and can't be
found anywhere else. Like NV1A and NV1F, it's an IGP.

.. todo:: verify all sorts of stuff on NV2A


Rankine family: NV30, NV35, NV31, NV36, NV34
--------------------------------------------

.. gpu-gen:: Rankine

The GPUs are:

===== ==== ======= ========= ========== ========
pciid GPU  vertex  pixel     date       notes
           shaders pipelines
                   and ROPs
===== ==== ======= ========= ========== ========
030X  NV30 2       8         27.01.2003 high-end GPU [GeForce FX 5800]
033X  NV35 3       8         12.05.2003 very high-end GPU [GeForce FX 59X0]
031X  NV31 1       4         06.03.2003 low-end GPU [GeForce FX 5600]
034X  NV36 3       4         23.10.2003 middle-end GPU [GeForce FX 5700]
032X  NV34 1       4         06.03.2003 low-end GPU [GeForce FX 5200]
===== ==== ======= ========= ========== ========

The pci vendor id is 0x10de.


Curie family
------------

.. gpu-gen:: Curie

The GPUs are:

========= ========= ============== ======= ======= ==== ========== =====
pciid     GPU id    GPU names      vertex  pixel   ROPs date       notes
                                   shaders shaders
========= ========= ============== ======= ======= ==== ========== =====
004X 021X 0x40/0x45 NV40/NV45/NV48 6       16      16   14.04.2004 AGP
00cX      0x41/0x42 NV41/NV42      5       12      12   08.11.2004
014X      0x43      NV43           3       8       4    12.08.2004
016X      0x44      NV44           3       4       2    15.12.2004 TURBOCACHE
022X      0x4a      NV44A          3       4       2    04.04.2005 AGP
009X      0x47      G70            8       24      16   22.06.2005
01dX      0x46      G72            3       4       2    18.01.2006 TURBOCACHE
029X      0x49      G71            8       24      16   09.03.2006
039X      0x4b      G73            8       12      8    09.03.2006
024X      0x4e      C51            1       2       1    20.10.2005 IGP, TURBOCACHE
03dX      0x4c      MCP61          1       2       1    ??.06.2006 IGP, TURBOCACHE
053X      0x67      MCP67          1       2       2    01.02.2006 IGP, TURBOCACHE
053X      0x68      MCP68          1       2       2    ??.07.2007 IGP, TURBOCACHE
07eX      0x63      MCP73          1       2       2    ??.07.2007 IGP, TURBOCACHE
\-        0x4d      RSX            ?       ?       ?    11.11.2006 FlexIO bus interface, used in PS3
========= ========= ============== ======= ======= ==== ========== =====

.. todo:: all geometry information unverified

.. todo:: any information on the RSX?

It's not clear how NV40 is different from NV45, or NV41 from NV42,
or MCP67 from MCP68 - they even share pciid ranges.

The NV4x IGPs actually have a memory controller as opposed to earlier ones.
This controller still accesses only host memory, though.

As execution units can be disabled on NV40+ cards, these configs are just the
maximum configs - a card can have just a subset of them enabled.


Tesla family
------------

.. gpu-gen:: Tesla

The GPUs in this family are:

===== ===== ==== =========== ==== ======= ===== ========== ======
core  hda   id   name        TPCs MPs/TPC PARTs date       notes
pciid pciid
===== ===== ==== =========== ==== ======= ===== ========== ======
019X  \-    0x50 G80         8    2       6     08.11.2006
040X  \-    0x84 G84         2    2       2     17.04.2007
042X  \-    0x86 G86         1    2       2     17.04.2007
060X+ \-    0x92 G92         8    2       4     29.10.2007
062X+ \-    0x94 G94         4    2       4     29.07.2008
064X+ \-    0x96 G96         2    2       2     29.07.2008
06eX+ \-    0x98 G98         1    1       1     04.12.2007
05eX+ \-    0xa0 G200        10   3       8     16.06.2008
084X+ \-    0xaa MCP77/MCP78 1    1       1     ??.06.2008 IGP
086X+ \-    0xac MCP79/MCP7A 1    2       1     ??.06.2008 IGP
0caX+ 0be4  0xa3 GT215       4    3       2     15.06.2009
0a2X+ 0be2  0xa5 GT216       2    3       2     15.06.2009
0a6X+ 0be3  0xa8 GT218       1    2       1     15.06.2009
08aX+ \-    0xaf MCP89       2    3       2     01.04.2010 IGP
===== ===== ==== =========== ==== ======= ===== ========== ======

Like NV40, these are just the maximal numbers.

.. todo:: geometry information not verified for G94, MCP77


Fermi/Kepler/Maxwell/Pascal/Volta/Turing family
-----------------------------------------------

.. gpu-gen:: Fermi

.. gpu-gen:: Kepler

.. gpu-gen:: Maxwell

.. gpu-gen:: Pascal

.. gpu-gen:: Volta

.. gpu-gen:: Turing

GPUs in Fermi/Kepler/Maxwell/Pascal/Volta/Turing families:

===== ===== ===== ===== ====== ==== ==== ===== === ====== ====== ===== ==== ==== ===== ====== === === === ==========
core  hda   usb   id    name   GPCs TPCs PARTs MCs ZCULLs PCOPYs HEADs UNK7 PPCs SUBPs SPOONs CE0 CE1 CE2 date
pciid pciid pciid                   /GPC           /GPC                     /GPC /PART
===== ===== ===== ===== ====== ==== ==== ===== === ====== ====== ===== ==== ==== ===== ====== === === === ==========
06cX+ 0be5  \-    0xc0  GF100  4    4    6     [6] [4]    [2]    [2]   \-   \-   2     3      0   0   \-  26.03.2010
0e2X+ 0beb  \-    0xc4  GF104  2    4    4     [4] [4]    [2]    [2]   \-   \-   2     3      0?  0?  \-  12.07.2010
120X+ 0e0c  \-    0xce  GF114  2    4    4     [4] [4]    [2]    [2]   \-   \-   2     3      0?  0?  \-  25.01.2011
0dcX+ 0be9  \-    0xc3  GF106  1    4    3     [3] [4]    [2]    [2]   \-   \-   2     3      3   4   \-  03.09.2010
124X+ 0bee  \-    0xcf  GF116  1    4    3     [3] [4]    [2]    [2]   \-   \-   2     3      3   4   \-  15.03.2011
0deX+ 0bea  \-    0xc1  GF108  1    2    1     2   4      [2]    [2]   \-   \-   2     1      3   4   \-  03.09.2010
108X+ 0e09  \-    0xc8  GF110  4    4    6     [6] [4]    [2]    [2]   \-   \-   2     3      0   0   \-  07.12.2010
104X* 0e08  \-    0xd9  GF119  1    1    1     1   4      1      2     \-   \-   1     1      3   \-  \-  05.01.2011
1140  \-    \-    0xd7  GF117  1    2    1     1   4      1      \-[4] \-   1    2     1      3   \-  \-  ??.04.2012
118X* 0e0a  \-    0xe4  GK104  4    2    4     4   4      3      4     \-   1    4     3      ?   3   3   22.03.2012
0fcX* 0e1b  \-    0xe7  GK107  1    2    2     2   4      3      4     \-   1    4     3      3   ?   3   24.04.2012
11cX+ 0e0b  \-    0xe6  GK106  3    2    3     3   4      3      4     \-   1    4     3      3   ?   3   22.04.2012
100X+ 0e1a  \-    0xf0  GK110  5    3    6     6   4      3      4     \-   2    4     3      ?   ?   ?   21.02.2013
100X+ 0e1a  \-    0xf1  GK110B 5    3    6     6   4      3      4     \-   2    4     3      ?   3   3   07.11.2013
\???? \???? \-    \???? GK210  ?    ?    ?     ?   ?      ?      ?     \-   ?    ?     ?      ?   ?   ?   ?
128X+ 0e0f  \-    0x108 GK208  1    2    1     1   4      3      4     \-   1    2     2      3   ?   3   19.02.2013
128X+ 0e0f  \-    0x106 GK208B 1    2    1     1   4      3      4     \-   1?   2?    2?     3   ?   3   ???
\-    \-    \-    0xea  GK20A  1    1    1     1   4      3      \-[4] \-   1    1     1      \-? \-? 3   ?
138X+ 0fbc  \-    0x117 GM107  1    5    2     2   4      3      4     1    2    4     2      3   ?   3   18.02.2014
134X+ \???? \-    0x118 GM108  1    3    1     1   4      3      4     0    ?    ?     2      3   ?   3   ?
17cX+ 0fb0  \-    0x120 GM200  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   ?
13cX+ 0fbb  \-    0x124 GM204  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   ?
140X+ 0fba  \-    0x126 GM206  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   ?
\-    \-    \-    0x12b GM20B  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   ?
158X# \???? \-    0x130 GP100  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   ?
1b0X# 10ef  \-    0x132 GP102  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   ?
1b8X# 10f0  \-    0x134 GP104  4    5    4     4   4      4      4     2    ?    ?     ?      ?   ?   ?   ?
1c0X# 10f1  \-    0x136 GP106  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   ?
1c8X# 0fb9  \-    0x137 GP107  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   10.25.2016
1d0X# 0fb8  \-    0x138 GP108  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   ?
10e5* \-    \-    0x13b GP10B  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   14.03.2017
1d8X# 10f2  \-    0x140 GV100  6    7    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   12.07.2017
\-    \-    \-    0x15b GV11B  ?    ?    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   03.06.2018
1e0X# 10f7  1ad6  0x162 TU102  6    6    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   27.09.2018
1e8X# 10f8  1ad8  0x164 TU104  6    4    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   20.09.2018
1f0X# 10f9  1ada  0x166 TU106  3    6    ?     ?   ?      ?      ?     ?    ?    ?     ?      ?   ?   ?   17.10.2018
===== ===== ===== ===== ====== ==== ==== ===== === ====== ====== ===== ==== ==== ===== ====== === === === ==========

.. todo:: it is said that one of the GPCs [0th one] has only one TPC on GK106

.. todo:: what the fuck is GK110B? and GK208B?

.. todo:: GK210

.. todo:: GK20A

.. todo:: GM20x, GP10x

.. todo:: another design counter available on GM107, another 4 on GP10x


Comparison table
================

.. gpu-table::
