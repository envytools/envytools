.. _pci-ids:

======================
nVidia PCI id database
======================

.. contents::


Introduction
============

nVidia uses PCI vendor id of ``0x10de``, which covers almost all of their
products. Other ids used for nVidia products include ``0x104a`` (SGS-Thompson)
and ``0x12d2`` (SGS-Thompson/nVidia joint venture). The PCI device ids with
vendor id ``0x104a`` related to nVidia are:

========== ========================================================
device id  product
========== ========================================================
``0x0008`` NV01 main function, DRAM version (SGS-Thompson branding)
``0x0009`` NV01 VGA function, DRAM version (SGS-Thompson branding)
========== ========================================================

The PCI device ids with vendor id ``0x12d2`` are:

========== ========================================================
device id  product
========== ========================================================
``0x0018`` NV03 [RIVA 128]
``0x0019`` NV03T [RIVA 128 ZX]
========== ========================================================

All other nVidia PCI devices use vendor id ``0x10de``. This includes:

- GPUs
- motherboard chipsets
- BR03 and NF200 PCIE switches
- the BR02 transparent AGP/PCIE bridge
- GVI, the SDI input card

The PCI device ids with vendor id ``0x10de`` are:

================= ========================================================
device id         product
================= ========================================================
``0x0008``        NV01 main function, VRAM version (nVidia branding)
``0x0009``        NV01 VGA function, VRAM version (nVidia branding)
``0x0020``        NV04 [RIVA TNT]
``0x0028-0x002f`` :ref:`NV05 <pci-ids-nv05>`
``0x0030-0x003f`` :ref:`MCP04 <pci-ids-mcp04>`
``0x0040-0x004f`` :ref:`NV40 <pci-ids-nv40>`
``0x0050-0x005f`` :ref:`CK804 <pci-ids-ck804>`
``0x0060-0x006e`` :ref:`MCP2 <pci-ids-mcp2>`
``0x006f-0x007f`` :ref:`C19 <pci-ids-c19>`
``0x0080-0x008f`` :ref:`MCP2A <pci-ids-mcp2a>`
``0x0090-0x009f`` :ref:`G70 <pci-ids-g70>`
``0x00c0-0x00cf`` :ref:`NV41/NV42 <pci-ids-nv41>`
``0x00a0``        NV0A [Aladdin TNT2]
``0x00b0``        :ref:`NV18 Firewire <pci-ids-nv18>`
``0x00b4``        :ref:`C19 <pci-ids-c19>`
``0x00d0-0x00d2`` :ref:`CK8 <pci-ids-ck8>`
``0x00d3``        :ref:`CK804 <pci-ids-ck804>`
``0x00d4-0x00dd`` :ref:`CK8 <pci-ids-ck8>`
``0x00df-0x00ef`` :ref:`CK8S <pci-ids-ck8s>`
``0x00f0-0x00ff`` :ref:`BR02 <pci-ids-br02>`
``0x0100-0x0103`` :ref:`NV10 <pci-ids-nv10>`
``0x0110-0x0113`` :ref:`NV11 <pci-ids-nv11>`
``0x0140-0x014f`` :ref:`NV43 <pci-ids-nv43>`
``0x0150-0x0153`` :ref:`NV15 <pci-ids-nv15>`
``0x0160-0x016f`` :ref:`NV44 <pci-ids-nv44>`
``0x0170-0x017f`` :ref:`NV17 <pci-ids-nv17>`
``0x0180-0x018f`` :ref:`NV18 <pci-ids-nv18>`
``0x0190-0x019f`` :ref:`G80 <pci-ids-g80>`
``0x01a0-0x01af`` :ref:`NV1A <pci-ids-nv1a>`
``0x01b0-0x01b2`` :ref:`MCP <pci-ids-mcp>`
``0x01b3``        :ref:`BR03 <pci-ids-br03>`
``0x01b4-0x01b2`` :ref:`MCP <pci-ids-mcp>`
``0x01b7``        :ref:`NV1A <pci-ids-nv1a>`, :ref:`NV2A <pci-ids-nv2a>`
``0x01b8-0x01cf`` :ref:`MCP <pci-ids-mcp>`
``0x01d0-0x01df`` :ref:`G72 <pci-ids-g72>`
``0x01e0-0x01f0`` :ref:`NV1F <pci-ids-nv1f>`
``0x01f0-0x01ff`` :ref:`NV1F GPU <pci-ids-nv1f-gpu>`
``0x0200-0x0203`` :ref:`NV20 <pci-ids-nv20>`
``0x0210-0x021f`` :ref:`NV40 <pci-ids-nv40>`?
``0x0220-0x022f`` :ref:`NV44A <pci-ids-nv44a>`
``0x0240-0x024f`` :ref:`C51 GPU <pci-ids-c51-gpu>`
``0x0250-0x025f`` :ref:`NV25 <pci-ids-nv25>`
``0x0260-0x0272`` :ref:`MCP51 <pci-ids-mcp51>`
``0x027e-0x027f`` :ref:`C51 <pci-ids-c51>`
``0x0280-0x028f`` :ref:`NV28 <pci-ids-nv28>`
``0x0290-0x029f`` :ref:`G71 <pci-ids-g71>`
``0x02a0-0x02af`` :ref:`NV2A <pci-ids-nv2a>`
``0x02e0-0x02ef`` :ref:`BR02 <pci-ids-br02>`
``0x02f0-0x02ff`` :ref:`C51 <pci-ids-c51>`
``0x0300-0x030f`` :ref:`NV30 <pci-ids-nv30>`
``0x0310-0x031f`` :ref:`NV31 <pci-ids-nv31>`
``0x0320-0x032f`` :ref:`NV34 <pci-ids-nv34>`
``0x0330-0x033f`` :ref:`NV35 <pci-ids-nv35>`
``0x0340-0x034f`` :ref:`NV36 <pci-ids-nv36>`
``0x0360-0x037f`` :ref:`MCP55 <pci-ids-mcp55>`
``0x0390-0x039f`` :ref:`G73 <pci-ids-g73>`
``0x03a0-0x03bc`` :ref:`C55 <pci-ids-c55>`
``0x03d0-0x03df`` :ref:`MCP61 GPU <pci-ids-mcp61-gpu>`
``0x03e0-0x03f7`` :ref:`MCP61 <pci-ids-mcp61>`
``0x0400-0x040f`` :ref:`G84 <pci-ids-g84>`
``0x0410-0x041f`` :ref:`G92 <pci-ids-g92>` extra IDs
``0x0420-0x042f`` :ref:`G86 <pci-ids-g86>`
``0x0440-0x045f`` :ref:`MCP65 <pci-ids-mcp65>`
``0x0530-0x053f`` :ref:`MCP67 GPU <pci-ids-mcp67-gpu>`
``0x0540-0x0563`` :ref:`MCP67 <pci-ids-mcp67>`
``0x0568-0x0569`` :ref:`MCP77 <pci-ids-mcp77>`
``0x056a-0x056f`` :ref:`MCP73 <pci-ids-mcp73>`
``0x0570-0x057f`` MCP* ethernet alt ID
``0x0580-0x058f`` MCP* SATA alt ID
``0x0590-0x059f`` MCP* HDA alt ID
``0x05a0-0x05af`` MCP* IDE alt ID
``0x05b0-0x05bf`` :ref:`BR04 <pci-ids-br04>`
``0x05e0-0x05ff`` :ref:`G200 <pci-ids-g200>`
``0x0600-0x061f`` :ref:`G92 <pci-ids-g92>`
``0x0620-0x063f`` :ref:`G94 <pci-ids-g94>`
``0x0640-0x065f`` :ref:`G96 <pci-ids-g96>`
``0x06c0-0x06df`` :ref:`GF100 <pci-ids-gf100>`
``0x06e0-0x06ff`` :ref:`G98 <pci-ids-g98>`
``0x0750-0x077f`` :ref:`MCP77 <pci-ids-mcp77>`
``0x07c0-0x07df`` :ref:`MCP73 <pci-ids-mcp73>`
``0x07e0-0x07ef`` :ref:`MCP73 GPU <pci-ids-mcp73-gpu>`
``0x07f0-0x07fe`` :ref:`MCP73 <pci-ids-mcp73>`
``0x0800-0x081a`` :ref:`C73 <pci-ids-c73>`
``0x0840-0x085f`` :ref:`MCP77 GPU <pci-ids-mcp77-gpu>`
``0x0860-0x087f`` :ref:`MCP79 GPU <pci-ids-mcp79-gpu>`
``0x08a0-0x08bf`` :ref:`MCP89 GPU <pci-ids-mcp89-gpu>`
``0x0a20-0x0a3f`` :ref:`GT216 <pci-ids-gt216>`
``0x0a60-0x0a7f`` :ref:`GT218 <pci-ids-gt218>`
``0x0a80-0x0ac8`` :ref:`MCP79 <pci-ids-mcp79>`
``0x0ad0-0x0adb`` :ref:`MCP77 <pci-ids-mcp77>`
``0x0be0-0x0bef`` :ref:`GPU HDA <pci-ids-gpu-hda>`
``0x0bf0-0x0bf1`` :ref:`T20 <pci-ids-t20>`
``0x0ca0-0x0cbf`` :ref:`GT215 <pci-ids-gt215>`
``0x0d60-0x0d9d`` :ref:`MCP89 <pci-ids-mcp89>`
``0x0dc0-0x0ddf`` :ref:`GF106 <pci-ids-gf106>`
``0x0de0-0x0dff`` :ref:`GF108 <pci-ids-gf108>`
``0x0e00``        GVI SDI input
``0x0e01-0x0e1b`` :ref:`GPU HDA <pci-ids-gpu-hda>`
``0x0e1c-0x0e1d`` :ref:`T30 <pci-ids-t30>`
``0x0e20-0x0e3f`` :ref:`GF104 <pci-ids-gf104>`
``0x0f00-0x0f1f`` :ref:`GF108 <pci-ids-gf108>` extra IDs
``0x0fb0-0x0fbf`` :ref:`GPU HDA <pci-ids-gpu-hda>`
``0x0fc0-0x0fff`` :ref:`GK107 <pci-ids-gk107>`
``0x1000-0x103f`` :ref:`GK110/GK110B <pci-ids-gk110>`
``0x1040-0x107f`` :ref:`GF119 <pci-ids-gf119>`
``0x1080-0x109f`` :ref:`GF110 <pci-ids-gf110>`
``0x10c0-0x10df`` :ref:`GT218 <pci-ids-gt218>` extra IDs
``0x1140-0x117f`` :ref:`GF117 <pci-ids-gf117>`
``0x1180-0x11bf`` :ref:`GK104 <pci-ids-gk104>`
``0x11c0-0x11ff`` :ref:`GK106 <pci-ids-gk106>`
``0x1200-0x121f`` :ref:`GF114 <pci-ids-gf114>`
``0x1240-0x125f`` :ref:`GF116 <pci-ids-gf116>`
``0x1280-0x12bf`` :ref:`GK208 <pci-ids-gk208>`
``0x1380-0x13bf`` :ref:`GM107 <pci-ids-gm107>`
================= ========================================================



GPUs
====


.. _pci-ids-nv05:

NV05
----

