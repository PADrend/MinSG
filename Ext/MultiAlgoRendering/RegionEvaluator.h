/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef MAR_REGIONEVALUATOR_H
#define MAR_REGIONEVALUATOR_H

#include "Region.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Geometry/BoxIntersection.h>
#include <Util/GenericAttribute.h>
#include <Util/Graphics/Color.h>
#include <Util/AttributeProvider.h>
#include <Util/ReferenceCounter.h>

#include <deque>
#include <map>
#include <string>
#include <queue>
#include <set>
#include <list>
#include <cstdlib>

namespace MinSG {
namespace MAR {

class RegionEvaluator : public Util::AttributeProvider,public Util::ReferenceCounter<RegionEvaluator> {
		PROVIDES_TYPE_NAME(RegionEvaluator)

	protected:
		class PrioSplit {
			public:
				PrioSplit() :
					prio(0.0f), ratio(0.5f) {
				}

				PrioSplit(float p, float r) :
					prio(p), ratio(r) {
				}
				float prio;
				float ratio;
		};

	protected:

		MINSGAPI static const Util::Color4ub colorFinished;
		MINSGAPI static const Util::Color4ub colorScheduled;
		MINSGAPI static const Util::Color4ub colorActive;

		typedef std::map<std::string, RegionEvaluator *> EvaluatorMap;
		typedef std::set<Region *, bool( *)(const Region *, const Region *)> RegionPrioQueue;
		RegionPrioQueue * evalQueue;
		Util::Reference<GroupNode> sceneRoot;

		RegionEvaluator() :
			Util::AttributeProvider(),Util::ReferenceCounter<RegionEvaluator>(), sceneRoot(nullptr) {
		}

		virtual void init(Region * region)=0;
		virtual void evaluate(Region *)=0;

		float countPolygons(const Geometry::Box & box) {
			const auto geoNodes = collectGeoNodesIntersectingBox(sceneRoot.get(), box);
			float approx = 0;
			for(const auto & geoNode : geoNodes) {
				const auto b = Geometry::Intersection::getBoxBoxIntersection(geoNode->getWorldBB(), box);
				if(b.isValid() && b.getExtentMin() > 0)
					approx += geoNode->getTriangleCount() / geoNode->getWorldBB().getVolume() * b.getVolume();
			}
			return approx;
		}

	public:

		virtual ~RegionEvaluator() {
		}

		void init(Region * region, GroupNode * scene, Util::GenericAttributeMap * newProperties) {
			setAttributes(newProperties);
			sceneRoot = scene;
			init(region);
		}

		uint32_t next(size_t count) {

			while(--count > 0 && !evalQueue->empty()) {
				Region * r = *(evalQueue->begin());
				evalQueue->erase(evalQueue->begin());
				evaluate(r);
			}

			return evalQueue->size();
		}
};

class PolygonDensityEvaluator: public RegionEvaluator {

	public:

		MINSGAPI static bool compare(const Region * a, const Region * b);

		PolygonDensityEvaluator() {
			evalQueue = new RegionPrioQueue(compare);
		}

	protected:

		MINSGAPI virtual void evaluate(Region * r) override;
		MINSGAPI virtual void init(Region * r) override;

		MINSGAPI void calcPriority(Region * r);
		MINSGAPI PrioSplit calcPriority(const std::vector<Geometry::Box> & r);
		MINSGAPI float calcDensity(const Geometry::Box & b);

	private:

		uint32_t regionCount;
};

class RegionSizeEvaluator: public RegionEvaluator {

	public:

		MINSGAPI static bool compare(const Region * a, const Region * b);

		RegionSizeEvaluator() {
			evalQueue = new RegionPrioQueue(compare);
		}

	protected:

		MINSGAPI virtual void evaluate(Region * r) override;
		MINSGAPI virtual void init(Region * r) override;

	private:

};

class PolygonCountEvaluator: public RegionEvaluator {

	public:

		MINSGAPI static bool compare(const Region * a, const Region * b);

		PolygonCountEvaluator() {
			evalQueue = new RegionPrioQueue(compare);
		}

	protected:

		MINSGAPI virtual void evaluate(Region * r) override;
		MINSGAPI virtual void init(Region * r) override;

	private:

};

}
}
#endif // REGIONEVALUATOR_H
#endif // MINSG_EXT_MULTIALGORENDERING
