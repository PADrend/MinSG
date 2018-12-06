/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "CommonStrategies.h"

#include <MinSG/Core/FrameContext.h>
#include <MinSG/Core/Transformations.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/Nodes/AbstractCameraNode.h>
#include <MinSG/Ext/BlueSurfels/SurfelAnalysis.h>

#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/MeshDataStrategy.h>

#include <Geometry/Tools.h>

#include <limits>

namespace MinSG {
namespace BlueSurfels {
  
// ------------------------
// FixedSizeStrategy
    
bool FixedSizeStrategy::update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel) {
  surfel.pointSize = getSize();
	float r = sizeToRadius(surfel.pointSize, surfel.relPixelSize);
  surfel.prefix = getPrefixForRadius(r, surfel.packing);
  if(surfel.prefix > surfel.maxPrefix) {
    surfel.prefix = 0;
    return false;
  }
  return true;
}
  
// ------------------------
// FixedCountStrategy
    
bool FixedCountStrategy::update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel) {
  surfel.prefix = std::min(surfel.maxPrefix, getCount());
  return surfel.prefix > 0;
}
  
// ------------------------
// FactorStrategy
    
bool FactorStrategy::update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel) {
  surfel.prefix *= getCountFactor();
  surfel.pointSize *= getSizeFactor();
  return false;
}


// ------------------------
// BlendStrategy

bool BlendStrategy::update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel) {  
	float r = MinSG::BlueSurfels::sizeToRadius(surfel.pointSize, surfel.relPixelSize);
  surfel.prefix = getPrefixForRadius(r, surfel.packing);  
  if(surfel.prefix > surfel.maxPrefix) {
    if(getBlend() > 0.0f) {
      uint32_t diff = std::min<uint32_t>((surfel.prefix - surfel.maxPrefix) / getBlend(), surfel.maxPrefix);
      surfel.prefix = surfel.maxPrefix - diff;
    } else {
      surfel.prefix = 0;
    }
    return false;
  }
  return true;
}

// ------------------------
// DebugStrategy

DebugStrategy::DebugStrategy() : AbstractSurfelStrategy(50000) {}

bool DebugStrategy::prepare(MinSG::FrameContext& context, MinSG::Node* node) {
  if(getFixSurfels() && debugCamera.isNull()) {
    debugCamera = static_cast<MinSG::AbstractCameraNode*>(context.getCamera()->clone());
    debugCamera->setWorldTransformation(context.getCamera()->getWorldTransformationSRT());
  }
  return false;
}

bool DebugStrategy::update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel) {  
  if(getFixSurfels()) 
    surfel.relPixelSize = MinSG::BlueSurfels::computeRelPixelSize(debugCamera.get(), node);
  return false;
}

bool DebugStrategy::beforeRendering(MinSG::FrameContext& context) { 
  return getHideSurfels(); 
}

void DebugStrategy::setFixSurfels(bool value) { debugCamera = nullptr; fixSurfels = value; }

// ------------------------------------------------------------------------
// Import/Export

static const Util::StringIdentifier ATTR_SIZE("size");
static const Util::StringIdentifier ATTR_COUNT("count");
static const Util::StringIdentifier ATTR_BLEND("blend");

static void exportFixedSizeStrategy(Util::GenericAttributeMap& desc, AbstractSurfelStrategy* _strategy) {
  const auto& strategy = dynamic_cast<FixedSizeStrategy*>(_strategy);
  desc.setValue(ATTR_SIZE, Util::GenericAttribute::createNumber(strategy->getSize()));
}

static AbstractSurfelStrategy* importFixedSizeStrategy(const Util::GenericAttributeMap* desc) {
  auto strategy = new FixedSizeStrategy;
  strategy->setSize(desc->getFloat(ATTR_SIZE, 2));
  return strategy;
}

static void exportFixedCountStrategy(Util::GenericAttributeMap& desc, AbstractSurfelStrategy* _strategy) {
  const auto& strategy = dynamic_cast<FixedCountStrategy*>(_strategy);
  desc.setValue(ATTR_COUNT, Util::GenericAttribute::createNumber(strategy->getCount()));
}

static AbstractSurfelStrategy* importFixedCountStrategy(const Util::GenericAttributeMap* desc) {
  auto strategy = new FixedCountStrategy;
  strategy->setCount(desc->getUInt(ATTR_COUNT, 0));
  return strategy;
}

static void exportFactorStrategy(Util::GenericAttributeMap& desc, AbstractSurfelStrategy* _strategy) {
  const auto& strategy = dynamic_cast<FactorStrategy*>(_strategy);
  desc.setValue(ATTR_COUNT, Util::GenericAttribute::createNumber(strategy->getCountFactor()));
  desc.setValue(ATTR_SIZE, Util::GenericAttribute::createNumber(strategy->getSizeFactor()));
}

static AbstractSurfelStrategy* importFactorStrategy(const Util::GenericAttributeMap* desc) {
  auto strategy = new FactorStrategy;
  strategy->setCountFactor(desc->getFloat(ATTR_COUNT, 1));
  strategy->setSizeFactor(desc->getFloat(ATTR_SIZE, 1));
  return strategy;
}

static void exportBlendStrategy(Util::GenericAttributeMap& desc, AbstractSurfelStrategy* _strategy) {
  const auto& strategy = dynamic_cast<BlendStrategy*>(_strategy);
  desc.setValue(ATTR_BLEND, Util::GenericAttribute::createNumber(strategy->getBlend()));
}

static AbstractSurfelStrategy* importBlendStrategy(const Util::GenericAttributeMap* desc) {
  auto strategy = new BlendStrategy;
  strategy->setBlend(desc->getFloat(ATTR_BLEND, 0.3));
  return strategy;
}

static void exportDebugStrategy(Util::GenericAttributeMap& desc, AbstractSurfelStrategy* _strategy) { }

static AbstractSurfelStrategy* importDebugStrategy(const Util::GenericAttributeMap* desc) {
  auto strategy = new DebugStrategy;
  return strategy;
}

static bool importerFixedSizeRegistered = registerImporter(FixedSizeStrategy::getClassId(), &importFixedSizeStrategy);
static bool exporterFixedSizeRegistered = registerExporter(FixedSizeStrategy::getClassId(), &exportFixedSizeStrategy);
static bool importerFixedCountRegistered = registerImporter(FixedCountStrategy::getClassId(), &importFixedCountStrategy);
static bool exporterFixedCountRegistered = registerExporter(FixedCountStrategy::getClassId(), &exportFixedCountStrategy);
static bool importerCountFactorRegistered = registerImporter(FactorStrategy::getClassId(), &importFactorStrategy);
static bool exporterCountFactorRegistered = registerExporter(FactorStrategy::getClassId(), &exportFactorStrategy);
static bool importerBlendRegistered = registerImporter(BlendStrategy::getClassId(), &importBlendStrategy);
static bool exporterBlendRegistered = registerExporter(BlendStrategy::getClassId(), &exportBlendStrategy);
static bool importerDebugRegistered = registerImporter(DebugStrategy::getClassId(), &importDebugStrategy);
static bool exporterDebugRegistered = registerExporter(DebugStrategy::getClassId(), &exportDebugStrategy);

} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS