/*
	This file is part of the MinSG library.
	Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "SkinningState.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Transformations.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Draw.h>
#include <Rendering/DrawCompound.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Shader/ShaderUtils.h>
#include <Geometry/Sphere.h>
#include <Geometry/Frustum.h>
#include <Geometry/Angle.h>
#include <Geometry/VecHelper.h>
#include <Util/Macros.h>
#include <Util/Graphics/Color.h>

namespace MinSG {
using namespace Rendering;
using namespace Geometry;

static const Uniform SKINNING_ENABLED({"sg_SkinningEnabled"}, true);
static const Uniform SKINNING_DISABLED({"sg_SkinningEnabled"}, false);

/**
 * [ctor]
 */
SkinningState::SkinningState() : State(), metaShader(ShaderUtils::createDefaultShader()) {
	//ctor
}

/**
 * [dtor]
 */
SkinningState::~SkinningState() {
	//dtor
}

/**
 * ---|> [State]
 */
SkinningState* SkinningState::clone() const {
	return new SkinningState();
}


/**
 * ---|> [State]
 */
State::stateResult_t SkinningState::doEnableState(FrameContext& context, Node* node, const RenderParam& rp) {

	if (rp.getFlag(SKIP_RENDERER))
		return State::STATE_SKIPPED;
	if(rootChanged)
		recomputeRoot();
	if(needsUpdate)
		updateJointPose(node);
	jointMatrices.bind(BufferObject::TARGET_SHADER_STORAGE_BUFFER, jointMatricesLocation);
	context.getRenderingContext().setGlobalUniform(SKINNING_ENABLED);
	return State::STATE_OK;
}


/**
 * ---|> [State]
 */
void SkinningState::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	jointMatrices.unbind(BufferObject::TARGET_SHADER_STORAGE_BUFFER, jointMatricesLocation);
	context.getRenderingContext().setGlobalUniform(SKINNING_DISABLED);
	
	if(rp.getFlag(RenderFlags::SHOW_META_OBJECTS)) {
		auto& rc = context.getRenderingContext();
		rc.pushAndSetShader(metaShader.get());
		rc.pushAndSetMatrix_modelToCamera( rc.getMatrix_worldToCamera() );
		rc.pushAndSetDepthBuffer(DepthBufferParameters(false, false, Comparison::LESS));
		
		Matrix4x4 skeletonMatrix = node->getWorldTransformationMatrix() * skeletonRoot->getWorldTransformationMatrix().inverse();
		for(uint32_t i=0; i<joints.size(); ++i) {
			auto parent = joints[i]->getParent();

			Geometry::Vec3 pos = joints[i]->getWorldOrigin();
			Geometry::Vec3 parentPos = parent->getWorldOrigin();
			Geometry::Vec3 parentRight = Transformations::localDirToWorldDir(*parent, {1,0,0});
			float dist = parentPos.distance(pos);
			if(dist < 1.0e-9)
				continue;

			Geometry::Vec3 dir = (parentPos - pos).getNormalized();
			//Geometry::Vec3 up = dir.cross(parentRight);
			Geometry::Vec3 up = Helper::createOrthogonal(dir);

			pos = skeletonMatrix.transformPosition(pos);
			dir = skeletonMatrix.transformDirection(dir);
			up = skeletonMatrix.transformDirection(up);

			if(joints[i]->getParent() == skeletonRoot.get()) {
				Sphere s(pos, node->getBB().getDiameter() * 0.01f);
				Util::Color4f c;
				drawWireframeSphere(rc, s, c);
			} else {
				Sphere s(pos, dist * 0.1f);
				Frustum f;
				f.setPosition(pos, dir, up);
				f.setPerspective(Angle::deg(5), 1.0, dist*0.1f, dist*0.9f);
				Util::Color4f c;
				drawWireframeSphere(rc, s, c);
				drawFrustum(rc, f, c, 1);
			}
		}
		rc.popDepthBuffer();
		rc.popMatrix_modelToCamera();
		rc.popShader();
	}
}

void SkinningState::addJoint(const Util::Reference<Node>& node, const Matrix4x4& inverseBindMatrix) {
	needsUpdate = true;
	rootChanged = true;
	joints.emplace_back(node);
	inverseBindMatrices.emplace_back(inverseBindMatrix);
}

void SkinningState::updateJointPose(Node* node) {

	jointMatrices.allocateData<Matrix4x4>(BufferObject::TARGET_SHADER_STORAGE_BUFFER, joints.size(), BufferObject::USAGE_DYNAMIC_DRAW); // orphans old buffer

	Matrix4x4 inverseWorldMatrix = node->getWorldTransformationMatrix().inverse();
	Matrix4x4* mat = reinterpret_cast<Matrix4x4*>(jointMatrices.map(0, 0, BufferObject::AccessFlag::WRITE_ONLY));
	if(mat) {
		for(uint32_t i=0; i<joints.size(); ++i) {
			Matrix4x4 jointMatrix = inverseWorldMatrix * joints[i]->getWorldTransformationMatrix() * inverseBindMatrices[i];
			*mat = jointMatrix.transpose();
			++mat;
		}
		jointMatrices.unmap();
	}
	needsUpdate = false;
}

void SkinningState::recomputeRoot() {
	if(skeletonRoot) {
		skeletonRoot->clearTransformationObservers(); // !this might clear other transformation observers!
	}
	// find common ancestor of all joint
	skeletonRoot = findCommonAncestor(joints.begin(), joints.end());
	if(skeletonRoot) {
		if(skeletonRoot->getParent())
			skeletonRoot = skeletonRoot->getParent();
		skeletonRoot->addTransformationObserver([this](Node*) {
			needsUpdate = true;
		});
	}
}

}