========== ========================================================
device id  product
========== ========================================================
``0x0028`` NV05 [RIVA TNT2]
``0x0029`` NV05 [RIVA TNT2 Ultra]
``0x002c`` NV05 [Vanta]
``0x002d`` NV05 [RIVA TNT2 Model 64]
========== ========================================================


.. _pci-ids-nv10:

NV10
----

========== ========================================================
device id  product
========== ========================================================
``0x0100`` NV10 [GeForce 256 SDR]
``0x0101`` NV10 [GeForce 256 DDR]
``0x0102`` NV10 [GeForce 256 Ultra]
``0x0103`` NV10 [Quadro]
========== ========================================================


.. _pci-ids-nv15:

NV15
----

========== ========================================================
device id  product
========== ========================================================
``0x0150`` NV15 [GeForce2 GTS/Pro]
``0x0151`` NV15 [GeForce2 Ti]
``0x0152`` NV15 [GeForce2 Ultra]
``0x0153`` NV15 [Quadro2 Pro]
========== ========================================================


.. _pci-ids-nv11:

NV11
----

========== ========================================================
device id  product
========== ========================================================
``0x0110`` NV11 [GeForce2 MX/MX 400]
``0x0111`` NV11 [GeForce2 MX 100/200]
``0x0112`` NV11 [GeForce2 Go]
``0x0113`` NV11 [Quadro2 MXR/EX/Go]
========== ========================================================


.. _pci-ids-nv20:

NV20
----

========== ========================================================
device id  product
========== ========================================================
``0x0200`` NV20 [GeForce3]
``0x0201`` NV20 [GeForce3 Ti 200]
``0x0202`` NV20 [GeForce3 Ti 500]
``0x0203`` NV20 [Quadro DCC]
========== ========================================================


.. _pci-ids-nv17:

NV17
----

========== ========================================================
device id  product
========== ========================================================
``0x0170`` NV17 [GeForce4 MX 460]
``0x0171`` NV17 [GeForce4 MX 440]
``0x0172`` NV17 [GeForce4 MX 420]
``0x0173`` NV17 [GeForce4 MX 440-SE]
``0x0174`` NV17 [GeForce4 440 Go]
``0x0175`` NV17 [GeForce4 420 Go]
``0x0176`` NV17 [GeForce4 420 Go 32M]
``0x0177`` NV17 [GeForce4 460 Go]
``0x0178`` NV17 [Quadro4 550 XGL]
``0x0179`` NV17 [GeForce4 440 Go 64M]
``0x017a`` NV17 [Quadro NVS 100/200/400]
``0x017b`` NV17 [Quadro4 550 XGL]???
``0x017c`` NV17 [Quadro4 500 GoGL]
``0x017d`` NV17 [GeForce4 410 Go 16M]
========== ========================================================


.. _pci-ids-nv18:

NV18
----

========== ========================================================
device id  product
========== ========================================================
``0x0181`` NV18 [GeForce4 MX 440 AGP 8x]
``0x0182`` NV18 [GeForce4 MX 440-SE AGP 8x]
``0x0183`` NV18 [GeForce4 MX 420 AGP 8x]
``0x0185`` NV18 [GeForce4 MX 4000]
``0x0186`` NV18 [GeForce4 448 Go]
``0x0187`` NV18 [GeForce4 488 Go]
``0x0188`` NV18 [Quadro4 580 XGL]
``0x0189`` NV18 [GeForce4 MX AGP 8x (Mac)]
``0x018a`` NV18 [Quadro NVS 280 SD]
``0x018b`` NV18 [Quadro4 380 XGL]
``0x018c`` NV18 [Quadro NVS 50 PCI]
``0x018d`` NV18 [GeForce4 448 Go]
``0x00b0`` NV18 Firewire controller
========== ========================================================


.. _pci-ids-nv1f-gpu:

NV1F (GPU)
----------

========== ========================================================
device id  product
========== ========================================================
``0x01f0`` NV1F GPU [GeForce4 MX IGP]
========== ========================================================


.. _pci-ids-nv25:

NV25
----

========== ========================================================
device id  product
========== ========================================================
``0x0250`` NV25 [GeForce4 Ti 4600]
``0x0251`` NV25 [GeForce4 Ti 4400]
``0x0252`` NV25 [GeForce4 Ti]
``0x0253`` NV25 [GeForce4 Ti 4200]
``0x0258`` NV25 [Quadro4 900 XGL]
``0x0259`` NV25 [Quadro4 750 XGL]
``0x025b`` NV25 [Quadro4 700 XGL]
========== ========================================================


.. _pci-ids-nv28:

NV28
----

========== ========================================================
device id  product
========== ========================================================
``0x0280`` NV28 [GeForce4 Ti 4800]
``0x0281`` NV28 [GeForce4 Ti 4200 AGP 8x]
``0x0282`` NV28 [GeForce4 Ti 4800 SE]
``0x0286`` NV28 [GeForce4 Ti 4200 Go]
``0x0288`` NV28 [Quadro4 980 XGL]
``0x0289`` NV28 [Quadro4 780 XGL]
``0x028c`` NV28 [Quadro4 700 GoGL]
========== ========================================================


.. _pci-ids-nv30:

NV30
----

========== ========================================================
device id  product
========== ========================================================
``0x0301`` NV30 [GeForce FX 5800 Ultra]
``0x0302`` NV30 [GeForce FX 5800]
``0x0308`` NV35 [Quadro FX 2000]
``0x0309`` NV35 [Quadro FX 1000]
========== ========================================================


.. _pci-ids-nv31:

NV31
----

========== ========================================================
device id  product
========== ========================================================
``0x0311`` NV31 [GeForce FX 5600 Ultra]
``0x0312`` NV31 [GeForce FX 5600]
``0x0314`` NV31 [GeForce FX 5600XT]
``0x031a`` NV31 [GeForce FX Go5600]
``0x031b`` NV31 [GeForce FX Go5650]
``0x031c`` NV31 [GeForce FX Go700]
========== ========================================================


.. _pci-ids-nv34:

NV34
----

========== ========================================================
device id  product
========== ========================================================
``0x0320`` NV34 [GeForce FX 5200]
``0x0321`` NV34 [GeForce FX 5200 Ultra]
``0x0322`` NV34 [GeForce FX 5200]
``0x0323`` NV34 [GeForce FX 5200LE]
``0x0324`` NV34 [GeForce FX Go5200]
``0x0325`` NV34 [GeForce FX Go5250]
``0x0326`` NV34 [GeForce FX 5500]
``0x0327`` NV34 [GeForce FX 5100]
``0x0328`` NV34 [GeForce FX Go5200 32M/64M]
``0x0329`` NV34 [GeForce FX Go5200 (Mac)]
``0x032a`` NV34 [Quadro NVS 280 PCI]
``0x032b`` NV34 [Quadro FX 500/FX 600]
``0x032c`` NV34 [GeForce FX Go5300/Go5350]
``0x032d`` NV34 [GeForce FX Go5100]
========== ========================================================


.. _pci-ids-nv35:

NV35
----

========== ========================================================
device id  product
========== ========================================================
``0x0330`` NV35 [GeForce FX 5900 Ultra]
``0x0331`` NV35 [GeForce FX 5900]
``0x0332`` NV35 [GeForce FX 5900XT]
``0x0333`` NV35 [GeForce FX 5950 Ultra]
``0x0334`` NV35 [GeForce FX 5900ZT]
``0x0338`` NV35 [Quadro FX 3000]
``0x033f`` NV35 [Quadro FX 700]
========== ========================================================


.. _pci-ids-nv36:

NV36
----

========== ========================================================
device id  product
========== ========================================================
``0x0341`` NV36 [GeForce FX 5700 Ultra]
``0x0342`` NV36 [GeForce FX 5700]
``0x0343`` NV36 [GeForce FX 5700LE]
``0x0344`` NV36 [GeForce FX 5700VE]
``0x0347`` NV36 [GeForce FX Go5700]
``0x0348`` NV36 [GeForce FX Go5700]
``0x034c`` NV36 [Quadro FX Go1000]
``0x034e`` NV36 [Quadro FX 1100]
========== ========================================================


.. _pci-ids-nv40:

NV40
----

========== ========================================================
device id  product
========== ========================================================
``0x0040`` NV40 [GeForce 6800 Ultra]
``0x0041`` NV40 [GeForce 6800]
``0x0042`` NV40 [GeForce 6800 LE]
``0x0043`` NV40 [GeForce 6800 XE]
``0x0044`` NV40 [GeForce 6800 XT]
``0x0045`` NV40 [GeForce 6800 GT]
``0x0046`` NV40 [GeForce 6800 GT]
``0x0047`` NV40 [GeForce 6800 GS]
``0x0048`` NV40 [GeForce 6800 XT]
``0x004e`` NV40 [Quadro FX 4000]
``0x0211`` NV40? [GeForce 6800]
``0x0212`` NV40? [GeForce 6800 LE]
``0x0215`` NV40? [GeForce 6800 GT]
``0x0218`` NV40? [GeForce 6800 XT]
========== ========================================================

.. todo:: wtf is with that 0x21x ID?


.. _pci-ids-nv41:

NV41/NV42
---------

========== ========================================================
device id  product
========== ========================================================
``0x00c0`` NV41/NV42 [GeForce 6800 GS]
``0x00c1`` NV41/NV42 [GeForce 6800]
``0x00c2`` NV41/NV42 [GeForce 6800 LE]
``0x00c3`` NV41/NV42 [GeForce 6800 XT]
``0x00c8`` NV41/NV42 [GeForce Go 6800]
``0x00c9`` NV41/NV42 [GeForce Go 6800 Ultra]
``0x00cc`` NV41/NV42 [Quadro FX Go1400]
``0x00cd`` NV41/NV42 [Quadro FX 3450/4000 SDI]
``0x00ce`` NV41/NV42 [Quadro FX 1400]
========== ========================================================


.. _pci-ids-nv43:

NV43
----

========== ========================================================
device id  product
========== ========================================================
``0x0140`` NV43 [GeForce 6600 GT]
``0x0141`` NV43 [GeForce 6600]
``0x0142`` NV43 [GeForce 6600 LE]
``0x0143`` NV43 [GeForce 6600 VE]
``0x0144`` NV43 [GeForce Go 6600]
``0x0145`` NV43 [GeForce 6610 XL]
``0x0146`` NV43 [GeForce Go 6200 TE / 6660 TE]
``0x0147`` NV43 [GeForce 6700 XL]
``0x0148`` NV43 [GeForce Go 6600]
``0x0149`` NV43 [GeForce Go 6600 GT]
``0x014a`` NV43 [Quadro NVS 440]
``0x014c`` NV43 [Quadro FX 540M]
``0x014d`` NV43 [Quadro FX 550]
``0x014e`` NV43 [Quadro FX 540]
``0x014f`` NV43 [GeForce 6200]
========== ========================================================


.. _pci-ids-nv44:

NV44
----

========== ========================================================
device id  product
========== ========================================================
``0x0160`` NV44 [GeForce 6500]
``0x0161`` NV44 [GeForce 6200 TurboCache]
``0x0162`` NV44 [GeForce 6200 SE TurboCache]
``0x0163`` NV44 [GeForce 6200 LE]
``0x0164`` NV44 [GeForce Go 6200]
``0x0165`` NV44 [Quadro NVS 285]
``0x0166`` NV44 [GeForce Go 6400]
``0x0167`` NV44 [GeForce Go 6200]
``0x0168`` NV44 [GeForce Go 6400]
``0x0169`` NV44 [GeForce 6250]
``0x016a`` NV44 [GeForce 7100 GS]
========== ========================================================


