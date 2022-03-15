/*
	This file is part of the MinSG library.
	Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_EXT_SKINNINGSTATE_H_
#define MINSG_EXT_SKINNINGSTATE_H_

#include "../../Core/States/State.h"
#include <Rendering/BufferObject.h>

#include <Geometry/Matrix4x4.h>

#include <unordered_map>

namespace Rendering {
class Shader;
} // Rendering

namespace MinSG {

/**
 *  [SkinningState] ---|> [State]
 * @ingroup states
 */
class SkinningState : public State {
		PROVIDES_TYPE_NAME(SkinningState)
	private:
		MINSGAPI stateResult_t doEnableState(FrameContext& context, Node* node, const RenderParam& rp) override;
		MINSGAPI void doDisableState(FrameContext& context, Node* node, const RenderParam& rp) override;

		void updateJointPose(Node* node);
		void recomputeRoot();
	public:
		MINSGAPI SkinningState();
		MINSGAPI virtual ~SkinningState();

		void addJoint(const Util::Reference<Node>& node, const Geometry::Matrix4x4& inverseBindMatrix);

		void setJointMatricesLocation(uint32_t location) { jointMatricesLocation = location; }

		const std::vector<Geometry::Matrix4x4>& getInverseBindMatrices() const { return inverseBindMatrices; }
		const std::vector<Util::Reference<Node>>& getJoints() const { return joints; }
		
		MINSGAPI SkinningState* clone() const override;
	private:
		bool needsUpdate = true;
		bool rootChanged = true;
		uint32_t jointMatricesLocation = 0;
		std::vector<Geometry::Matrix4x4> inverseBindMatrices;
		Rendering::BufferObject jointMatrices;
		std::vector<Util::Reference<Node>> joints;
		Util::Reference<Node> skeletonRoot;
		Util::Reference<Rendering::Shader> metaShader;
};

}

#endif // MINSG_EXT_SKINNINGSTATE_H_
