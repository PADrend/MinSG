/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_IMAGECOMPARE

#ifndef ABSTRACTONGPUCOMPARATOR_H_
#define ABSTRACTONGPUCOMPARATOR_H_

#include "AbstractImageComparator.h"
#include <cstdint>
#include <cstddef>
#include <Util/References.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Texture/Texture.h>
#include <Util/Timer.h>

#include <Geometry/Vec2.h>

#include <set>
#include <map>
#include <vector>

namespace Util {
class FileLocator;
}
namespace Rendering {
class Shader;
class FBO;
}

namespace MinSG {
namespace ImageCompare {

class AbstractOnGpuComparator: public AbstractImageComparator {
	PROVIDES_TYPE_NAME(AbstractOnGpuComparator)

protected:

	class TexRef: public Util::ReferenceCounter<TexRef> {

	private:
		bool autoMode;
		Util::Reference<Rendering::Texture> tex;

		MINSGAPI TexRef(const TexRef & other);
		MINSGAPI TexRef & operator=(const TexRef & other);

	public:
		TexRef(const Geometry::Vec2i & vec) :
				ReferenceCounter_t(), autoMode(true), tex(createTexture(vec)) {
		}
		TexRef(Util::Reference<Rendering::Texture> _tex) : ReferenceCounter_t(), autoMode(false), tex(std::move(_tex)){
		}
		~TexRef() {
			if(autoMode)
				releaseTexture(tex);
		}
		Rendering::Texture * get() {
			return tex.get();
		}
	};

	typedef Util::Reference<TexRef> TexRef_t;

	struct Vec2iComp {
		bool operator()(const Geometry::Vec2i & a, const Geometry::Vec2i & b) const {
			return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
		}
	};

public:
	MINSGAPI static void initShaderFileLocator( const Util::FileLocator& locator);
	MINSGAPI static const Util::FileLocator& getShaderFileLocator();

	enum FilterType {
		GAUSS, BOX
	};

	MINSGAPI AbstractOnGpuComparator(int32_t _filterSize);
	MINSGAPI virtual ~AbstractOnGpuComparator();

	MINSGAPI virtual bool compare(Rendering::RenderingContext & context, Rendering::Texture * firstTex, Rendering::Texture * secondTex, double & value,
			Rendering::Texture * resultTex) override;

	virtual bool doCompare(Rendering::RenderingContext & context, Rendering::Texture * firstTex, Rendering::Texture * secondTex, double & value,
			Rendering::Texture * resultTex) = 0;

	int32_t getFilterSize() const								{	return filterSize;	}

	virtual void setFilterSize(int32_t _filterSize) {
		filterSize = _filterSize;
		filterValid = false;
	}

	virtual void setFilterType(FilterType type) {
		filterType = type;
		filterValid = false;
	}

	FilterType getFilterType()const								{	return filterType;	}

	uint32_t getTextureDownloadSize()const						{	return texDownSize;	}

	virtual void setTextureDownloadSize(uint32_t sideLength)	{	texDownSize = sideLength;	}

	MINSGAPI virtual void setFBO(Util::Reference<Rendering::FBO> _fbo);
	MINSGAPI virtual bool init(Rendering::RenderingContext & context);
	
protected:

	MINSGAPI void prepare(Rendering::RenderingContext & context);

	MINSGAPI void finish(Rendering::RenderingContext & context);

	MINSGAPI void checkTextureSize(Geometry::Vec2i size);
	MINSGAPI void checkTextureSize(uint32_t width, uint32_t height);

	MINSGAPI float average(Rendering::RenderingContext & context, TexRef_t src);
	MINSGAPI void filter(Rendering::RenderingContext & context, TexRef_t src, TexRef_t dst);
	MINSGAPI void copy(Rendering::RenderingContext & context, TexRef_t src, TexRef_t dst);

	MINSGAPI static void deleteTextures();
	MINSGAPI static Util::Reference<Rendering::Texture> createTexture(const Geometry::Vec2i & size);
	MINSGAPI static void releaseTexture(const Util::Reference<Rendering::Texture> & tex);

	Util::Reference<Rendering::Shader> shaderShrink;
	Util::Reference<Rendering::Shader> shaderCopy;
	Util::Reference<Rendering::Shader> shaderFilterH;
	Util::Reference<Rendering::Shader> shaderFilterV;

	Util::Reference<Rendering::FBO> fbo;

	uint32_t texDownSize;
	int32_t filterSize;
	FilterType filterType;
	bool filterValid;
	bool initialized;

//	Util::Timer timer;
//	int testCounter;

	static std::set<Util::Reference<Rendering::Texture> > usedTextures;
	static std::map<Geometry::Vec2i, std::vector<Util::Reference<Rendering::Texture> >, Vec2iComp> freeTextures;
};

}

}

#endif /* ABSTRACTONGPUCOMPARATOR_H_ */

#endif // MINSG_EXT_IMAGECOMPARE
