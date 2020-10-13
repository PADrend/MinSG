/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef MAR_SURFELRENDERER_H
#define MAR_SURFELRENDERER_H

#include "../../Core/States/NodeRendererState.h"
#include "../../Core/FrameContext.h"
#include <Geometry/Matrix4x4.h>
#include <set>

namespace MinSG {
namespace MAR {

class SurfelRenderer : public NodeRendererState {
		PROVIDES_TYPE_NAME(MAR::SurfelRenderer)

	public:

		SurfelRenderer(Util::StringIdentifier channel = FrameContext::DEFAULT_CHANNEL)
			: NodeRendererState(channel), surfelCountFac(8), surfelSizeFac(1), maxSurfelSize(10), forceFlag(false){}

		SurfelRenderer(const SurfelRenderer &) = default;

		virtual ~SurfelRenderer() {}

		MINSGAPI virtual NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		float getSurfelCountFactor() const{
			return surfelCountFac;
		}
		void setSurfelCountFactor(float factor) {
			if(factor <= 0)
				throw std::logic_error("surfel count factor has to be greater zero");
			surfelCountFac = factor;
		}

		float getSurfelSizeFactor() const{
			return surfelSizeFac;
		}
		void setSurfelSizeFactor(float factor) {
			if(factor <= 0)
				throw std::logic_error("surfel size factor has to be greater zero");
			surfelSizeFac = factor;
		}
		
		float getMaxAutoSurfelSize() const{
			return maxSurfelSize;
		}
		void setMaxAutoSurfelSize(float size) {
			if(size <= 0)
				throw std::logic_error("surfel size factor has to be greater zero");
			maxSurfelSize = size;
		}

		bool getForceSurfels() const{
			return forceFlag;
		}
		void setForceSurfels(bool force) {
			forceFlag = force;
		}

		State * clone() const override {
			return new SurfelRenderer(*this);
		};

		MINSGAPI Rendering::Mesh * getSurfels(Node * node);
		bool hasSurfels(Node * node) {
			return getSurfels(node) != nullptr;
		}

		MINSGAPI float getSurfelCoverage(Node * node);

		MINSGAPI void displaySurfels(FrameContext & context, Rendering::Mesh * surfelMesh, Geometry::Matrix4x4f worldMatrix, float surfelCount, float surfelSize);

		//! render surfels for forced nodes.
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;

	private:
		float surfelCountFac;
		float surfelSizeFac;
		float maxSurfelSize;
		bool forceFlag;
		std::set<Node *> displayOnDeaktivate;
		
		MINSGAPI float getProjSize(FrameContext & context, Node * node);
};

}
}

#endif // SURFELRENDERER_H

#endif // MINSG_EXT_MULTIALGORENDERING
