===================================
List and overview of object classes
===================================

This file lists FIFO object classes recognised in hardware by the card. The
list does not include classes that are never put into actual objects [ie.
software objects], nor classes that are put into objects, but never checked,
or checked in firmware.

The classes listed here are the value put into low 8 [NV04:NV25], 12
[NV25:NV40], or 16 [NV40:NVC0] bits of the first word of the object, or
into low 16 bits of method 0 on NVC0+. For some objects, this is different
from the object class used by the blob due to space constraints. Note that
NV30+ objects also present on NV40 have two classes - one shortened to 12
bits for NV30 cards, and a full one used by NV40 cards. They both refer to
the same thing, and will be listed here as such.


======= ======= ===========
Type    Subtype Description
======= ======= ===========
null            a null object, used instead of a dma/ctx2d/ctx3d object to clear a slot

dma             a DMA object

ctx2d           a PGRAPH engine object representing a piece of context used for 2d
                rendering

ctx3d           a PGRAPH engine object representing a piece of context used for 3d
                rendering
\       beta    defines a beta blending factor
\       beta4   defines a 4-component beta blending factor
\       clip    defines a clip rectangle
\       chroma  defines a chroma key
\       rop     defines a raster operation
\       patt    defines a pattern for raster operation
\       surf    defines a surface or surfaces

r2d             a PGRAPH engine object representing a 2d object performing the
                actual rendering, with help of ctx2d objects
\       point   renders a point
\       line    renders a line
\       lin     renders a line without one endpoint
\       tri     renders a triangle
\       rect    renders a rectangle
\       gdi     renders GDI primitives
\       blit    blits between surfaces
\       sifm    scaled blits between surfaces
\       ifc     upload from cpu
\       sifc    upload from cpu with stretching
\       bitmap  bitmap upload from cpu with conversion
\       index   indexed upload from cpu with conversion
\       tex     texture upload from cpu

r3d             a PGRAPH engine object representing a 3d object performing the
                actual rendering, with help of ctx3d objects
\       d3d3    Direct 3D version 3
\       d3d5    Direct 3D version 5
\       d3d6    Direct 3D version 6

op2d            a PGRAPH engine object representing an image transformer - not
                actually usable in hardware, NV01 leftover

uni2d           a PGRAPH engine object representing a unified 2d engine [no ctx2d]

\       dvd     YUV / YUVA blending
\       gen     general-purpose 2d engine

uni3d           a PGRAPH engine object represening a unified 3d engine [no ctx3d]

\       celsius Celsius Direct 3D version 7 engine
\       kelvin  Kelvin Direct 3D version 8 / SM 1 engine
\       rankine Rankine Direct 3D version 9 / SM 2 engine
\       curie   Curie Direct 3D version 9 / SM 3 engine
\       tesla   Tesla Direct 3D version 10 / SM 4 engine
\       fermi   Fermi Direct 3D version 11 / SM 5 engine
\       kepler  Kepler Direct 3D version 11.1 / SM 5 engine

compute         a PGRAPH engine object representing a compute engine
\       tesla   Tesla CUDA 1.x engine
\       fermi   Fermi CUDA 2.x engine
\       kepler  Kepler CUDA 3.x engine

mem             an engine object representing memory copy/fill/upload functionality to
                memory copy functionality
\       copy    a PCOPY engine object representing memory to memory copy functionality
\       m2mf    a PGRAPH engine object representing memory to memory copy functionality
\       p2mf    a PGRAPH engine object representing FIFO to memory upload functionality

crypt           a PCRYPT2 engine object representing crypto functionality
======= ======= ===========

.. todo:: VPE trio


Sorted by class:

