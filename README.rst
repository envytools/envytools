=========
envytools
=========
*Tools for people envious of nvidia's blob driver.*

.. image:: https://travis-ci.org/envytools/envytools.svg?branch=master
       :target: https://travis-ci.org/envytools/envytools

.. image:: https://readthedocs.org/projects/envytools/badge/?version=latest
       :target: https://envytools.readthedocs.io

The canonical repo is at: https://github.com/envytools/envytools/. Pushing
anywhere else will result in a de-facto fork!

Contents
========

Subdirectories:

- ``docs``: plain-text documentation of the GPUs, nVidia binary driver, and
  the tools (in-sync HTML version at https://envytools.readthedocs.io)
- ``envydis``: Disassembler and assembler for various ISAs found on nvidia GPUs
- ``rnn``: Tools and libraries for the rules-ng-ng XML register database format
- ``rnndb``: rnn database of nvidia MMIO registers, FIFO methods, and memory
  structures.
- ``nvbios``: Tools to decode the card description structures found in nvidia
  VBIOS
- ``nva``: Tools to directly access the GPU registers
- ``vstream``: Tools to decode and encode raw video bitstreams
- ``vdpow``: A tool aiding in VP3 reverse engineering
- ``easm``: Utility code dealing with assembly language parsing & printing.
- ``util``: Misc utility code shared between envytools modules


Building, installing
====================

Dependencies:

- ``cmake``
- ``libxml2``
- ``flex``
- ``bison``
- ``pkg-config``

Optional dependencies needed by hwtest:

- ``libpciaccess``

Optional dependencies needed by demmt:

- ``libdrm (nouveau)``
- ``libseccomp``

Optional dependencies needed by nva:

- ``libpciaccess``
- ``libx11``
- ``libxext``
- ``python3``
- ``cython3``

Optional dependencies needed by vdpow:

- ``libpciaccess``
- ``libx11``
- ``vdpau``

Optional dependencies needed for ninja build (faster recompile times):

- ``ninja``

If your distribution has -dev or -devel packages, you'll also need ones
corresponding to the dependencies above.

On ubuntu it can be done like this::

    apt-get install cmake flex libpciaccess-dev bison libx11-dev libxext-dev libxml2-dev libvdpau-dev python3-dev cython3

To build using ninja (recommended if you work on envytools)::

    $ cmake . -G Ninja
    $ ninja

If you prefer to stay with make, use ::

    $ cmake .
    $ make

To install [which is optional], use ::

    $ ninja install

or ::

    $ make install

If you want to install to a non-default directory, you'll also need to pass
it as an option to cmake before building, eg.::

    $ cmake -D CMAKE_INSTALL_PREFIX:PATH=/usr/local .

Cmake options
-------------

If you don't want to compile some parts of envytools, you can pass the
following options to cmake:

- Hwtest:	``-DDISABLE_HWTEST=ON``
- Nva:	        ``-DDISABLE_NVA=ON``
- VDPOW:	``-DDISABLE_VDPOW=ON``