.. _pci-ids-nv44a:

NV44A [NV4A]
------------

========== ========================================================
device id  product
========== ========================================================
``0x0221`` NV44A [GeForce 6200 (AGP)]
``0x0222`` NV44A [GeForce 6200 A-LE (AGP)]
========== ========================================================


.. _pci-ids-c51-gpu:

C51 GPU [NV4E]
--------------

========== ========================================================
device id  product
========== ========================================================
``0x0240`` C51 GPU [GeForce 6150]
``0x0241`` C51 GPU [GeForce 6150 LE]
``0x0242`` C51 GPU [GeForce 6100]
``0x0244`` C51 GPU [GeForce Go 6150]
``0x0245`` C51 GPU [Quadro NVS 210S / NVIDIA GeForce 6150LE]
``0x0247`` C51 GPU [GeForce Go 6100]
========== ========================================================


.. _pci-ids-g70:

G70 [NV47]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0090`` G70 [GeForce 7800 GTX]
``0x0091`` G70 [GeForce 7800 GTX]
``0x0092`` G70 [GeForce 7800 GT]
``0x0093`` G70 [GeForce 7800 GS]
``0x0095`` G70 [GeForce 7800 SLI]
``0x0098`` G70 [GeForce Go 7800]
``0x0099`` G70 [GeForce Go 7800 GTX]
``0x009d`` G70 [Quadro FX 4500]
========== ========================================================


.. _pci-ids-g72:

G72 [NV46]
----------

========== ========================================================
device id  product
========== ========================================================
``0x01d0`` G72 [GeForce 7350 LE]
``0x01d1`` G72 [GeForce 7300 LE]
``0x01d2`` G72 [GeForce 7550 LE]
``0x01d3`` G72 [GeForce 7300 SE/7200 GS]
``0x01d6`` G72 [GeForce Go 7200]
``0x01d7`` G72 [Quadro NVS 110M / GeForce Go 7300]
``0x01d8`` G72 [GeForce Go 7400]
``0x01d9`` G72 [GeForce Go 7450]
``0x01da`` G72 [Quadro NVS 110M]
``0x01db`` G72 [Quadro NVS 120M]
``0x01dc`` G72 [Quadro FX 350M]
``0x01dd`` G72 [GeForce 7500 LE]
``0x01de`` G72 [Quadro FX 350]
``0x01df`` G72 [GeForce 7300 GS]
========== ========================================================


.. _pci-ids-g71:

G71 [NV49]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0290`` G71 [GeForce 7900 GTX]
``0x0291`` G71 [GeForce 7900 GT/GTO]
``0x0292`` G71 [GeForce 7900 GS]
``0x0293`` G71 [GeForce 7900 GX2]
``0x0294`` G71 [GeForce 7950 GX2]
``0x0295`` G71 [GeForce 7950 GT]
``0x0297`` G71 [GeForce Go 7950 GTX]
``0x0298`` G71 [GeForce Go 7900 GS]
``0x0299`` G71 [GeForce Go 7900 GTX]
``0x029a`` G71 [Quadro FX 2500M]
``0x029b`` G71 [Quadro FX 1500M]
``0x029c`` G71 [Quadro FX 5500]
``0x029d`` G71 [Quadro FX 3500]
``0x029e`` G71 [Quadro FX 1500]
``0x029f`` G71 [Quadro FX 4500 X2]
========== ========================================================


.. _pci-ids-g73:

G73 [NV4B]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0390`` G73 [GeForce 7650 GS]
``0x0391`` G73 [GeForce 7600 GT]
``0x0392`` G73 [GeForce 7600 GS]
``0x0393`` G73 [GeForce 7300 GT]
``0x0394`` G73 [GeForce 7600 LE]
``0x0395`` G73 [GeForce 7300 GT]
``0x0397`` G73 [GeForce Go 7700]
``0x0398`` G73 [GeForce Go 7600]
``0x0399`` G73 [GeForce Go 7600 GT]
``0x039a`` G73 [Quadro NVS 300M]
``0x039b`` G73 [GeForce Go 7900 SE]
``0x039c`` G73 [Quadro FX 560M]
``0x039e`` G73 [Quadro FX 560]
========== ========================================================


.. _pci-ids-mcp61-gpu:

MCP61 GPU [NV4C]
----------------

========== ========================================================
device id  product
========== ========================================================
``0x03d0`` MCP61 GPU [GeForce 6150SE nForce 430]
``0x03d1`` MCP61 GPU [GeForce 6100 nForce 405]
``0x03d2`` MCP61 GPU [GeForce 6100 nForce 400]
``0x03d5`` MCP61 GPU [GeForce 6100 nForce 420]
``0x03d6`` MCP61 GPU [GeForce 7025 / nForce 630a]
========== ========================================================


.. _pci-ids-mcp67-gpu:

MCP67 GPU [NV67/NV68]
---------------------

========== ========================================================
device id  product
========== ========================================================
``0x0531`` MCP67 GPU [GeForce 7150M / nForce 630M]
``0x0533`` MCP67 GPU [GeForce 7000M / nForce 610M]
``0x053a`` MCP67 GPU [GeForce 7050 PV / nForce 630a]
``0x053b`` MCP67 GPU [GeForce 7050 PV / nForce 630a]
``0x053e`` MCP67 GPU [GeForce 7025 / nForce 630a]
========== ========================================================

.. note:: mobile is apparently considered to be MCP67, desktop MCP68


.. _pci-ids-mcp73-gpu:

MCP73 GPU [NV63]
----------------

========== ========================================================
device id  product
========== ========================================================
``0x07e0`` MCP73 GPU [GeForce 7150 / nForce 630i]
``0x07e1`` MCP73 GPU [GeForce 7100 / nForce 630i]
``0x07e2`` MCP73 GPU [GeForce 7050 / nForce 630i]
``0x07e3`` MCP73 GPU [GeForce 7050 / nForce 610i]
``0x07e5`` MCP73 GPU [GeForce 7050 / nForce 620i]
========== ========================================================


.. _pci-ids-g80:

G80 [NV50]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0191`` G80 [GeForce 8800 GTX]
``0x0193`` G80 [GeForce 8800 GTS]
``0x0194`` G80 [GeForce 8800 Ultra]
``0x0197`` G80 [Tesla C870]
``0x019d`` G80 [Quadro FX 5600]
``0x019e`` G80 [Quadro FX 4600]
========== ========================================================


.. _pci-ids-g84:

G84 [NV84]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0400`` G84 [GeForce 8600 GTS]
``0x0401`` G84 [GeForce 8600 GT]
``0x0402`` G84 [GeForce 8600 GT]
``0x0403`` G84 [GeForce 8600 GS]
``0x0404`` G84 [GeForce 8400 GS]
``0x0405`` G84 [GeForce 9500M GS]
``0x0406`` G84 [GeForce 8300 GS]
``0x0407`` G84 [GeForce 8600M GT]
``0x0408`` G84 [GeForce 9650M GS]
``0x0409`` G84 [GeForce 8700M GT]
``0x040a`` G84 [Quadro FX 370]
``0x040b`` G84 [Quadro NVS 320M]
``0x040c`` G84 [Quadro FX 570M]
``0x040d`` G84 [Quadro FX 1600M]
``0x040e`` G84 [Quadro FX 570]
``0x040f`` G84 [Quadro FX 1700]
========== ========================================================


.. _pci-ids-g86:

G86 [NV86]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0420`` G86 [GeForce 8400 SE]
``0x0421`` G86 [GeForce 8500 GT]
``0x0422`` G86 [GeForce 8400 GS]
``0x0423`` G86 [GeForce 8300 GS]
``0x0424`` G86 [GeForce 8400 GS]
``0x0425`` G86 [GeForce 8600M GS]
``0x0426`` G86 [GeForce 8400M GT]
``0x0427`` G86 [GeForce 8400M GS]
``0x0428`` G86 [GeForce 8400M G]
``0x0429`` G86 [Quadro NVS 140M]
``0x042a`` G86 [Quadro NVS 130M]
``0x042b`` G86 [Quadro NVS 135M]
``0x042c`` G86 [GeForce 9400 GT]
``0x042d`` G86 [Quadro FX 360M]
``0x042e`` G86 [GeForce 9300M G]
``0x042f`` G86 [Quadro NVS 290]
========== ========================================================


.. _pci-ids-g92:

G92 [NV92]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0410`` G92 [GeForce GT 330]
``0x0600`` G92 [GeForce 8800 GTS 512]
``0x0601`` G92 [GeForce 9800 GT]
``0x0602`` G92 [GeForce 8800 GT]
``0x0603`` G92 [GeForce GT 230]
``0x0604`` G92 [GeForce 9800 GX2]
``0x0605`` G92 [GeForce 9800 GT]
``0x0606`` G92 [GeForce 8800 GS]
``0x0607`` G92 [GeForce GTS 240]
``0x0608`` G92 [GeForce 9800M GTX]
``0x0609`` G92 [GeForce 8800M GTS]
``0x060a`` G92 [GeForce GTX 280M]
``0x060b`` G92 [GeForce 9800M GT]
``0x060c`` G92 [GeForce 8800M GTX]
``0x060f`` G92 [GeForce GTX 285M]
``0x0610`` G92 [GeForce 9600 GSO]
``0x0611`` G92 [GeForce 8800 GT]
``0x0612`` G92 [GeForce 9800 GTX/9800 GTX+]
``0x0613`` G92 [GeForce 9800 GTX+]
``0x0614`` G92 [GeForce 9800 GT]
``0x0615`` G92 [GeForce GTS 250]
``0x0617`` G92 [GeForce 9800M GTX]
``0x0618`` G92 [GeForce GTX 260M]
``0x0619`` G92 [Quadro FX 4700 X2]
``0x061a`` G92 [Quadro FX 3700]
``0x061b`` G92 [Quadro VX 200]
``0x061c`` G92 [Quadro FX 3600M]
``0x061d`` G92 [Quadro FX 2800M]
``0x061e`` G92 [Quadro FX 3700M]
``0x061f`` G92 [Quadro FX 3800M]
========== ========================================================


.. _pci-ids-g94:

G94 [NV94]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0621`` G94 [GeForce GT 230]
``0x0622`` G94 [GeForce 9600 GT]
``0x0623`` G94 [GeForce 9600 GS]
``0x0625`` G94 [GeForce 9600 GSO 512]
``0x0626`` G94 [GeForce GT 130]
``0x0627`` G94 [GeForce GT 140]
``0x0628`` G94 [GeForce 9800M GTS]
``0x062a`` G94 [GeForce 9700M GTS]
``0x062b`` G94 [GeForce 9800M GS]
``0x062c`` G94 [GeForce 9800M GTS    ]
``0x062d`` G94 [GeForce 9600 GT]
``0x062e`` G94 [GeForce 9600 GT]
``0x0631`` G94 [GeForce GTS 160M]
``0x0635`` G94 [GeForce 9600 GSO]
``0x0637`` G94 [GeForce 9600 GT]
``0x0638`` G94 [Quadro FX 1800]
``0x063a`` G94 [Quadro FX 2700M]
========== ========================================================