======= ======= ======= =============== ====
Class   Type    Subtype Chipsets        Name
======= ======= ======= =============== ====
0002	dma		NV04:NVC0	DMA_FROM_MEMORY
0003	dma		NV04:NVC0	DMA_TO_MEMORY
0010	op2d		NV04:NV05	NV01_OP_CLIP
0011	op2d		NV04:NV05	NV01_OP_BLEND_AND
0012	ctx2d	beta	NV04:NV84	NV01_BETA
0013	op2d		NV04:NV05	NV01_OP_ROP_AND
0015	op2d		NV04:NV05	NV01_OP_CHROMA
0017	ctx2d	chroma	NV04:NV50	NV01_CHROMA
0018	ctx2d	patt	NV04:NV50	NV01_PATTERN
0019	ctx2d	clip	NV04:NV84	NV01_CLIP
001c	r2d	lin	NV04:NV40	NV01_LIN
001d	r2d	tri	NV04:NV40	NV01_TRI
001e	r2d	rect	NV04:NV40	NV01_RECT
001f	r2d	blit	NV04:NV50	NV01_BLIT
0021	r2d	ifc	NV04:NV40	NV01_IFC
0030	null		NV04:NVC0	NULL
0036	r2d	sifc	NV04:NV50	NV03_SIFC
0037	r2d	sifm	NV04:NV50	NV03_SIFM
0038	uni2d	dvd	NV04:NV50	NV04_DVD_SUBPICTURE
0039	mem	m2mf	NV04:NV50	NV03_M2MF
003d	dma		NV04:NVC0	DMA_IN_MEMORY
0042	ctx2d	surf	NV04:NV50	NV04_SURFACE_2D
0043	ctx2d	rop	NV04:NV84	NV03_ROP
0044	ctx2d	patt	NV04:NV84	NV04_PATTERN
0048	r3d	d3d3	NV04:NV15	NV03_TEXTURED_TRIANGLE
004a	r2d	gdi	NV04:NV84	NV04_GDI
004b	r2d	gdi	NV04:NV40	NV03_GDI
0052	ctx2d	surf	NV04:NV50	NV04_SURFACE_SWZ
0053	ctx3d	surf	NV04:NV20	NV04_SURFACE_3D
0054	r3d	d3d5	NV04:NV20	NV04_TEXTURED_TRIANGLE
0055	r3d	d3d6	NV04:NV20	NV04_MULTITEX_TRIANGLE
0056	uni3d	celsius	NV10:NV30	NV10_3D
0057	ctx2d	chroma	NV04:NV84	NV04_CHROMA
0058	ctx2d	surf	NV04:NV50	NV03_SURFACE_DST
0059	ctx2d	surf	NV04:NV50	NV03_SURFACE_SRC
005a	ctx3d	surf	NV04:NV50	NV03_SURFACE_COLOR
005b	ctx3d	surf	NV04:NV50	NV03_SURFACE_ZETA
005c	r2d	lin	NV04:NV50	NV04_LIN
005d	r2d	tri	NV04:NV84	NV04_TRI
005e	r2d	rect	NV04:NV40	NV04_RECT
005f	r2d	blit	NV04:NV84	NV04_BLIT
0060	r2d	index	NV04:NV50	NV04_INDEX
0061	r2d	ifc	NV04:NV50	NV04_IFC
0062	ctx2d	surf	NV10:NV50	NV10_SURFACE_2D
0063	r2d	sifm	NV10:NV50	NV05_SIFM
0064	op2d		NV04:NV05	NV01_OP_SRCCOPY_AND
0064	r2d	index	NV05:NV50	NV05_INDEX
0065	op2d		NV04:NV05	NV03_OP_SRCCOPY
0065	r2d	ifc	NV05:NV50	NV05_IFC
0066	op2d		NV04:NV05	NV04_OP_SRCCOPY_PREMULT
0066	r2d	sifc	NV05:NV50	NV05_SIFC
0067	op2d		NV04:NV05	NV04_OP_BLEND_PREMULT
0072	ctx2d	beta4	NV04:NV84	NV04_BETA4
0076	r2d	sifc	NV04:NV50	NV04_SIFC
0077	r2d	sifm	NV04:NV50	NV04_SIFM
007b	r2d	tex	NV10:NV50	NV10_TEXUPLOAD
0088	uni2d	dvd	NV10:NV50	NV10_DVD_SUBPICTURE
0089	r2d	sifm	NV10:NV40	NV10_SIFM
008a	r2d	ifc	NV10:NV50	NV10_IFC
0093	ctx3d	surf	NV10:NV20	NV10_SURFACE_3D
0094	r3d	d3d5	NV10:NV20	NV10_TEXTURED_TRIANGLE
0095	r3d	d3d6	NV10:NV20	NV10_MULTITEX_TRIANGLE
0096	uni3d	celsius	NV15:NV30	NV15_3D
0097	uni3d	kelvin	NV20:NV34	NV20_3D
0098	uni3d	celsius	NV17:NV30	NV11_3D
0099	uni3d	celsius	NV17:NV20	NV17_3D
009e	ctx2d	surf	NV10:NV50	NV20_SURFACE_SWZ [buggy on NV10]
009f	r2d	blit	NV15:NV50	NV15_BLIT
035c	r2d	lin	NV30:NV40	NV30_LIN
0362	ctx2d	surf	NV30:NV40	NV30_SURFACE_2D
0364	r2d	index	NV30:NV40	NV30_INDEX
0366	r2d	sifc	NV30:NV40	NV30_SIFC
037b	r2d	tex	NV30:NV40	NV30_TEXUPLOAD
0389	r2d	sifm	NV30:NV40	NV30_SIFM
038a	r2d	ifc	NV30:NV40	NV30_IFC
0397	uni3d	rankine	NV30:NV40	NV30_3D
039e	ctx2d	surf	NV30:NV40	NV30_SURFACE_SWZ
0497	uni3d	rankine	NV35:NV34	NV35_3D
0597	uni3d	kelvin	NV25:NV40	NV25_3D
0697	uni3d	rankine	NV34:NV40	NV34_3D
305c	r2d	lin	NV40:NV84	NV30_LIN
3062	ctx2d	surf	NV40:NV50	NV30_SURFACE_2D
3064	r2d	index	NV40:NV84	NV30_INDEX
3066	r2d	sifc	NV40:NV84	NV30_SIFC
307b	r2d	tex	NV40:NV84	NV30_TEXUPLOAD
3089	r2d	sifm	NV40:NV50	NV30_SIFM
308a	r2d	ifc	NV40:NV84	NV30_IFC
309e	ctx2d	surf	NV40:NV50	NV30_SURFACE_SWZ
4097	uni3d	curie	NV40:NV44	NV40_3D
4497	uni3d	curie	NV44:NV50	NV44_3D
502d	uni2d	gen	NV50:NVC0	NV50_2D
5039	mem	m2mf	NV50:NVC0	NV50_M2MF
5062	ctx2d	surf	NV50:NV84	NV50_SURFACE_2D
5089	r2d	sifm	NV50:NV84	NV50_SIFM
5097	uni3d	tesla	NV50:NVA0	NV50_3D
50c0	comp	tesla	NV50:NVC0	NV50_COMPUTE
74c1	crypt		NV84:NV98	NV84_CRYPT
8297	uni3d	tesla	NV84:NVA0	NV84_3D
8397	uni3d	tesla	NVA0:NVA3	NVA0_3D
8597	uni3d	tesla	NVA3:NVAF	NVA3_3D
85c0	comp	tesla	NVA3:NVC0	NVA3_COMPUTE
8697	uni3d	tesla	NVAF:NVC0	NVAF_3D
902d	uni2d	gen	NVC0:...	NVC0_2D
9039	mem	m2mf	NVC0:NVE4	NVC0_M2MF
9097	uni3d	fermi	NVC0:NVE4	NVC0_3D
90c0	comp	fermi	NVC0:NVE4	NVC0_COMPUTE
9197	uni3d	fermi	NVC1:NVE4	NVC1_3D
91c0	comp	fermi	NVC8:NVE4	NVC8_COMPUTE
9297	uni3d	fermi	NVC8:NVE4	NVC8_3D
a040	mem	p2mf	NVE4:NVF0	NVE4_P2MF
a097	uni3d	kepler	NVE4:NVF0	NVE4_3D
a0b5	mem	copy	NVE4:...	NVE4_COPY
a0c0	comp	kepler	NVE4:NVF0	NVE4_COMPUTE
a140	mem	p2mf	NVF0:...	NVF0_P2MF
a197	uni3d	kepler	NVF0:...	NVF0_3D
a1c0	comp	kepler	NVF0:...	NVF0_COMPUTE
======= ======= ======= =============== ====

