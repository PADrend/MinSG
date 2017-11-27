/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2016-2017 Sascha Brandt <myeti@mail.uni-paderborn.de>
	
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
#include <unordered_map>
#include <tuple>
#include <Geometry/Vec3.h>
#include <Util/References.h>
#include <Util/Graphics/Color.h>
#include <Rendering/Mesh/VertexDescription.h>

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
		mutable std::unordered_map<std::string,float> benchmarkResults;
		Rendering::VertexDescription vertexDescription;
	public:
		struct Parameters {
			uint32_t maxAbsSurfels;
			uint32_t medianDistCount;
			uint32_t samplesPerRound;
			bool pureRandomStrategy;
			bool guessSurfelSize;
			bool benchmarkingEnabled;
			uint32_t seed;
		};
		
		struct SurfelResult {
			Util::Reference<Rendering::Mesh> mesh;
			float minDist;
			float medianDist;
		};
		
		struct Surfel {
			Geometry::Vec3 pos;
			uint32_t index;
			Surfel() = default;
			Surfel(Geometry::Vec3 _pos, uint32_t _index) : pos(std::move(_pos)), index(_index) { }
			bool operator==(const Surfel&other) const {
				return pos==other.pos && index==other.index; 
			}
			const Geometry::Vec3 & getPosition() const {	return pos;	}			
			uint32_t getIndex() const {	return index;	}			
		};
				
		class SurfelBuilder {
		public:
			virtual uint32_t addSurfel(const Surfel& s) = 0;
			virtual Rendering::Mesh* buildMesh() = 0;
		};
		
		SurfelGenerator() : parameters{10000,1000,160,false,false,false,0} {			
			vertexDescription.appendPosition3D();
			vertexDescription.appendNormalByte();
			vertexDescription.appendColorRGBAByte();
		}
		void setParameters(const Parameters& p) { parameters = p; };
		const Parameters& getParameters() const { return parameters; };
		uint32_t getMaxAbsSurfels()const			{	return parameters.maxAbsSurfels;	}
		void setMaxAbsSurfels(uint32_t i)			{	parameters.maxAbsSurfels = i;	}
		void setBenchmarkingEnabled(bool b)			{	parameters.benchmarkingEnabled = b;	}
		const std::unordered_map<std::string,float>& getBenchmarkResults()const	{	return benchmarkResults;	}
		void clearBenchmarkResults()const			{	benchmarkResults.clear();	}
		void setVertexDescription(const Rendering::VertexDescription& vd);
	
		
		std::vector<Surfel> extractSurfelsFromTextures(	
													Util::PixelAccessor & pos,
													Util::PixelAccessor & normal,
													Util::PixelAccessor & color
													)const;

		SurfelResult buildBlueSurfels(const std::vector<Surfel> & allSurfels, SurfelBuilder* sb)const;
		
		/*! Calculate a surfel-mesh from a set of bitmaps encoding the positions, normals, colors and size using the SurfelGenerator's current settings.
			\param The bitmaps are given as Util::PixelAccessors parameters.

			\return The result is [created Mesh, relativeCovering]. The relativeCovering is a value from 0...1 describing how much of the given textures is
				actually covered by the object.
				
			\see Uses extractSurfelsFromTextures(...) and buildBlueSurfels(...)
		*/
		SurfelResult createSurfels(
													Util::PixelAccessor & pos,
													Util::PixelAccessor & normal,
													Util::PixelAccessor & color
													)const;
		
		SurfelResult createSurfelsFromMesh(Rendering::Mesh& mesh) const;
	private:
		Parameters parameters;
};

}
}

#endif /* SURFEL_GENERATOR_H_ */

#endif /* MINSG_EXT_BLUE_SURFELS */