.. _pci-ids-g96:

G96 [NV96]
----------

========== ========================================================
device id  product
========== ========================================================
``0x0640`` G96 [GeForce 9500 GT]
``0x0641`` G96 [GeForce 9400 GT]
``0x0643`` G96 [GeForce 9500 GT]
``0x0644`` G96 [GeForce 9500 GS]
``0x0645`` G96 [GeForce 9500 GS]
``0x0646`` G96 [GeForce GT 120]
``0x0647`` G96 [GeForce 9600M GT]
``0x0648`` G96 [GeForce 9600M GS]
``0x0649`` G96 [GeForce 9600M GT]
``0x064a`` G96 [GeForce 9700M GT]
``0x064b`` G96 [GeForce 9500M G]
``0x064c`` G96 [GeForce 9650M GT]
``0x0651`` G96 [GeForce G 110M]
``0x0652`` G96 [GeForce GT 130M]
``0x0653`` G96 [GeForce GT 120M]
``0x0654`` G96 [GeForce GT 220M]
``0x0655`` G96 [GeForce GT 120]
``0x0656`` G96 [GeForce GT 120 ]
``0x0658`` G96 [Quadro FX 380]
``0x0659`` G96 [Quadro FX 580]
``0x065a`` G96 [Quadro FX 1700M]
``0x065b`` G96 [GeForce 9400 GT]
``0x065c`` G96 [Quadro FX 770M]
``0x065f`` G96 [GeForce G210]
========== ========================================================


.. _pci-ids-g98:

G98 [NV98]
----------

========== ========================================================
device id  product
========== ========================================================
``0x06e0`` G98 [GeForce 9300 GE]
``0x06e1`` G98 [GeForce 9300 GS]
``0x06e2`` G98 [GeForce 8400]
``0x06e3`` G98 [GeForce 8400 SE]
``0x06e4`` G98 [GeForce 8400 GS]
``0x06e6`` G98 [GeForce G100]
``0x06e7`` G98 [GeForce 9300 SE]
``0x06e8`` G98 [GeForce 9200M GS]
``0x06e9`` G98 [GeForce 9300M GS]
``0x06ea`` G98 [Quadro NVS 150M]
``0x06eb`` G98 [Quadro NVS 160M]
``0x06ec`` G98 [GeForce G 105M]
``0x06ef`` G98 [GeForce G 103M]
``0x06f1`` G98 [GeForce G105M]
``0x06f8`` G98 [Quadro NVS 420]
``0x06f9`` G98 [Quadro FX 370 LP]
``0x06fa`` G98 [Quadro NVS 450]
``0x06fb`` G98 [Quadro FX 370M]
``0x06fd`` G98 [Quadro NVS 295]
``0x06ff`` G98 [HICx16 + Graphics]
========== ========================================================


.. _pci-ids-g200:

G200 [NVA0]
-----------

========== ========================================================
device id  product
========== ========================================================
``0x05e0`` G200 [GeForce GTX 295]
``0x05e1`` G200 [GeForce GTX 280]
``0x05e2`` G200 [GeForce GTX 260]
``0x05e3`` G200 [GeForce GTX 285]
``0x05e6`` G200 [GeForce GTX 275]
``0x05e7`` G200 [Tesla C1060]
``0x05e9`` G200 [Quadro CX]
``0x05ea`` G200 [GeForce GTX 260]
``0x05eb`` G200 [GeForce GTX 295]
``0x05ed`` G200 [Quadro FX 5800]
``0x05ee`` G200 [Quadro FX 4800]
``0x05ef`` G200 [Quadro FX 3800]
========== ========================================================


.. _pci-ids-mcp77-gpu:

MCP77 GPU [NVAA]
----------------

========== ========================================================
device id  product
========== ========================================================
``0x0840`` MCP77 GPU [GeForce 8200M]
``0x0844`` MCP77 GPU [GeForce 9100M G]
``0x0845`` MCP77 GPU [GeForce 8200M G]
``0x0846`` MCP77 GPU [GeForce 9200]
``0x0847`` MCP77 GPU [GeForce 9100]
``0x0848`` MCP77 GPU [GeForce 8300]
``0x0849`` MCP77 GPU [GeForce 8200]
``0x084a`` MCP77 GPU [nForce 730a]
``0x084b`` MCP77 GPU [GeForce 9200]
``0x084c`` MCP77 GPU [nForce 980a/780a SLI]
``0x084d`` MCP77 GPU [nForce 750a SLI]
``0x084f`` MCP77 GPU [GeForce 8100 / nForce 720a]
========== ========================================================


.. _pci-ids-mcp79-gpu:

MCP79 GPU [NVAC]
----------------

========== ========================================================
device id  product
========== ========================================================
``0x0860`` MCP79 GPU [GeForce 9400]
``0x0861`` MCP79 GPU [GeForce 9400]
``0x0862`` MCP79 GPU [GeForce 9400M G]
``0x0863`` MCP79 GPU [GeForce 9400M]
``0x0864`` MCP79 GPU [GeForce 9300]
``0x0865`` MCP79 GPU [ION]
``0x0866`` MCP79 GPU [GeForce 9400M G]
``0x0867`` MCP79 GPU [GeForce 9400]
``0x0868`` MCP79 GPU [nForce 760i SLI]
``0x0869`` MCP79 GPU [GeForce 9400]
``0x086a`` MCP79 GPU [GeForce 9400]
``0x086c`` MCP79 GPU [GeForce 9300 / nForce 730i]
``0x086d`` MCP79 GPU [GeForce 9200]
``0x086e`` MCP79 GPU [GeForce 9100M G]
``0x086f`` MCP79 GPU [GeForce 8200M G]
``0x0870`` MCP79 GPU [GeForce 9400M]
``0x0871`` MCP79 GPU [GeForce 9200]
``0x0872`` MCP79 GPU [GeForce G102M]
``0x0873`` MCP79 GPU [GeForce G102M]
``0x0874`` MCP79 GPU [ION]
``0x0876`` MCP79 GPU [ION]
``0x087a`` MCP79 GPU [GeForce 9400]
``0x087d`` MCP79 GPU [ION]
``0x087e`` MCP79 GPU [ION LE]
``0x087f`` MCP79 GPU [ION LE]
========== ========================================================


.. _pci-ids-gt215:

GT215 [NVA3]
------------

========== ========================================================
device id  product
========== ========================================================
``0x0ca0`` GT215 [GeForce GT 330]
``0x0ca2`` GT215 [GeForce GT 320]
``0x0ca3`` GT215 [GeForce GT 240]
``0x0ca4`` GT215 [GeForce GT 340]
``0x0ca5`` GT215 [GeForce GT 220]
``0x0ca7`` GT215 [GeForce GT 330]
``0x0ca9`` GT215 [GeForce GTS 250M]
``0x0cac`` GT215 [GeForce GT 220]
``0x0caf`` GT215 [GeForce GT 335M]
``0x0cb0`` GT215 [GeForce GTS 350M]
``0x0cb1`` GT215 [GeForce GTS 360M]
``0x0cbc`` GT215 [Quadro FX 1800M]
========== ========================================================


.. _pci-ids-gt216:

GT216 [NVA5]
------------

========== ========================================================
device id  product
========== ========================================================
``0x0a20`` GT216 [GeForce GT 220]
``0x0a22`` GT216 [GeForce 315]
``0x0a23`` GT216 [GeForce 210]
``0x0a26`` GT216 [GeForce 405]
``0x0a27`` GT216 [GeForce 405]
``0x0a28`` GT216 [GeForce GT 230M]
``0x0a29`` GT216 [GeForce GT 330M]
``0x0a2a`` GT216 [GeForce GT 230M]
``0x0a2b`` GT216 [GeForce GT 330M]
``0x0a2c`` GT216 [NVS 5100M]
``0x0a2d`` GT216 [GeForce GT 320M]
``0x0a32`` GT216 [GeForce GT 415]
``0x0a34`` GT216 [GeForce GT 240M]
``0x0a35`` GT216 [GeForce GT 325M]
``0x0a38`` GT216 [Quadro 400]
``0x0a3c`` GT216 [Quadro FX 880M]
========== ========================================================


.. _pci-ids-gt218:

GT218 [NVA8]
------------

========== ========================================================
device id  product
========== ========================================================
``0x0a60`` GT218 [GeForce G210]
``0x0a62`` GT218 [GeForce 205]
``0x0a63`` GT218 [GeForce 310]
``0x0a64`` GT218 [ION]
``0x0a65`` GT218 [GeForce 210]
``0x0a66`` GT218 [GeForce 310]
``0x0a67`` GT218 [GeForce 315]
``0x0a68`` GT218 [GeForce G105M]
``0x0a69`` GT218 [GeForce G105M]
``0x0a6a`` GT218 [NVS 2100M]
``0x0a6c`` GT218 [NVS 3100M]
``0x0a6e`` GT218 [GeForce 305M]
``0x0a6f`` GT218 [ION]
``0x0a70`` GT218 [GeForce 310M]
``0x0a71`` GT218 [GeForce 305M]
``0x0a72`` GT218 [GeForce 310M]
``0x0a73`` GT218 [GeForce 305M]
``0x0a74`` GT218 [GeForce G210M]
``0x0a75`` GT218 [GeForce 310M]
``0x0a76`` GT218 [ION]
``0x0a78`` GT218 [Quadro FX 380 LP]
``0x0a7a`` GT218 [GeForce 315M]
``0x0a7c`` GT218 [Quadro FX 380M]
``0x10c0`` GT218 [GeForce 9300 GS]
``0x10c3`` GT218 [GeForce 8400GS]
``0x10c5`` GT218 [GeForce 405]
``0x10d8`` GT218 [NVS 300]
========== ========================================================


.. _pci-ids-mcp89-gpu:

MCP89 GPU [NVAF]
----------------

========== ========================================================
device id  product
========== ========================================================
``0x08a0`` MCP89 GPU [GeForce 320M]
``0x08a2`` MCP89 GPU [GeForce 320M]
``0x08a3`` MCP89 GPU [GeForce 320M]
``0x08a4`` MCP89 GPU [GeForce 320M]
========== ========================================================


.. _pci-ids-gf100:

GF100 [NVC0]
------------

========== ========================================================
device id  product
========== ========================================================
``0x06c0`` GF100 [GeForce GTX 480]
``0x06c4`` GF100 [GeForce GTX 465]
``0x06ca`` GF100 [GeForce GTX 480M]
``0x06cb`` GF100 [GeForce GTX 480]
``0x06cd`` GF100 [GeForce GTX 470]
``0x06d1`` GF100 [Tesla C2050 / C2070]
``0x06d2`` GF100 [Tesla M2070]
``0x06d8`` GF100 [Quadro 6000]
``0x06d9`` GF100 [Quadro 5000]
``0x06da`` GF100 [Quadro 5000M]
``0x06dc`` GF100 [Quadro 6000]
``0x06dd`` GF100 [Quadro 4000]
``0x06de`` GF100 [Tesla T20 Processor]
``0x06df`` GF100 [Tesla M2070-Q]
========== ========================================================