Sorted by type:

======= ======= ======= =============== ====
Class   Type    Subtype Chipsets        Name
======= ======= ======= =============== ====
0030	null		NV04:NVC0	NULL
------- ------- ------- --------------- ----
0002	dma		NV04:NVC0	DMA_FROM_MEMORY
0003	dma		NV04:NVC0	DMA_TO_MEMORY
003d	dma		NV04:NVC0	DMA_IN_MEMORY
------- ------- ------- --------------- ----
0039	mem	m2mf	NV04:NV50	NV03_M2MF
5039	mem	m2mf	NV50:NVC0	NV50_M2MF
9039	mem	m2mf	NVC0:NVE4	NVC0_M2MF
a040	mem	p2mf	NVE4:NVF0	NVE4_P2MF
a140	mem	p2mf	NVF0:...	NVF0_P2MF
a0b5	mem	copy	NVE4:...	NVE4_COPY
------- ------- ------- --------------- ----
0010	op2d		NV04:NV05	NV01_OP_CLIP
0011	op2d		NV04:NV05	NV01_OP_BLEND_AND
0013	op2d		NV04:NV05	NV01_OP_ROP_AND
0015	op2d		NV04:NV05	NV01_OP_CHROMA
0064	op2d		NV04:NV05	NV01_OP_SRCCOPY_AND
0065	op2d		NV04:NV05	NV03_OP_SRCCOPY
0066	op2d		NV04:NV05	NV04_OP_SRCCOPY_PREMULT
0067	op2d		NV04:NV05	NV04_OP_BLEND_PREMULT
------- ------- ------- --------------- ----
0012	ctx2d	beta	NV04:NV84	NV01_BETA
0072	ctx2d	beta4	NV04:NV84	NV04_BETA4
0017	ctx2d	chroma	NV04:NV50	NV01_CHROMA
0057	ctx2d	chroma	NV04:NV84	NV04_CHROMA
0018	ctx2d	patt	NV04:NV50	NV01_PATTERN
0044	ctx2d	patt	NV04:NV84	NV04_PATTERN
0019	ctx2d	clip	NV04:NV84	NV01_CLIP
0043	ctx2d	rop	NV04:NV84	NV03_ROP
0058	ctx2d	surf	NV04:NV50	NV03_SURFACE_DST
0059	ctx2d	surf	NV04:NV50	NV03_SURFACE_SRC
005a	ctx3d	surf	NV04:NV50	NV03_SURFACE_COLOR
005b	ctx3d	surf	NV04:NV50	NV03_SURFACE_ZETA
0052	ctx2d	surf	NV04:NV50	NV04_SURFACE_SWZ
009e	ctx2d	surf	NV10:NV50	NV20_SURFACE_SWZ [buggy on NV10]
039e	ctx2d	surf	NV30:NV40	NV30_SURFACE_SWZ
309e	ctx2d	surf	NV40:NV50	NV30_SURFACE_SWZ
0042	ctx2d	surf	NV04:NV50	NV04_SURFACE_2D
0062	ctx2d	surf	NV10:NV50	NV10_SURFACE_2D
0362	ctx2d	surf	NV30:NV40	NV30_SURFACE_2D
3062	ctx2d	surf	NV40:NV50	NV30_SURFACE_2D
5062	ctx2d	surf	NV50:NV84	NV50_SURFACE_2D
0053	ctx3d	surf	NV04:NV20	NV04_SURFACE_3D
0093	ctx3d	surf	NV10:NV20	NV10_SURFACE_3D
------- ------- ------- --------------- ----
001c	r2d	lin	NV04:NV40	NV01_LIN
005c	r2d	lin	NV04:NV50	NV04_LIN
035c	r2d	lin	NV30:NV40	NV30_LIN
305c	r2d	lin	NV40:NV84	NV30_LIN
------- ------- ------- --------------- ----
001d	r2d	tri	NV04:NV40	NV01_TRI
005d	r2d	tri	NV04:NV84	NV04_TRI
------- ------- ------- --------------- ----
001e	r2d	rect	NV04:NV40	NV01_RECT
005e	r2d	rect	NV04:NV40	NV04_RECT
------- ------- ------- --------------- ----
001f	r2d	blit	NV04:NV50	NV01_BLIT
005f	r2d	blit	NV04:NV84	NV04_BLIT
009f	r2d	blit	NV15:NV50	NV15_BLIT
------- ------- ------- --------------- ----
0060	r2d	index	NV04:NV50	NV04_INDEX
0064	r2d	index	NV05:NV50	NV05_INDEX
0364	r2d	index	NV30:NV40	NV30_INDEX
3064	r2d	index	NV40:NV84	NV30_INDEX
------- ------- ------- --------------- ----
0021	r2d	ifc	NV04:NV40	NV01_IFC
0061	r2d	ifc	NV04:NV50	NV04_IFC
0065	r2d	ifc	NV05:NV50	NV05_IFC
008a	r2d	ifc	NV10:NV50	NV10_IFC
038a	r2d	ifc	NV30:NV40	NV30_IFC
308a	r2d	ifc	NV40:NV84	NV30_IFC
------- ------- ------- --------------- ----
0036	r2d	sifc	NV04:NV50	NV03_SIFC
0076	r2d	sifc	NV04:NV50	NV04_SIFC
0066	r2d	sifc	NV05:NV50	NV05_SIFC
0366	r2d	sifc	NV30:NV40	NV30_SIFC
3066	r2d	sifc	NV40:NV84	NV30_SIFC
------- ------- ------- --------------- ----
0037	r2d	sifm	NV04:NV50	NV03_SIFM
0077	r2d	sifm	NV04:NV50	NV04_SIFM
0063	r2d	sifm	NV10:NV50	NV05_SIFM
0089	r2d	sifm	NV10:NV40	NV10_SIFM
0389	r2d	sifm	NV30:NV40	NV30_SIFM
3089	r2d	sifm	NV40:NV50	NV30_SIFM
5089	r2d	sifm	NV50:NV84	NV50_SIFM
------- ------- ------- --------------- ----
004b	r2d	gdi	NV04:NV40	NV03_GDI
004a	r2d	gdi	NV04:NV84	NV04_GDI
------- ------- ------- --------------- ----
007b	r2d	tex	NV10:NV50	NV10_TEXUPLOAD
037b	r2d	tex	NV30:NV40	NV30_TEXUPLOAD
307b	r2d	tex	NV40:NV84	NV30_TEXUPLOAD
------- ------- ------- --------------- ----
0038	uni2d	dvd	NV04:NV50	NV04_DVD_SUBPICTURE
0088	uni2d	dvd	NV10:NV50	NV10_DVD_SUBPICTURE
------- ------- ------- --------------- ----
502d	uni2d	gen	NV50:NVC0	NV50_2D
902d	uni2d	gen	NVC0:...	NVC0_2D
------- ------- ------- --------------- ----
0048	r3d	d3d3	NV04:NV15	NV03_TEXTURED_TRIANGLE
------- ------- ------- --------------- ----
0054	r3d	d3d5	NV04:NV20	NV04_TEXTURED_TRIANGLE
0094	r3d	d3d5	NV10:NV20	NV10_TEXTURED_TRIANGLE
------- ------- ------- --------------- ----
0055	r3d	d3d6	NV04:NV20	NV04_MULTITEX_TRIANGLE
0095	r3d	d3d6	NV10:NV20	NV10_MULTITEX_TRIANGLE
------- ------- ------- --------------- ----
0056	uni3d	celsius	NV10:NV30	NV10_3D
0096	uni3d	celsius	NV15:NV30	NV15_3D
0098	uni3d	celsius	NV17:NV30	NV11_3D
0099	uni3d	celsius	NV17:NV20	NV17_3D
------- ------- ------- --------------- ----
0097	uni3d	kelvin	NV20:NV34	NV20_3D
0597	uni3d	kelvin	NV25:NV40	NV25_3D
------- ------- ------- --------------- ----
0397	uni3d	rankine	NV30:NV40	NV30_3D
0497	uni3d	rankine	NV35:NV34	NV35_3D
0697	uni3d	rankine	NV34:NV40	NV34_3D
------- ------- ------- --------------- ----
4097	uni3d	curie	NV40:NV44	NV40_3D
4497	uni3d	curie	NV44:NV50	NV44_3D
------- ------- ------- --------------- ----
5097	uni3d	tesla	NV50:NVA0	NV50_3D
8297	uni3d	tesla	NV84:NVA0	NV84_3D
8397	uni3d	tesla	NVA0:NVA3	NVA0_3D
8597	uni3d	tesla	NVA3:NVAF	NVA3_3D
8697	uni3d	tesla	NVAF:NVC0	NVAF_3D
------- ------- ------- --------------- ----
9097	uni3d	fermi	NVC0:NVE4	NVC0_3D
9197	uni3d	fermi	NVC1:NVE4	NVC1_3D
9297	uni3d	fermi	NVC8:NVE4	NVC8_3D
------- ------- ------- --------------- ----
a097	uni3d	kepler	NVE4:NVF0	NVE4_3D
a197	uni3d	kepler	NVF0:...	NVF0_3D
------- ------- ------- --------------- ----
50c0	comp	tesla	NV50:NVC0	NV50_COMPUTE
85c0	comp	tesla	NVA3:NVC0	NVA3_COMPUTE
------- ------- ------- --------------- ----
90c0	comp	fermi	NVC0:NVE4	NVC0_COMPUTE
91c0	comp	fermi	NVC8:NVE4	NVC8_COMPUTE
------- ------- ------- --------------- ----
a0c0	comp	kepler	NVE4:NVF0	NVE4_COMPUTE
a1c0	comp	kepler	NVF0:...	NVF0_COMPUTE
------- ------- ------- --------------- ----
74c1	crypt		NV84:NV98	NV84_CRYPT
======= ======= ======= =============== ====
