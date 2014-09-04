/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION

#include "PVSRenderer.h"
#include "VisibilityVector.h"
#include "../ValuatedRegion/ValuatedRegionNode.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/FrustumTest.h"
#include <stdexcept>

namespace MinSG {
namespace VisibilitySubdivision {

static const VisibilityVector & getVV(const PVSRenderer::cell_ptr node) {
	if (node->getValue() == nullptr) {
		throw std::logic_error("Given node has no value.");
	}
	auto gal = dynamic_cast<Util::GenericAttributeList *>(node->getValue());
	if (gal == nullptr) {
		throw std::logic_error("Given node does not have a GenericAttributeList.");
	}
	auto * vva = dynamic_cast<VisibilityVectorAttribute *>(gal->front());
	if (vva == nullptr) {
		throw std::logic_error("List entry is no VisibilityVectorAttribute.");
	}
	return vva->ref();
}

PVSRenderer::PVSRenderer() :
	State(), viewCells(nullptr), lastViewCell(nullptr) {
}

PVSRenderer::PVSRenderer(const PVSRenderer &) = default;
PVSRenderer::~PVSRenderer() = default;

PVSRenderer * PVSRenderer::clone() const {
	return new PVSRenderer(*this);
}

State::stateResult_t PVSRenderer::doEnableState(FrameContext & context,
												Node *,
												const RenderParam & rp) {
	if(rp.getFlag(SKIP_RENDERER)) {
		return State::STATE_SKIPPED;
	}

	if(viewCells == nullptr) {
		// Invalid information. => Fall back to standard rendering.
		return State::STATE_SKIPPED;
	}

	const auto pos = context.getCamera()->getWorldOrigin();
	// Check if cached cell can be used.
	if(lastViewCell == nullptr || !lastViewCell->getBB().contains(pos)) {
		lastViewCell = viewCells->getNodeAtPosition(pos);
	}
	if(lastViewCell == nullptr || !lastViewCell->isLeaf()) {
		// Invalid information. => Fall back to standard rendering.
		return State::STATE_SKIPPED;
	}

	try {
		const auto & frustum = context.getCamera()->getFrustum();
		const auto & vv = getVV(lastViewCell);
		const uint32_t maxIndex = vv.getIndexCount();
		for(uint_fast32_t index = 0; index < maxIndex; ++index) {
			const auto object = vv.getNode(index);
			if(conditionalFrustumTest(frustum, object->getWorldBB(), rp)) {
				context.displayNode(object, rp);
			}
		}
	} catch(...) {
		// Invalid information. => Fall back to standard rendering.
		return State::STATE_SKIPPED;
	}

	return State::STATE_SKIP_RENDERING;
}

}
}

#endif // MINSG_EXT_VISIBILITY_SUBDIVISION
