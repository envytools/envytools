/*
 * Copyright (C) 2016 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HWTEST_PGRAPH_CLASS_H
#define HWTEST_PGRAPH_CLASS_H

#include "pgraph.h"
#include "pgraph_mthd.h"

namespace hwtest {
namespace pgraph {

class Class {
protected:
	TestOptions opt;
	std::mt19937 rnd;
public:
	uint32_t cls;
	std::string name;
	virtual std::vector<SingleMthdTest *> mthds() = 0;
	Class(hwtest::TestOptions &opt, uint32_t seed, uint32_t cls, const std::string &name) : opt(opt), rnd(seed), cls(cls), name(name) {}
};

class ClassTest : public Test {
	Class *cls;
	Subtests subtests() override;
public:
	ClassTest(TestOptions &opt, uint32_t seed, Class *cls)
	: Test(opt, seed), cls(cls) {}
};

class Beta : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Beta4 : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Rop : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Chroma : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Plane : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Clip : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Pattern : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class CPattern : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Surf : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Point : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Line : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Tri : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Rect : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class GdiNv3 : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class GdiNv4 : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Blit : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Ifc : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Bitmap : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Ifm : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Itm : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class TexLin : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class TexQuad : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class TexLinBeta : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class TexQuadBeta : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Iifc : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Sifc : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class Tfc : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class M2mf : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class ZPoint : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class D3D0 : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class OpClip : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class OpBlendAnd : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class OpRopAnd : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class OpChroma : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class OpSrccopyAnd : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class OpSrccopy : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class OpSrccopyPremult : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

class OpBlendPremult : public Class {
	std::vector<SingleMthdTest *> mthds() override;
	using Class::Class;
};

}
}

#endif
