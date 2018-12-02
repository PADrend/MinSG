/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "AdaptiveStrategy.h"

#include <MinSG/Core/FrameContext.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Ext/BlueSurfels/SurfelAnalysis.h>

#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>

#include <Rendering/RenderingContext/RenderingContext.h>

#include <limits>

namespace MinSG {
namespace BlueSurfels {
	
bool AdaptiveStrategy::prepare(MinSG::FrameContext& context, MinSG::Node* node) {
	timer.reset();
	return false;
}
	
bool AdaptiveStrategy::update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel) {  
  surfel.pointSize = size;
	float r = sizeToRadius(surfel.pointSize, surfel.mpp);
  surfel.prefix = getPrefixForRadius(r, surfel.packing);
  if(surfel.prefix > surfel.maxPrefix) {
    surfel.prefix = 0;
    return false;
  }
  return true;
}

void AdaptiveStrategy::afterRendering(MinSG::FrameContext& context) {
	context.getRenderingContext().finish();
	float factor = timer.getMilliseconds() / getTargetTime();
	if(factor > 1.1f || factor < 0.9f) {
		size = std::min(std::max((3.0f*size + size*factor)/4.0f, 1.0f), getMaxSize());
	}
}

// ------------------------------------------------------------------------
// Import/Export

static const Util::StringIdentifier ATTR_MAX_SIZE("maxSize");
static const Util::StringIdentifier ATTR_TARGET_TIME("targetTime");

static void exportAdaptiveStrategy(Util::GenericAttributeMap& desc, AbstractSurfelStrategy* _strategy) {
  const auto& strategy = dynamic_cast<AdaptiveStrategy*>(_strategy);
  desc.setValue(ATTR_MAX_SIZE, Util::GenericAttribute::createNumber(strategy->getMaxSize()));
  desc.setValue(ATTR_TARGET_TIME, Util::GenericAttribute::createNumber(strategy->getTargetTime()));
}

static AbstractSurfelStrategy* importAdaptiveStrategy(const Util::GenericAttributeMap* desc) {
  auto strategy = new AdaptiveStrategy;
  strategy->setMaxSize(desc->getFloat(ATTR_MAX_SIZE, 8));
  strategy->setTargetTime(desc->getFloat(ATTR_TARGET_TIME, 16));
  return strategy;
}

static bool importerRegistered = registerImporter(AdaptiveStrategy::getClassId(), &importAdaptiveStrategy);
static bool exporterRegistered = registerExporter(AdaptiveStrategy::getClassId(), &exportAdaptiveStrategy);

} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS