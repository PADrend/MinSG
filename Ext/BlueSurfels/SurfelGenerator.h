/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef SURFEL_GENERATOR_H_
#define SURFEL_GENERATOR_H_

#include <cstddef>
#include <cstdint>
#include <vector>
#include <Geometry/Vec3.h>
#include <Util/References.h>
#include <Util/Graphics/Color.h>

namespace Util {
class PixelAccessor;
}
namespace Rendering {
class Mesh;
}
namespace MinSG {
class Node;
class FrameContext;
namespace BlueSurfels {

class SurfelGenerator{
		uint32_t maxAbsSurfels;
		float reusalRate;

	public:
		
		struct Surfel{
			Geometry::Vec3 pos;
			Geometry::Vec3 normal;
			Util::Color4f color;
			float size;
			Surfel() : size(0){}
			Surfel(Geometry::Vec3 _pos,Geometry::Vec3 _normal,Util::Color4f _color,float s)
					: pos(std::move(_pos)),normal(std::move(_normal)),color(std::move(_color)),size(s){}
			bool operator==(const Surfel&other)const{
				return pos==other.pos 
						&& normal==other.normal 
						&& color==other.color 
						&& size==other.size; 
			}
			const Geometry::Vec3 & getPosition()const {	return pos;	}
			
		};
		
		
		SurfelGenerator() : maxAbsSurfels(10000),reusalRate(0.6){}
		uint32_t getMaxAbsSurfels()const			{	return maxAbsSurfels;	}
		float getReusalRate()const				{	return reusalRate;	}
		void setMaxAbsSurfels(uint32_t i)			{	maxAbsSurfels = i;	}
		void setReusalRate(float f)				{	reusalRate = f;	}
	
		
		std::vector<Surfel> extractSurfelsFromTextures(	
													Util::PixelAccessor & pos,
													Util::PixelAccessor & normal,
													Util::PixelAccessor & color,
													Util::PixelAccessor & size
													)const;

		Util::Reference<Rendering::Mesh> buildBlueSurfels(const std::vector<Surfel> & allSurfels)const;
		
		/*! Calculate a surfel-mesh from a set of bitmaps encoding the positions, normals, colors and size using the SurfelGenerator's current settings.
			\param The bitmaps are given as Util::PixelAccessors parameters.

			\return The result is [created Mesh, relativeCovering]. The relativeCovering is a value from 0...1 describing how much of the given textures is
				actually covered by the object.
				
			\see Uses extractSurfelsFromTextures(...) and buildBlueSurfels(...)
		*/
		std::pair<Util::Reference<Rendering::Mesh>,float> createSurfels(
													Util::PixelAccessor & pos,
													Util::PixelAccessor & normal,
													Util::PixelAccessor & color,
													Util::PixelAccessor & size
													)const;
};

}
}

#endif /* SURFEL_GENERATOR_H_ */

#endif /* MINSG_EXT_BLUE_SURFELS */