.. _pci-ids-gf104:

GF104 [NVC4]
------------

========== ========================================================
device id  product
========== ========================================================
``0x0e22`` GF104 [GeForce GTX 460]
``0x0e23`` GF104 [GeForce GTX 460 SE]
``0x0e24`` GF104 [GeForce GTX 460 OEM]
``0x0e30`` GF104 [GeForce GTX 470M]
``0x0e31`` GF104 [GeForce GTX 485M]
``0x0e3a`` GF104 [Quadro 3000M]
``0x0e3b`` GF104 [Quadro 4000M]
========== ========================================================


.. _pci-ids-gf114:

GF114 [NVCE]
------------

========== ========================================================
device id  product
========== ========================================================
``0x1200`` GF114 [GeForce GTX 560 Ti]
``0x1201`` GF114 [GeForce GTX 560]
``0x1202`` GF114 [GeForce GTX 560 Ti OEM]
``0x1203`` GF114 [GeForce GTX 460 SE v2]
``0x1205`` GF114 [GeForce GTX 460 v2]
``0x1206`` GF114 [GeForce GTX 555]
``0x1207`` GF114 [GeForce GT 645 OEM]
``0x1208`` GF114 [GeForce GTX 560 SE]
``0x1210`` GF114 [GeForce GTX 570M]
``0x1211`` GF114 [GeForce GTX 580M]
``0x1212`` GF114 [GeForce GTX 675M]
``0x1213`` GF114 [GeForce GTX 670M]
========== ========================================================


.. _pci-ids-gf106:

GF106 [NVC3]
------------

========== ========================================================
device id  product
========== ========================================================
``0x0dc0`` GF106 [GeForce GT 440]
``0x0dc4`` GF106 [GeForce GTS 450]
``0x0dc5`` GF106 [GeForce GTS 450]
``0x0dc6`` GF106 [GeForce GTS 450]
``0x0dcd`` GF106 [GeForce GT 555M]
``0x0dce`` GF106 [GeForce GT 555M]
``0x0dd1`` GF106 [GeForce GTX 460M]
``0x0dd2`` GF106 [GeForce GT 445M]
``0x0dd3`` GF106 [GeForce GT 435M]
``0x0dd6`` GF106 [GeForce GT 550M]
``0x0dd8`` GF106 [Quadro 2000]
``0x0dda`` GF106 [Quadro 2000M]
========== ========================================================


.. _pci-ids-gf116:

GF116 [NVCF]
------------

========== ========================================================
device id  product
========== ========================================================
``0x1241`` GF116 [GeForce GT 545 OEM]
``0x1243`` GF116 [GeForce GT 545]
``0x1244`` GF116 [GeForce GTX 550 Ti]
``0x1245`` GF116 [GeForce GTS 450 Rev. 2]
``0x1246`` GF116 [GeForce GT 550M]
``0x1247`` GF116 [GeForce GT 635M]
``0x1248`` GF116 [GeForce GT 555M]
``0x1249`` GF116 [GeForce GTS 450 Rev. 3]
``0x124b`` GF116 [GeForce GT 640 OEM]
``0x124d`` GF116 [GeForce GT 555M]
``0x1251`` GF116 [GeForce GTX 560M]
========== ========================================================


.. _pci-ids-gf108:

GF108 [NVC1]
------------

========== ========================================================
device id  product
========== ========================================================
``0x0de0`` GF108 [GeForce GT 440]
``0x0de1`` GF108 [GeForce GT 430]
``0x0de2`` GF108 [GeForce GT 420]
``0x0de3`` GF108 [GeForce GT 635M]
``0x0de4`` GF108 [GeForce GT 520]
``0x0de5`` GF108 [GeForce GT 530]
``0x0de8`` GF108 [GeForce GT 620M]
``0x0de9`` GF108 [GeForce GT 630M]
``0x0dea`` GF108 [GeForce 610M]
``0x0deb`` GF108 [GeForce GT 555M]
``0x0dec`` GF108 [GeForce GT 525M]
``0x0ded`` GF108 [GeForce GT 520M]
``0x0dee`` GF108 [GeForce GT 415M]
``0x0def`` GF108 [NVS 5400M]
``0x0df0`` GF108 [GeForce GT 425M]
``0x0df1`` GF108 [GeForce GT 420M]
``0x0df2`` GF108 [GeForce GT 435M]
``0x0df3`` GF108 [GeForce GT 420M]
``0x0df4`` GF108 [GeForce GT 540M]
``0x0df5`` GF108 [GeForce GT 525M]
``0x0df6`` GF108 [GeForce GT 550M]
``0x0df7`` GF108 [GeForce GT 520M]
``0x0df8`` GF108 [Quadro 600]
``0x0df9`` GF108 [Quadro 500M]
``0x0dfa`` GF108 [Quadro 1000M]
``0x0dfc`` GF108 [NVS 5200M]
``0x0f00`` GF108 [GeForce GT 630]
``0x0f01`` GF108 [GeForce GT 620]
========== ========================================================


.. _pci-ids-gf110:

GF110 [NVC8]
------------

========== ========================================================
device id  product
========== ========================================================
``0x1080`` GF110 [GeForce GTX 580]
``0x1081`` GF110 [GeForce GTX 570]
``0x1082`` GF110 [GeForce GTX 560 Ti]
``0x1084`` GF110 [GeForce GTX 560]
``0x1086`` GF110 [GeForce GTX 570]
``0x1087`` GF110 [GeForce GTX 560 Ti]
``0x1088`` GF110 [GeForce GTX 590]
``0x1089`` GF110 [GeForce GTX 580]
``0x108b`` GF110 [GeForce GTX 580]
``0x1091`` GF110 [Tesla M2090]
``0x109a`` GF110 [Quadro 5010M]
``0x109b`` GF110 [Quadro 7000]
========== ========================================================


.. _pci-ids-gf119:

GF119 [NVD9]
------------

========== ========================================================
device id  product
========== ========================================================
``0x1040`` GF119 [GeForce GT 520]
``0x1042`` GF119 [GeForce 510]
``0x1048`` GF119 [GeForce 605]
``0x1049`` GF119 [GeForce GT 620]
``0x104a`` GF119 [GeForce GT 610]
``0x1050`` GF119 [GeForce GT 520M]
``0x1051`` GF119 [GeForce GT 520MX]
``0x1052`` GF119 [GeForce GT 520M]
``0x1054`` GF119 [GeForce 410M]
``0x1055`` GF119 [GeForce 410M]
``0x1056`` GF119 [NVS 4200M]
``0x1057`` GF119 [NVS 4200M]
``0x1058`` GF119 [GeForce 610M]
``0x1059`` GF119 [GeForce 610M]
``0x105a`` GF119 [GeForce 610M]
``0x107d`` GF119 [NVS 310]
========== ========================================================


.. _pci-ids-gf117:

GF117 [NVD7]
------------

========== ========================================================
device id  product
========== ========================================================
``0x1140`` GF117 [GeForce GT 620M]
========== ========================================================


.. _pci-ids-gk104:

GK104 [NVE4]
------------

========== ========================================================
device id  product
========== ========================================================
``0x1180`` GK104 [GeForce GTX 680]
``0x1183`` GK104 [GeForce GTX 660 Ti]
``0x1185`` GK104 [GeForce GTX 660]
``0x1188`` GK104 [GeForce GTX 690]
``0x1189`` GK104 [GeForce GTX 670]
``0x11a0`` GK104 [GeForce GTX 680M]
``0x11a1`` GK104 [GeForce GTX 670MX]
``0x11a2`` GK104 [GeForce GTX 675MX]
``0x11a3`` GK104 [GeForce GTX 680MX]
``0x11a7`` GK104 [GeForce GTX 675MX]
``0x11ba`` GK104 [Quadro K5000]
``0x11bc`` GK104 [Quadro K5000M]
``0x11bd`` GK104 [Quadro K4000M]
``0x11be`` GK104 [Quadro K3000M]
``0x11bf`` GK104 [GRID K2]
========== ========================================================


.. _pci-ids-gk106:

GK106 [NVE6]
------------

========== ========================================================
device id  product
========== ========================================================
``0x11c0`` GK106 [GeForce GTX 660]
``0x11c6`` GK106 [GeForce GTX 650 Ti]
``0x11fa`` GK106 [Quadro K4000]
========== ========================================================


.. _pci-ids-gk107:

GK107 [NVE7]
------------

========== ========================================================
device id  product
========== ========================================================
``0x0fc0`` GK107 [GeForce GT 640]
``0x0fc1`` GK107 [GeForce GT 640]
``0x0fc2`` GK107 [GeForce GT 630]
``0x0fc6`` GK107 [GeForce GTX 650]
``0x0fd1`` GK107 [GeForce GT 650M]
``0x0fd2`` GK107 [GeForce GT 640M]
``0x0fd3`` GK107 [GeForce GT 640M LE]
``0x0fd4`` GK107 [GeForce GTX 660M]
``0x0fd5`` GK107 [GeForce GT 650M]
``0x0fd8`` GK107 [GeForce GT 640M]
``0x0fd9`` GK107 [GeForce GT 645M]
``0x0fe0`` GK107 [GeForce GTX 660M]
``0x0ff9`` GK107 [Quadro K2000D]
``0x0ffa`` GK107 [Quadro K600]
``0x0ffb`` GK107 [Quadro K2000M]
``0x0ffc`` GK107 [Quadro K1000M]
``0x0ffd`` GK107 [NVS 510]
``0x0ffe`` GK107 [Quadro K2000]
``0x0fff`` GK107 [Quadro 410]
========== ========================================================


.. _pci-ids-gk110:

GK110 [NVF0/NVF1]
-----------------

========== ========================================================
device id  product
========== ========================================================
``0x1003`` GK110 [GeForce GTX Titan LE]
``0x1004`` GK110 [GeForce GTX 780]
``0x1005`` GK110 [GeForce GTX Titan]
``0x101f`` GK110 [Tesla K20]
``0x1020`` GK110 [Tesla K20X]
``0x1021`` GK110 [Tesla K20Xm]
``0x1022`` GK110 [Tesla K20c]
``0x1026`` GK110 [Tesla K20s]
``0x1028`` GK110 [Tesla K20m]
========== ========================================================


.. _pci-ids-gk208:

GK208 [NV108]
-------------

========== ========================================================
device id  product
========== ========================================================
``0x1280`` GK208 [GeForce GT 635]
``0x1282`` GK208 [GeForce GT 640 Rev. 2]
``0x1284`` GK208 [GeForce GT 630 Rev. 2]
``0x1290`` GK208 [GeForce GT 730M]
``0x1291`` GK208 [GeForce GT 735M]
``0x1292`` GK208 [GeForce GT 740M]
``0x1293`` GK208 [GeForce GT 730M]
``0x1294`` GK208 [GeForce GT 740M]
``0x1295`` GK208 [GeForce 710M]
``0x12b9`` GK208 [Quadro K610M]
``0x12ba`` GK208 [Quadro K510M]
========== ========================================================


.. _pci-ids-gm107:

GM107 [NV117]
-------------

========== ========================================================
device id  product
========== ========================================================
``0x1381`` GM107 [GeForce GTX 750]
========== ========================================================



