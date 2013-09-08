.. _nv50-pgraph:
.. _nv50-pgraph-vfetch:
.. _nv50-pgraph-strmout:
.. _nv50-pgraph-ccache:
.. _nv50-pgraph-texture:
.. _nv50-pgraph-rop:
.. _nv50-pgraph-dispatch:
.. _nv50-pgraph-clipid:

PGRAPH: 2d/3d graphics and compute engine
=========================================

Contents:

.. toctree::

::

    PGRAPH, user perspective
    [**   ] graph/intro.txt - Overview of graph objects, functionality common between object classes
    [     ] graph/m2mf.txt - The memory copying objects
    [**   ] graph/2d.txt - Overview of the 2D pipeline
    [**** ] graph/pattern.txt - 2D pattern
    [**   ] graph/ctxobj.txt - graph context objects
    [**   ] graph/solid.txt - 2d solid shape rendering
    [     ] graph/ifc.txt - 2d image from cpu upload
    [     ] graph/nv01-blit.txt - BLIT object
    [     ] graph/nv01-ifm.txt - image to/from memory objects
    [     ] graph/nv01-tex.txt - NV01 textured quad objects
    [     ] graph/nv03-3d.txt - NV03-style 3D objects
    [     ] graph/nv03-gdi.txt - GDI object
    [     ] graph/nv03-sifm.txt - scaled image from memory object
    [     ] graph/nv04-dvd.txt - YCbCr blending object
    [     ] graph/nv10-3d.txt - NV10 Celsius 3D objects
    [     ] graph/nv20-3d.txt - NV20 Kelvin 3D objects
    [     ] graph/nv30-3d.txt - NV30 Rankine 3D objects
    [     ] graph/nv40-3d.txt - NV40 Curie 3D objects
    [     ] graph/nv50-3d.txt - NV50 Tesla 3D objects
    [     ] graph/nvc0-3d.txt - NVC0 Fermi 3D objects
    [     ] graph/nv50-compute.txt - NV50 Compute object
    [     ] graph/nvc0-compute.txt - NVC0 Compute object
    [     ] graph/nv50-texture.txt - NV50 and NVC0 texturing
    [**   ] graph/nv50-cuda-isa.txt - NV50 CUDA/shader ISA overview
    [     ] graph/nvc0-macro.txt - NVC0 graph macro ISA
    [     ] graph/nvc0-cuda-isa.txt - NVC0 CUDA/shader ISA overview


    PGRAPH, kernel perspective
    [*    ] graph/nv01-pgraph.txt - NV01 graphics engine
    [     ] graph/nv03-pgraph.txt - NV03 graphics engine
    [     ] graph/nv03-pdma.txt - NV03 PGRAPH's DMA controller
    [     ] graph/nv04-pgraph.txt - NV04 graphics engine
    [     ] graph/nv10-pgraph.txt - NV10 graphics engine
    [     ] graph/nv20-pgraph.txt - NV20 graphics engine
    [     ] graph/nv40-pgraph.txt - NV40 graphics engine
    [     ] graph/nv50-pgraph.txt - NV50 graphics engine
    [     ] graph/nv50-ctxctl.txt - NV50 PGRAPH context switching unit
    [     ] graph/nvc0-pgraph.txt - NVC0 graphics engine
    [*    ] graph/nvc0-ctxctl/intro.txt - the NVC0 context switching microcoded engines
    [     ] graph/nvc0-ctxctl/mmio.txt - NVC0 CTXCTL MMIO access
    [     ] graph/nvc0-ctxctl/strand.txt - NVC0 strands
    [     ] graph/nvc0-ctxctl/memif.txt - NVC0 CTXCTL memory interface
