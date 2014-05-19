.. _tesla-texture:

====================
Texture instructions
====================

.. contents::


Introduction
============

.. todo:: write me


Automatic texture load: texauto
===============================

.. todo:: write me

::

  texauto [deriv] live/all <texargs>

    Does a texture fetch. Inputs are: x, y, z, array index, dref [skip all
    that your current sampler setup doesn't use]. x, y, z, dref are floats,
    array index is integer. If running in FP or the deriv flag is on,
    derivatives are computed based on coordinates in all threads of current
    quad. Otherwise, derivatives are assumed 0. For FP, if the live flag
    is on, the tex instruction is only run for fragments that are going to
    be actually written to the render target, ie. for ones that are inside
    the rendered primitive and haven't been discarded yet. all executes
    the tex even for non-visible fragments, which is needed if they're going
    to be used for further derivatives, explicit or implicit.


Raw texel fetch: texfetch
=========================

.. todo:: write me

::

  texfetch live/all <texargs>

    A single-texel fetch. The inputs are x, y, z, index, lod, and are all
    integer.


Texture load with LOD bias: texbias
===================================

.. todo:: write me

::

  texbias [deriv] live/all <texargs>

    Same as texauto, except takes an additional [last] float input specifying
    the LOD bias to add. Note that bias needs to be the same for all threads
    in the current quad executing the texbias insn.


Texture load with manual LOD: texlod
====================================

.. todo:: write me

::

    Does a texture fetch with given coordinates and LOD. Inputs are like
    texbias, except you have explicit LOD instead of the bias. Just like
    in texbias, the LOD should be the same for all threads involved.


Texture size query: texsize
===========================

.. todo:: write me

::

  texsize live/all <texargs>

    Gives you (width, height, depth, mipmap level count) in output, takes
    integer LOD parameter as its only input.


Texture cube calculations: texprep
==================================

.. todo:: write me


Texture LOD query: texquerylod
==============================

.. todo:: write me


Texture CSAA load: texcsaa
==========================

.. todo:: write me


Texture quad load: texgather
============================

.. todo:: write me