.. _pci-ids-gpu-hda:

GPU HDA codecs
==============

========== ========================================================
device id  product
========== ========================================================
``0x0be2`` GT216 HDA
``0x0be3`` GT218 HDA
``0x0be4`` GT215 HDA
``0x0be5`` GF100 HDA
``0x0be9`` GF106 HDA
``0x0bea`` GF108 HDA
``0x0beb`` GF104 HDA
``0x0bee`` GF116 HDA
``0x0e08`` GF119 HDA
``0x0e09`` GF110 HDA
``0x0e0a`` GK104 HDA
``0x0e0b`` GK106 HDA
``0x0e0c`` GF114 HDA
``0x0e0f`` GK208 HDA
``0x0e1a`` GK110 HDA
``0x0e1b`` GK107 HDA
``0x0fbc`` GM107 HDA
========== ========================================================


.. _pci-ids-br02:

BR02
====

The BR02 aka HSI is a transparent PCI-Express - AGP bridge. It can be used
to connect PCIE GPU to AGP bus, or the other way around. Its PCI device id
shadows the actual GPU's device id.

========== ========================================================
device id  product
========== ========================================================
``0x00f1`` BR02+NV43 [GeForce 6600 GT]
``0x00f2`` BR02+NV43 [GeForce 6600]
``0x00f3`` BR02+NV43 [GeForce 6200]
``0x00f4`` BR02+NV43 [GeForce 6600 LE]
``0x00f5`` BR02+G71 [GeForce 7800 GS]
``0x00f6`` BR02+NV43 [GeForce 6800 GS/XT]
``0x00f8`` BR02+NV40 [Quadro FX 3400/4400]
``0x00f9`` BR02+NV40 [GeForce 6800 Series GPU]
``0x00fa`` BR02+NV36 [GeForce PCX 5750]
``0x00fb`` BR02+NV35 [GeForce PCX 5900]
``0x00fc`` BR02+NV34 [GeForce PCX 5300 / Quadro FX 330]
``0x00fd`` BR02+NV34 [Quadro FX 330]
``0x00fe`` BR02+NV35 [Quadro FX 1300]
``0x00ff`` BR02+NV18 [GeForce PCX 4300]
``0x02e0`` BR02+G73 [GeForce 7600 GT]
``0x02e1`` BR02+G73 [GeForce 7600 GS]
``0x02e2`` BR02+G73 [GeForce 7300 GT]
``0x02e3`` BR02+G71 [GeForce 7900 GS]
``0x02e4`` BR02+G71 [GeForce 7950 GT]
========== ========================================================


.. _pci-ids-br03:

BR03
====

The BR03 aka NF100 is a PCI-Express switch with 2 downstream 16x ports. It's
used on NV40 generation dual-GPU cards.

========== ========================================================
device id  product
========== ========================================================
``0x01b3`` BR03 [GeForce 7900 GX2/7950 GX2]
========== ========================================================


.. _pci-ids-br04:

BR04
====

The BR04 aka NF200 is a PCI-Express switch with 4 downstream 16x ports. It's
used on NV50 and NVC0 generation dual-GPU cards, as well as some SLI-capable
motherboards.

========== ========================================================
device id  product
========== ========================================================
``0x05b1`` BR04 [motherboard]
``0x05b8`` BR04 [GeForce GTX 295]
``0x05b9`` BR04 [GeForce GTX 590]
``0x05be`` BR04 [GeForce 9800 GX2/Quadro Plex S4/Tesla S*]
========== ========================================================



Motherboard chipsets
====================


.. _pci-ids-nv1a:

NV1A [nForce 220 IGP / 420 IGP / 415 SPP]
-----------------------------------------

The northbridge of nForce1 chipset, paired with :ref:`MCP <pci-ids-mcp>`.

========== ========================================================
device id  product
========== ========================================================
``0x01a0`` NV1A GPU [GeForce2 MX IGP]
``0x01a4`` NV1A host bridge
``0x01a5`` NV1A host bridge [?]
``0x01a6`` NV1A host bridge [?]
``0x01a8`` NV1A memory controller [?]
``0x01a9`` NV1A memory controller [?]
``0x01aa`` NV1A memory controller #3, 64-bit
``0x01ab`` NV1A memory controller #3, 128-bit
``0x01ac`` NV1A memory controller #1
``0x01ad`` NV1A memory controller #2
``0x01b7`` NV1A/NV2A AGP bridge
========== ========================================================

Note: ``0x01b7`` is also used on :ref:`NV2A <pci-ids-nv2a>`.


.. _pci-ids-nv2a:

NV2A [XGPU]
-----------

The northbridge of xbox, paired with :ref:`MCP <pci-ids-mcp>`.

========== ========================================================
device id  product
========== ========================================================
``0x02a0`` NV2A GPU
``0x02a5`` NV2A host bridge
``0x02a6`` NV2A memory controller
``0x01b7`` NV1A/NV2A AGP bridge
========== ========================================================

Note: ``0x01b7`` is also used on :ref:`NV1A <pci-ids-nv1a>`.


.. _pci-ids-mcp:

MCP
---

The southbridge of nForce1 chipset and xbox, paired with
:ref:`NV1A <pci-ids-nv1a>` or :ref:`NV2A <pci-ids-nv2a>`.

========== ========================================================
device id  product
========== ========================================================
``0x01b0`` MCP APU
``0x01b1`` MCP AC'97
``0x01b2`` MCP LPC bridge
``0x01b4`` MCP SMBus controller
``0x01b8`` MCP PCI bridge
``0x01bc`` MCP IDE controller
``0x01c1`` MCP MC'97
``0x01c2`` MCP USB controller
``0x01c3`` MCP ethernet controller
========== ========================================================


.. _pci-ids-nv1f:

NV1F [nForce2 IGP/SPP]
----------------------

The northbridge of nForce2 chipset, paired with :ref:`MCP2 <pci-ids-mcp2>`
or :ref:`MCP2A <pci-ids-mcp2a>`.

================= ========================================================
device id         product
================= ========================================================
``0x01e0``        NV1F host bridge
``0x01e8``        NV1F AGP bridge
``0x01ea``        NV1F memory controller #1
``0x01eb``        NV1F memory controller #1
``0x01ec``        NV1F memory controller #4
``0x01ed``        NV1F memory controller #3
``0x01ee``        NV1F memory controller #2
``0x01ef``        NV1F memory controller #5
================= ========================================================



.. _pci-ids-mcp2:

MCP2
----

The southbridge of nForce2 chipset, original revision. Paired with
:ref:`NV1F <pci-ids-nv1f>`.

========== ========================================================
device id  product
========== ========================================================
``0x0060`` MCP2 LPC bridge
``0x0064`` MCP2 SMBus controller
``0x0065`` MCP2 IDE controller
``0x0066`` MCP2 ethernet controller
``0x0067`` MCP2 USB controller
``0x0068`` MCP2 USB 2.0 controller
``0x0069`` MCP2 MC'97
``0x006a`` MCP2 AC'97
``0x006b`` MCP2 APU
``0x006c`` MCP2 PCI bridge
``0x006d`` MCP2 internal PCI bridge for 3com ethernet
``0x006e`` MCP2 Firewire controller
========== ========================================================


.. _pci-ids-mcp2a:

MCP2A
-----

The southbridge of nForce2 400 chipset. Paired with :ref:`NV1F <pci-ids-nv1f>`.

========== ========================================================
device id  product
========== ========================================================
``0x0080`` MCP2A LPC bridge
``0x0084`` MCP2A SMBus controller
``0x0085`` MCP2A IDE controller
``0x0086`` MCP2A ethernet controller (class 0200)
``0x0087`` MCP2A USB controller
``0x0088`` MCP2A USB 2.0 controller
``0x0089`` MCP2A MC'97
``0x008a`` MCP2A AC'97
``0x008b`` MCP2A PCI bridge
``0x008c`` MCP2A ethernet controller (class 0680)
``0x008e`` MCP2A SATA controller
========== ========================================================


.. _pci-ids-ck8:

CK8
---

The nforce3-150 chipset.

========== ========================================================
device id  product
========== ========================================================
``0x00d0`` CK8 LPC bridge
``0x00d1`` CK8 host bridge
``0x00d2`` CK8 AGP bridge
``0x00d4`` CK8 SMBus controller
``0x00d5`` CK8 IDE controller
``0x00d6`` CK8 ethernet controller
``0x00d7`` CK8 USB controller
``0x00d8`` CK8 USB 2.0 controller
``0x00d9`` CK8 MC'97
``0x00da`` CK8 AC'97
``0x00dd`` CK8 PCI bridge
========== ========================================================


.. _pci-ids-ck8s:

CK8S
----

The nforce3-250 chipset.

========== ========================================================
device id  product
========== ========================================================
``0x00df`` CK8S ethernet controller (class 0680)
``0x00e0`` CK8S LPC bridge
``0x00e1`` CK8S host bridge
``0x00e2`` CK8S AGP bridge
``0x00e3`` CK8S SATA controller #1
``0x00e4`` CK8S SMBus controller
``0x00e5`` CK8S IDE controller
``0x00e6`` CK8S ethernet controller (class 0200)
``0x00e7`` CK8S USB controller
``0x00e8`` CK8S USB 2.0 controller
``0x00e9`` CK8S MC'97
``0x00ea`` CK8S AC'97
``0x00ec`` CK8S ???? (class 0780)
``0x00ed`` CK8S PCI bridge
``0x00ee`` CK8S SATA controller #0
========== ========================================================


.. _pci-ids-ck804:

CK804
-----

The AMD nforce4 chipset, standalone or paired with C19 or C51 to make nforce4
SLI x16 chipset.

========== ========================================================
device id  product
========== ========================================================
``0x0050`` CK804 LPC bridge
``0x0051`` CK804 LPC bridge
``0x0052`` CK804 SMBus controller
``0x0053`` CK804 IDE controller
``0x0054`` CK804 SATA controller #0
``0x0055`` CK804 SATA controller #1
``0x0056`` CK804 ethernet controller (class 0200)
``0x0057`` CK804 ethernet controller (class 0680)
``0x0058`` CK804 MC'97
``0x0059`` CK804 AC'97
``0x005a`` CK804 USB controller
``0x005b`` CK804 USB 2.0 controller
``0x005c`` CK804 PCI subtractive bridge
``0x005d`` CK804 PCI-Express port
``0x005e`` CK804 memory controller #0
``0x005f`` CK804 memory controller #12
``0x00d3`` CK804 memory controller #10
========== ========================================================


.. _pci-ids-c19:

C19
---

The intel nforce4 northbridge, paired with MCP04 or CK804.

========== ========================================================
device id  product
========== ========================================================
``0x006f`` C19 memory controller #3
``0x0070`` C19 host bridge
``0x0071`` C19 host bridge
``0x0074`` C19 memory controller #1
``0x0075`` C19 memory controller #2
``0x0076`` C19 memory controller #10
``0x0078`` C19 memory controller #11
``0x0079`` C19 memory controller #12
``0x007a`` C19 memory controller #13
``0x007b`` C19 memory controller #14
``0x007c`` C19 memory controller #15
``0x007d`` C19 memory controller #16
``0x007e`` C19 PCI-Express port
``0x007f`` C19 memory controller #1
``0x00b4`` C19 memory controller #4
========== ========================================================


