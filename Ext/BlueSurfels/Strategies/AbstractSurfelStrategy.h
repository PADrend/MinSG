/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef BLUESURFELS_STRATEGIES_ABSTRACT_STRATEGY_H_
#define BLUESURFELS_STRATEGIES_ABSTRACT_STRATEGY_H_

#include <Geometry/Matrix4x4.h>

#include <Util/Macros.h>
#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>
#include <Util/StringIdentifier.h>
#include <Util/Factory/WrapperFactory.h>

#include <cstdint>
#include <memory>

namespace MinSG {
class Node;
class FrameContext;
}

namespace Rendering {
class Mesh;
}

namespace Util {
class GenericAttributeMap;
}

namespace MinSG {
namespace BlueSurfels {
	
struct SurfelObject {
	Rendering::Mesh* mesh;
	uint32_t maxPrefix;
	float packing;
	Geometry::Matrix4x4 surfelToCamera;
	float relPixelSize;
	uint32_t prefix;
	float pointSize;
	float radius;
	float sizeFactor;
};

class AbstractSurfelStrategy : public Util::ReferenceCounter<AbstractSurfelStrategy> {
		PROVIDES_TYPE_NAME(AbstractSurfelStrategy)
	public:
		AbstractSurfelStrategy(float priority = 0) : priority(priority) {}
		
		virtual ~AbstractSurfelStrategy() = default;
		virtual bool prepare(MinSG::FrameContext& context, MinSG::Node* node) { return false; }
		virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel) { return false; }
		virtual bool beforeRendering(MinSG::FrameContext& context) { return false; }
		virtual void afterRendering(MinSG::FrameContext& context) {}
		
		float getPriority() const { return priority; }
		void setEnabled(bool v) { enabled = v; }
		bool isEnabled() const { return enabled; }
	private:
		const float priority;
		bool enabled = true;
};

// ------------------------------------------------------------------------
// Import/Export

typedef std::function<void (Util::GenericAttributeMap& desc, AbstractSurfelStrategy* strategy)> SurfelStrategyExporter_t;
typedef std::function<AbstractSurfelStrategy* (const Util::GenericAttributeMap* desc)> SurfelStrategyImporter_t;

MINSGAPI extern const std::string TYPE_STRATEGY;
MINSGAPI extern const Util::StringIdentifier ATTR_STRATEGY_TYPE;

MINSGAPI bool registerExporter(const Util::StringIdentifier& type, const SurfelStrategyExporter_t& exporter);
MINSGAPI bool registerImporter(const Util::StringIdentifier& type, const SurfelStrategyImporter_t& importer);
MINSGAPI std::unique_ptr<Util::GenericAttributeMap> exportStrategy(AbstractSurfelStrategy* strategy);
MINSGAPI AbstractSurfelStrategy* importStrategy(const Util::GenericAttributeMap * d);

} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: BLUESURFELS_STRATEGIES_ABSTRACT_STRATEGY_H_ */
#endif // MINSG_EXT_BLUE_SURFELS