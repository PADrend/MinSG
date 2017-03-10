/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2015 Sascha Brandt <myeti@mail.uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef STREAMED_SURFEL_GENERATOR_H_
#define STREAMED_SURFEL_GENERATOR_H_

#include "SurfelGenerator.h"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
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

class StreamedSurfelGenerator : public SurfelGenerator {
	public:
		struct State;

		StreamedSurfelGenerator();
		virtual ~StreamedSurfelGenerator();
		StreamedSurfelGenerator(const StreamedSurfelGenerator & other);
		StreamedSurfelGenerator(StreamedSurfelGenerator && other);

		void setTimeLimit(float ms) { timeLimit_ms = ms; }
		float getTimeLimit() const { return timeLimit_ms; }

		void begin(
				Util::PixelAccessor & pos,
				Util::PixelAccessor & normal,
				Util::PixelAccessor & color,
				Util::PixelAccessor & size
				);
		bool step();
		std::pair<Util::Reference<Rendering::Mesh>,float> getResult();
	private:
		float timeLimit_ms;
		std::unique_ptr<State> state;
};

}
}

#endif /* STREAMED_SURFEL_GENERATOR_H_ */

#endif /* MINSG_EXT_BLUE_SURFELS */