.. _pci-ids-mcp04:

MCP04
-----

The intel nforce4 southbridge, paired with C19.

========== ========================================================
device id  product
========== ========================================================
``0x0030`` MCP04 LPC bridge
``0x0034`` MCP04 SMBus controller
``0x0035`` MCP04 IDE controller
``0x0036`` MCP04 SATA controller #0
``0x0037`` MCP04 ethernet controller (class 0200)
``0x0038`` MCP04 ethernet controller (class 0680)
``0x0039`` MCP04 MC'97
``0x003a`` MCP04 AC'97
``0x003b`` MCP04 USB controller
``0x003c`` MCP04 USB 2.0 controller
``0x003d`` MCP04 PCI subtractive bridge
``0x003e`` MCP04 SATA controller #1
``0x003f`` MCP04 memory controller
========== ========================================================


.. _pci-ids-c51:

C51
---

The AMD nforce4xx/nforce5xx northbridge, paired with CK804, MCP51, or MCP55.

========== ========================================================
device id  product
========== ========================================================
``0x02f0`` C51 memory controller #0
``0x02f1`` C51 memory controller #0
``0x02f2`` C51 memory controller #0
``0x02f3`` C51 memory controller #0
``0x02f4`` C51 memory controller #0
``0x02f5`` C51 memory controller #0
``0x02f6`` C51 memory controller #0
``0x02f7`` C51 memory controller #0
``0x02f8`` C51 memory controller #3
``0x02f9`` C51 memory controller #4
``0x02fa`` C51 memory controller #1
``0x02fb`` C51 PCI-Express x16 port
``0x02fc`` C51 PCI-Express x1 port #0
``0x02fd`` C51 PCI-Express x1 port #1
``0x02fe`` C51 memory controller #2
``0x02ff`` C51 memory controller #5
``0x027e`` C51 memory controller #7
``0x027f`` C51 memory controller #6
========== ========================================================


.. _pci-ids-mcp51:

MCP51
-----

The AMD nforce5xx southbridge, paired with C51 or C55.

========== ========================================================
device id  product
========== ========================================================
``0x0260`` MCP51 LPC bridge
``0x0261`` MCP51 LPC bridge
``0x0262`` MCP51 LPC bridge [?]
``0x0263`` MCP51 LPC bridge [?]
``0x0264`` MCP51 SMBus controller
``0x0265`` MCP51 IDE controller
``0x0266`` MCP51 SATA controller #0
``0x0267`` MCP51 SATA controller #1
``0x0268`` MCP51 ethernet controller (class 0200)
``0x0269`` MCP51 ethernet controller (class 0680)
``0x026a`` MCP51 MC'97
``0x026b`` MCP51 AC'97
``0x026c`` MCP51 HDA
``0x026d`` MCP51 USB controller
``0x026e`` MCP51 USB 2.0 controller
``0x026f`` MCP51 PCI subtractive bridge
``0x0270`` MCP51 memory controller #0
``0x0271`` MCP51 SMU
``0x0272`` MCP51 memory controller #12
========== ========================================================


.. _pci-ids-c55:

C55
---

Paired with MCP51 or MCP55.

========== ========================================================
device id  product
========== ========================================================
``0x03a0`` C55 host bridge [?]
``0x03a1`` C55 host bridge
``0x03a2`` C55 host bridge
``0x03a3`` C55 host bridge
``0x03a4`` C55 host bridge [?]
``0x03a5`` C55 host bridge [?]
``0x03a6`` C55 host bridge [?]
``0x03a7`` C55 host bridge [?]
``0x03a8`` C55 memory controller #5
``0x03a9`` C55 memory controller #3
``0x03aa`` C55 memory controller #2
``0x03ab`` C55 memory controller #4
``0x03ac`` C55 memory controller #1
``0x03ad`` C55 memory controller #10
``0x03ae`` C55 memory controller #11
``0x03af`` C55 memory controller #12
``0x03b0`` C55 memory controller #13
``0x03b1`` C55 memory controller #14
``0x03b2`` C55 memory controller #15
``0x03b3`` C55 memory controller #16
``0x03b4`` C55 memory controller #7
``0x03b5`` C55 memory controller #6
``0x03b6`` C55 memory controller #20
``0x03b7`` C55 PCI-Express x16/x8 port
``0x03b8`` C55 PCI-Express x8 port
``0x03b9`` C55 PCI-Express x1 port #0
``0x03ba`` C55 memory controller #22
``0x03bb`` C55 PCI-Express x1 port #1
``0x03bc`` C55 memory controller #21
========== ========================================================

.. todo:: shouldn't ``0x03b8`` support x4 too?


.. _pci-ids-mcp55:

MCP55
-----

Standalone or paired with C51, C55 or C73.

========== ========================================================
device id  product
========== ========================================================
``0x0360`` MCP55 LPC bridge
``0x0361`` MCP55 LPC bridge
``0x0362`` MCP55 LPC bridge
``0x0363`` MCP55 LPC bridge
``0x0364`` MCP55 LPC bridge
``0x0365`` MCP55 LPC bridge [?]
``0x0366`` MCP55 LPC bridge [?]
``0x0367`` MCP55 LPC bridge [?]
``0x0368`` MCP55 SMBus controller
``0x0369`` MCP55 memory controller #0
``0x036a`` MCP55 memory controller #12
``0x036b`` MCP55 SMU
``0x036c`` MCP55 USB controller
``0x036d`` MCP55 USB 2.0 controller
``0x036e`` MCP55 IDE controller
``0x036f`` MCP55 SATA [???]
``0x0370`` MCP55 PCI subtractive bridge
``0x0371`` MCP55 HDA
``0x0372`` MCP55 ethernet controller (class 0200)
``0x0373`` MCP55 ethernet controller (class 0680)
``0x0374`` MCP55 PCI-Express x1/x4 port #0
``0x0375`` MCP55 PCI-Express x1/x8 port
``0x0376`` MCP55 PCI-Express x8 port
``0x0377`` MCP55 PCI-Express x8/x16 port
``0x0378`` MCP55 PCI-Express x1/x4 port #1
``0x037e`` MCP55 SATA controller [?]
``0x037f`` MCP55 SATA controller
========== ========================================================


.. _pci-ids-mcp61:

MCP61
-----

Standalone.

========== ========================================================
device id  product
========== ========================================================
``0x03e0`` MCP61 LPC bridge
``0x03e1`` MCP61 LPC bridge
``0x03e2`` MCP61 memory controller #0
``0x03e3`` MCP61 LPC bridge [?]
``0x03e4`` MCP61 HDA [?]
``0x03e5`` MCP61 ethernet controller [?]
``0x03e6`` MCP61 ethernet controller [?]
``0x03e7`` MCP61 SATA controller [?]
``0x03e8`` MCP61 PCI-Express x16 port
``0x03e9`` MCP61 PCI-Express x1 port
``0x03ea`` MCP61 memory controller #0
``0x03eb`` MCP61 SMBus controller
``0x03ec`` MCP61 IDE controller
``0x03ee`` MCP61 ethernet controller [?]
``0x03ef`` MCP61 ethernet controller (class 0680)
``0x03f0`` MCP61 HDA
``0x03f1`` MCP61 USB controller
``0x03f2`` MCP61 USB 2.0 controller
``0x03f3`` MCP61 PCI subtractive bridge
``0x03f4`` MCP61 SMU
``0x03f5`` MCP61 memory controller #12
``0x03f6`` MCP61 SATA controller
``0x03f7`` MCP61 SATA controller [?]
========== ========================================================


.. _pci-ids-mcp65:

MCP65
-----

Standalone.

========== ========================================================
device id  product
========== ========================================================
``0x0440`` MCP65 LPC bridge [?]
``0x0441`` MCP65 LPC bridge
``0x0442`` MCP65 LPC bridge
``0x0443`` MCP65 LPC bridge [?]
``0x0444`` MCP65 memory controller #0
``0x0445`` MCP65 memory controller #12
``0x0446`` MCP65 SMBus controller
``0x0447`` MCP65 SMU
``0x0448`` MCP65 IDE controller
``0x0449`` MCP65 PCI subtractive bridge
``0x044a`` MCP65 HDA
``0x044b`` MCP65 HDA [?]
``0x044c`` MCP65 SATA controller (AHCI mode) [?]
``0x044d`` MCP65 SATA controller (AHCI mode)
``0x044e`` MCP65 SATA controller (AHCI mode) [?]
``0x044f`` MCP65 SATA controller (AHCI mode) [?]
``0x0450`` MCP65 ethernet controller (class 0200)
``0x0451`` MCP65 ethernet controller [?]
``0x0452`` MCP65 ethernet controller [?]
``0x0453`` MCP65 ethernet controller [?]
``0x0454`` MCP65 USB controller
``0x0455`` MCP65 USB 2.0 controller
``0x0456`` MCP65 USB controller [?]
``0x0457`` MCP65 USB controller [?]
``0x0458`` MCP65 PCI-Express x8/x16 port
``0x0459`` MCP65 PCI-Express x8 port
``0x045a`` MCP65 PCI-Express x1/x2 port
``0x045b`` MCP65 PCI-Express x2 port
``0x045c`` MCP65 SATA controller (compatibility mode) [?]
``0x045d`` MCP65 SATA controller (compatibility mode)
``0x045e`` MCP65 SATA controller (compatibility mode) [?]
``0x045f`` MCP65 SATA controller (compatibility mode) [?]
========== ========================================================


.. _pci-ids-mcp67:

MCP67
-----

Standalone.

========== ========================================================
device id  product
========== ========================================================
``0x0541`` MCP67 memory controller #12
``0x0542`` MCP67 SMBus controller
``0x0543`` MCP67 SMU
``0x0547`` MCP67 memory controller #0
``0x0548`` MCP67 LPC bridge
``0x054c`` MCP67 ethernet controller (class 0200)
``0x054d`` MCP67 ethernet controller [?]
``0x054e`` MCP67 ethernet controller [?]
``0x054f`` MCP67 ethernet controller [?]
``0x0550`` MCP67 SATA controller (compatibility mode)
``0x0551`` MCP67 SATA controller (compatibility mode) [?]
``0x0552`` MCP67 SATA controller (compatibility mode) [?]
``0x0553`` MCP67 SATA controller (compatibility mode) [?]
``0x0554`` MCP67 SATA controller (AHCI mode)
``0x0555`` MCP67 SATA controller (AHCI mode) [?]
``0x0556`` MCP67 SATA controller (AHCI mode) [?]
``0x0557`` MCP67 SATA controller (AHCI mode) [?]
``0x0558`` MCP67 SATA controller (AHCI mode) [?]
``0x0559`` MCP67 SATA controller (AHCI mode) [?]
``0x055a`` MCP67 SATA controller (AHCI mode) [?]
``0x055b`` MCP67 SATA controller (AHCI mode) [?]
``0x055c`` MCP67 HDA
``0x055d`` MCP67 HDA [?]
``0x055e`` MCP67 USB controller
``0x055f`` MCP67 USB 2.0 controller
``0x0560`` MCP67 IDE controller
``0x0561`` MCP67 PCI subtractive bridge
``0x0562`` MCP67 PCI-Express x16 port
``0x0563`` MCP67 PCI-Express x1 port
========== ========================================================


