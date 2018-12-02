/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "FoveatedStrategy.h"

#include <MinSG/Core/FrameContext.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/Nodes/AbstractCameraNode.h>
#include <MinSG/Ext/BlueSurfels/SurfelAnalysis.h>

#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>

#include <limits>

namespace MinSG {
namespace BlueSurfels {

static Geometry::Vec2 closestRect(const Geometry::Rect& rect, const Geometry::Vec2& pos) {
  Geometry::Vec2 out(pos);
  if(pos.x() < rect.getMinX())
    out.x(rect.getMinX());
  else if(pos.x() > rect.getMaxX())
    out.x(rect.getMaxX());
  if(pos.y() < rect.getMinY())
    out.y(rect.getMinY());
  else if(pos.y() > rect.getMaxY())
    out.y(rect.getMaxY());
  return out;
}

bool FoveatedStrategy::update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel) {
	const auto viewport = context.getCamera()->getViewport();
	Geometry::Vec2 vpCenter(viewport.getCenter());
  vpCenter += getOffset();
	auto projRect = context.getProjectedRect(node);
	float dist = closestRect(projRect, vpCenter).distance(vpCenter);
  if(dist <= 0)
    return false;
      
	float maxDist = std::sqrt(viewport.getWidth()*viewport.getWidth() + viewport.getHeight()*viewport.getHeight()) * 0.5f;
	float z1 = 0;
	float z2 = maxDist;
	float c1 = surfel.pointSize;
	float c2 = surfel.pointSize;
  // find interval
  for(const auto& z : foveaZones) {
    if(dist < z.first * maxDist) {
      z2 = z.first * maxDist;
      c2 = z.second * surfel.pointSize;
      break;
    } else {
			z1 = z.first * maxDist;
			c1 = z.second * surfel.pointSize;
    }
  }
	float a = std::min((dist-z1) / (z2-z1), 1.0f);
	surfel.pointSize = (1.0f - a) * c1 + a * c2;	
	float r = sizeToRadius(surfel.pointSize, surfel.mpp);
  surfel.prefix = getPrefixForRadius(r, surfel.packing);
  if(surfel.prefix > surfel.maxPrefix) {
    surfel.prefix = 0;
    return false;
  }
  return true;
}

// ------------------------------------------------------------------------
// Import/Export

static const Util::StringIdentifier ATTR_OFFSET("offset");
static const Util::StringIdentifier ATTR_FOVEA_ZONES("foveaZones");

static void exportFoveatedStrategy(Util::GenericAttributeMap& desc, AbstractSurfelStrategy* _strategy) {
  const auto& strategy = dynamic_cast<FoveatedStrategy*>(_strategy);
  
	const Geometry::Vec2 offset = strategy->getOffset();
	std::ostringstream s;
	s << offset.getX() << " " << offset.getY();
	desc.setString(ATTR_OFFSET, s.str());
  
	std::ostringstream zones;
  for(const auto& z : strategy->getFoveaZones()) {
    zones << z.first << " " << z.second << " ";
  }
	desc.setString(ATTR_FOVEA_ZONES, Util::StringUtils::trim(zones.str()));
}

static AbstractSurfelStrategy* importFoveatedStrategy(const Util::GenericAttributeMap* desc) {
  auto strategy = new FoveatedStrategy;
  
  float x,y;
  std::istringstream s(desc->getString(ATTR_OFFSET, "0 0"));
  s >> x >> y;
  strategy->setOffset({x,y});
  
  std::istringstream zones(Util::StringUtils::trim(desc->getString(ATTR_FOVEA_ZONES, "")));
  while(!zones.eof()) {
    float pos, scale;
    zones >> pos >> scale; // TODO: check if we can actually consume two floats
    strategy->addFoveaZone(pos, scale);
  }
  return strategy;
}

static bool importerRegistered = registerImporter(FoveatedStrategy::getClassId(), &importFoveatedStrategy);
static bool exporterRegistered = registerExporter(FoveatedStrategy::getClassId(), &exportFoveatedStrategy);

} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS