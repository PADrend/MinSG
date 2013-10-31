/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#include "ViewCells.h"
#include "Definitions.h"
#include "Sample.h"
#include "../ValuatedRegion/ValuatedRegionNode.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Geometry/BoxIntersection.h>
#include <Geometry/Definitions.h>
#include <Geometry/Line.h>
#include <Geometry/RayBoxIntersection.h>
#include <Geometry/Vec3.h>
#include <cstdint>

using namespace MinSG::VisibilitySubdivision;

namespace MinSG {
namespace AGVS {

static VisibilityVector & getOrCreateVisibilityVector(ValuatedRegionNode * viewCell) {
	auto valueList = dynamic_cast<Util::GenericAttributeList *>(viewCell->getValue());
	if(valueList == nullptr) {
		valueList = new Util::GenericAttributeList;
		viewCell->setValue(valueList);
	}
	if(valueList->empty()) {
		valueList->push_back(new VisibilityVectorAttribute);
	}
	auto vvAttribute = dynamic_cast<VisibilityVectorAttribute *>(valueList->front());
	if(vvAttribute == nullptr) {
		vvAttribute = new VisibilityVectorAttribute;
		valueList->clear();
		valueList->push_back(vvAttribute);
	}
	return vvAttribute->ref();
}

void splitViewCell(ValuatedRegionNode * viewCell) {
	if(viewCell->isLeaf() && viewCell->getSize() > 1) {
		viewCell->splitUp(viewCell->getXResolution() > 1 ? 2 : 1,
						  viewCell->getYResolution() > 1 ? 2 : 1,
						  viewCell->getZResolution() > 1 ? 2 : 1);
	}
	const auto children = getChildNodes(viewCell);
	for(const auto & child : children) {
		splitViewCell(static_cast<ValuatedRegionNode *>(child));
	}
}

/**
 * Cast a ray against the view cell hierarchy and return all leaf cells that
 * intersect the ray.
 * 
 * @param rootViewCell Root node of the view cell hierarchy
 * @param ray The ray that is cast
 * @return Array of intersecting view cells
 */
template<typename value_t>
static std::deque<ValuatedRegionNode *> getIntersectingLeafCells(
						ValuatedRegionNode * rootViewCell,
						const Geometry::_Ray<Geometry::_Vec3<value_t>> & ray) {
	struct Visitor {
		const Geometry::Intersection::Slope<value_t> m_slope;
		std::deque<ValuatedRegionNode *> collectedCells;

		Visitor(const Geometry::_Ray<Geometry::_Vec3<value_t>> & p_ray) :
			m_slope(p_ray) {
		}

		void visit(ValuatedRegionNode * cell) {
			if(!m_slope.isRayIntersectingBox(cell->getWorldBB())) {
				return;
			}
			if(cell->isLeaf()) {
				collectedCells.push_back(cell);
			} else {
				const auto children = getChildNodes(cell);
				for(const auto & child : children) {
					visit(static_cast<ValuatedRegionNode *>(child));
				}
			}
		}
	};

	Visitor visitor(ray);
	visitor.visit(rootViewCell);
	return visitor.collectedCells;
}

template<typename value_t>
static void updateLeafCellWithSample(ValuatedRegionNode * viewCell,
									 const ValuatedRegionNode * originCell,
									 const Sample<value_t> & sample,
									 contribution_t & contribution) {
	auto & visiVec = getOrCreateVisibilityVector(viewCell);
	if(sample.hasForwardResult()) {
		const auto forwardResult = sample.getForwardResult();
		const auto oldBenefits = visiVec.increaseBenefits(forwardResult, 1);
		if(oldBenefits == 0) {
			++std::get<0>(contribution);
			if(viewCell == originCell) {
				++std::get<2>(contribution);
			}
		}
	}
	if(sample.hasBackwardResult()) {
		const auto backwardResult = sample.getBackwardResult();
		const auto oldBenefits = visiVec.increaseBenefits(backwardResult, 1);
		if(oldBenefits == 0) {
			++std::get<1>(contribution);
			if(viewCell == originCell) {
				++std::get<2>(contribution);
			}
		}
	}
}

template<typename value_t>
static contribution_t updateCellsWithSample(ValuatedRegionNode * rootViewCell,
											const Sample<value_t> & sample,
											const ValuatedRegionNode * originCell) {
	Geometry::_Ray<Geometry::_Vec3<value_t>> ray = sample.getForwardRay();
	// Let the ray start at the backward intersection point if there is one
	if(sample.hasBackwardResult()) {
		ray.setOrigin(sample.getBackwardTerminationPoint());
	}
	const auto cells = getIntersectingLeafCells(rootViewCell, ray);
	contribution_t contribution(0, 0, 0);
	for(const auto & cell : cells) {
		updateLeafCellWithSample(cell, originCell, sample, contribution);
	}
	return contribution;
}

contribution_t updateWithSample(ValuatedRegionNode * rootViewCell,
								const Sample<float> & sample,
								const ValuatedRegionNode * originCell) {
	return updateCellsWithSample(rootViewCell, sample, originCell);
}

}
}

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