.. _pci-ids-c73:

C73
---

Paired with MCP55.

========== ========================================================
device id  product
========== ========================================================
``0x0800`` C73 host bridge
``0x0808`` C73 memory controller #1
``0x0809`` C73 memory controller #2
``0x080a`` C73 memory controller #3
``0x080b`` C73 memory controller #4
``0x080c`` C73 memory controller #5
``0x080d`` C73 memory controller #6
``0x080e`` C73 memory controller #7/#17
``0x080f`` C73 memory controller #10
``0x0810`` C73 memory controller #11
``0x0811`` C73 memory controller #12
``0x0812`` C73 memory controller #13
``0x0813`` C73 memory controller #14
``0x0814`` C73 memory controller #15
``0x0815`` C73 PCI-Express x? port #0
``0x0817`` C73 PCI-Express x? port #1
``0x081a`` C73 memory controller #16
========== ========================================================


.. _pci-ids-mcp73:

MCP73
-----

Standalone.

========== ========================================================
device id  product
========== ========================================================
``0x056a`` MCP73 USB 2.0 controller
``0x056c`` MCP73 IDE controller
``0x056d`` MCP73 PCI subtractive bridge
``0x056e`` MCP73 PCI-Express x16 port
``0x056f`` MCP73 PCI-Express x1 port
``0x07c0`` MCP73 host bridge
``0x07c1`` MCP73 host bridge
``0x07c2`` MCP73 host bridge [?]
``0x07c3`` MCP73 host bridge
``0x07c5`` MCP73 host bridge
``0x07c7`` MCP73 host bridge
``0x07c8`` MCP73 memory controller #34
``0x07cb`` MCP73 memory controller #1
``0x07cd`` MCP73 memory controller #10
``0x07ce`` MCP73 memory controller #11
``0x07cf`` MCP73 memory controller #12
``0x07d0`` MCP73 memory controller #13
``0x07d1`` MCP73 memory controller #14
``0x07d2`` MCP73 memory controller #15
``0x07d3`` MCP73 memory controller #16
``0x07d6`` MCP73 memory controller #20
``0x07d7`` MCP73 LPC bridge
``0x07d8`` MCP73 SMBus controller
``0x07d9`` MCP73 memory controller #32
``0x07da`` MCP73 SMU
``0x07dc`` MCP73 ethernet controller (class 0200)
``0x07dd`` MCP73 ethernet controller [?]
``0x07de`` MCP73 ethernet controller [?]
``0x07df`` MCP73 ethernet controller [?]
``0x07f0`` MCP73 SATA controller (compatibility mode)
``0x07f1`` MCP73 SATA controller (compatibility mode) [?]
``0x07f2`` MCP73 SATA controller (compatibility mode) [?]
``0x07f3`` MCP73 SATA controller (compatibility mode) [?]
``0x07f4`` MCP73 SATA controller (AHCI mode)
``0x07f5`` MCP73 SATA controller (AHCI mode) [?]
``0x07f6`` MCP73 SATA controller (AHCI mode) [?]
``0x07f7`` MCP73 SATA controller (AHCI mode) [?]
``0x07f8`` MCP73 SATA controller (RAID mode)
``0x07f9`` MCP73 SATA controller (RAID mode) [?]
``0x07fa`` MCP73 SATA controller (RAID mode) [?]
``0x07fb`` MCP73 SATA controller (RAID mode) [?]
``0x07fc`` MCP73 HDA
``0x07fd`` MCP73 HDA [?]
``0x07fe`` MCP73 USB controller
========== ========================================================


.. _pci-ids-mcp77:

MCP77
-----

Standalone.

================= ========================================================
device id         product
================= ========================================================
``0x0568``        MCP77 memory controller #14
``0x0569``        MCP77 IGP bridge
``0x0570-0x057f`` MCP* ethernet controller (class 0200 alt) [XXX]
``0x0580-0x058f`` MCP* SATA controller (alt ID) [XXX]
``0x0590-0x059f`` MCP* HDA (alt ID) [XXX]
``0x05a0-0x05af`` MCP* IDE (alt ID) [XXX]
``0x0751``        MCP77 memory controller #12
``0x0752``        MCP77 SMBus controller
``0x0753``        MCP77 SMU
``0x0754``        MCP77 memory controller #0
``0x0759``        MCP77 IDE controller
``0x075a``        MCP77 PCI subtractive bridge
``0x075b``        MCP77 PCI-Express x1/x4 port
``0x075c``        MCP77 LPC bridge
``0x075d``        MCP77 LPC bridge
``0x075e``        MCP77 LPC bridge
``0x0760``        MCP77 ethernet controller (class 0200)
``0x0761``        MCP77 ethernet controller [?]
``0x0762``        MCP77 ethernet controller [?]
``0x0763``        MCP77 ethernet controller [?]
``0x0764``        MCP77 ethernet controller (class 0680)
``0x0765``        MCP77 ethernet controller [?]
``0x0766``        MCP77 ethernet controller [?]
``0x0767``        MCP77 ethernet controller [?]
``0x0774``        MCP77 HDA
``0x0775``        MCP77 HDA [?]
``0x0776``        MCP77 HDA [?]
``0x0777``        MCP77 HDA [?]
``0x0778``        MCP77 PCI-Express 2.0 x8/x16 port
``0x0779``        MCP77 PCI-Express 2.0 x8 port
``0x077a``        MCP77 PCI-Express x1 port
``0x077b``        MCP77 USB controller #0
``0x077c``        MCP77 USB 2.0 controller #0
``0x077d``        MCP77 USB controller #1
``0x077e``        MCP77 USB 2.0 controller #1
``0x0ad0-0x0ad3`` MCP77 SATA controller (compatibility mode)
``0x0ad4-0x0ad7`` MCP77 SATA controller (AHCI mode)
``0x0ad8-0x0adb`` MCP77 SATA controller (RAID mode)
================= ========================================================


.. _pci-ids-mcp79:

MCP79
-----

Standalone.

================= ========================================================
device id         product
================= ========================================================
``0x0570-0x057f`` MCP* ethernet controller (class 0200 alt) [XXX]
``0x0580-0x058f`` MCP* SATA controller (alt ID) [XXX]
``0x0590-0x059f`` MCP* HDA (alt ID) [XXX]
``0x0a80``        MCP79 host bridge
``0x0a81``        MCP79 host bridge [?]
``0x0a82``        MCP79 host bridge
``0x0a83``        MCP79 host bridge
``0x0a84``        MCP79 host bridge
``0x0a85``        MCP79 host bridge [?]
``0x0a86``        MCP79 host bridge
``0x0a87``        MCP79 host bridge [?]
``0x0a88``        MCP79 memory controller #1
``0x0a89``        MCP79 memory controller #33
``0x0a8d``        MCP79 memory controller #13
``0x0a8e``        MCP79 memory controller #14
``0x0a8f``        MCP79 memory controller #15
``0x0a90``        MCP79 memory controller #16
``0x0a94``        MCP79 memory controller #23
``0x0a95``        MCP79 memory controller #24
``0x0a98``        MCP79 memory controller #34
``0x0aa0``        MCP79 IGP bridge
``0x0aa2``        MCP79 SMBus controller
``0x0aa3``        MCP79 SMU
``0x0aa4``        MCP79 memory controller #31
``0x0aa5``        MCP79 USB controller #0
``0x0aa6``        MCP79 USB 2.0 controller #0
``0x0aa7``        MCP79 USB controller #1
``0x0aa8``        MCP79 USB controller [?]
``0x0aa9``        MCP79 USB 2.0 controller #1
``0x0aaa``        MCP79 USB 2.0 controller [?]
``0x0aab``        MCP79 PCI subtractive bridge
``0x0aac``        MCP79 LPC bridge
``0x0aad``        MCP79 LPC bridge
``0x0aae``        MCP79 LPC bridge
``0x0aaf``        MCP79 LPC bridge
``0x0ab0``        MCP79 ethernet controller (class 0200)
``0x0ab1``        MCP79 ethernet controller [?]
``0x0ab2``        MCP79 ethernet controller [?]
``0x0ab3``        MCP79 ethernet controller [?]
``0x0ab4-0x0ab7`` MCP79 SATA controller (compatibility mode)
``0x0ab8-0x0abb`` MCP79 SATA controller (AHCI mode)
``0x0abc-0x0abc`` MCP79 SATA controller (RAID mode) [XXX: actually 0x0ab0-0xabb are accepted by hw without trickery]
``0x0ac0``        MCP79 HDA
``0x0ac1``        MCP79 HDA [?]
``0x0ac2``        MCP79 HDA [?]
``0x0ac3``        MCP79 HDA [?]
``0x0ac4``        MCP79 PCI-Express 2.0 x16 port
``0x0ac5``        MCP79 PCI-Express 2.0 x4/x8 port
``0x0ac6``        MCP79 PCI-Express 2.0 x1/x4 port
``0x0ac7``        MCP79 PCI-Express 2.0 x1 port
``0x0ac8``        MCP79 PCI-Express 2.0 x4 port
========== ========================================================


.. _pci-ids-mcp89:

MCP89
-----

Standalone.

================= ========================================================
device id         product
================= ========================================================
``0x0580-0x058f`` MCP* SATA controller (alt ID) [XXX]
``0x0590-0x059f`` MCP* HDA (alt ID) [XXX]
``0x0d60``        MCP89 host bridge
``0x0d68``        MCP89 memory controller #1
``0x0d69``        MCP89 memory controller #33
``0x0d6d``        MCP89 memory controller #10
``0x0d6e``        MCP89 memory controller #11
``0x0d6f``        MCP89 memory controller #12
``0x0d70``        MCP89 memory controller #13
``0x0d71``        MCP89 memory controller #20
``0x0d72``        MCP89 memory controller #21
``0x0d75``        MCP89 memory controller #110
``0x0d76``        MCP89 IGP bridge
``0x0d79``        MCP89 SMBus controller
``0x0d7a``        MCP89 SMU
``0x0d7b``        MCP89 memory controller #31
``0x0d7d``        MCP89 ethernet controller (class 0200)
``0x0d80``        MCP89 LPC bridge
``0x0d84-0x0d87`` MCP89 SATA controller (compatibility mode)
``0x0d88-0x0d8b`` MCP89 SATA controller (AHCI mode)
``0x0d8c-0x0d8f`` MCP89 SATA controller (RAID mode)
``0x0d94-0x0d97`` MCP89 HDA [XXX: actually 1-0xf]
``0x0d9a``        MCP89 PCI-Express x1 port #0
``0x0d9b``        MCP89 PCI-Express x1 port #1
``0x0d9c``        MCP89 USB controller
``0x0d9d``        MCP89 USB 2.0 controller
================= ========================================================


Tegra
=====


.. _pci-ids-t20:

T20
---

========== ========================================================
device id  product
========== ========================================================
``0x0bf0`` T20 PCI-Express x? port #0
``0x0bf1`` T20 PCI-Express x? port #1
========== ========================================================


.. _pci-ids-t30:

T30
---

========== ========================================================
device id  product
========== ========================================================
``0x0e1c`` T30 PCI-Express x? port #0
``0x0e1d`` T30 PCI-Express x? port #1
========== ========================================================
