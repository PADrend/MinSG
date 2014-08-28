/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MirrorState.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include "../../Core/States/LightingState.h"
#include "../../Core/States/TextureState.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>
#include <Geometry/Convert.h>
#include <Geometry/Definitions.h>
#include <Geometry/Rect.h>
#include <Geometry/SRT.h>
#include <Geometry/Vec3.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Draw.h>
#include <Rendering/FBO.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Color.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <cmath>
#include <cstddef>
#include <functional>

namespace MinSG {

MirrorState::MirrorState(uint16_t textureSize) :
	State(),
	texSize(textureSize),
	mirrorTexture(new TextureState(Rendering::TextureUtils::createStdTexture(texSize, texSize, true).get())),
	depthTexture(Rendering::TextureUtils::createDepthTexture(texSize, texSize)),
	fbo(),
	rootNode(nullptr),
	lastCamPos() {
}

MirrorState::~MirrorState() {
}

class MirrorRenderer {
	private:
		Util::Reference<MirrorState> mirrorState;
		Util::Reference<CameraNode> mirrorCamera;
		Util::Reference<GroupNode> displayNode;
		Util::Reference<Node> mirrorNode;

	public:
		MirrorRenderer(MirrorState * _mirrorState, CameraNode * _mirrorCamera, GroupNode * _displayNode, Node * _mirrorNode) :
			mirrorState(_mirrorState), mirrorCamera(_mirrorCamera), displayNode(_displayNode), mirrorNode(_mirrorNode) {
		}

		bool operator()(FrameContext & context) {
			if(!mirrorState->isActive()) {
				return true;
			}

			// ##### Create mirror texture #####
			context.getRenderingContext().pushMatrix_modelToCamera();

			context.getRenderingContext().pushAndSetFBO(mirrorState->getFBO());

			context.pushAndSetCamera(mirrorCamera.get());

			// If displayNode is the root node, no states will be collected.
			const std::deque<LightingState *> lightingStates = collectStatesUpwards<LightingState>(displayNode.get());
			for(auto & lightingState : lightingStates) {
				(lightingState)->enableState(context, nullptr, 0);
			}

			// Clear depth buffer.
			context.getRenderingContext().clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f));

			mirrorNode->deactivate();
			mirrorState->deactivate();
			context.displayNode(displayNode.get(), FRUSTUM_CULLING);
			mirrorState->activate();
			mirrorNode->activate();

			// If no states have been collected, the loop body will not be executed.
			for(auto & lightingState : lightingStates) {
				(lightingState)->disableState(context, nullptr, 0);
			}

			context.getRenderingContext().popFBO();

			// Restore.
			context.popCamera();
			context.getRenderingContext().popMatrix_modelToCamera();
			return true;
		}
};

State::stateResult_t MirrorState::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	// Bind textures to FBO.
	if(fbo.isNull()) {
		fbo = new Rendering::FBO();
		context.getRenderingContext().pushAndSetFBO(fbo.get());
		fbo->attachColorTexture(context.getRenderingContext(), mirrorTexture->getTexture());
		fbo->attachDepthTexture(context.getRenderingContext(), depthTexture.get());
		context.getRenderingContext().popFBO();
	}

	// ##### Calculations for new frustum #####

	const Geometry::Vec3 camPos = context.getCamera()->getWorldOrigin();

	const Geometry::Box & box = node->getWorldBB();

	Geometry::Vec3 frameTopLeft;
	Geometry::Vec3 frameBottomLeft;
	Geometry::Vec3 frameBottomRight;
	Geometry::Vec3 frameNormal;

	// Get main axis for bounding box to determine the frame that will be used as frustum.
	float minExtent = box.getExtent(static_cast<Geometry::dimension_t>(0));
	uint8_t minDim = 0;
	for (uint_fast8_t dim = 1; dim < 3; ++dim) {
		const float extent = box.getExtent(static_cast<Geometry::dimension_t>(dim));
		if (extent < minExtent) {
			minExtent = extent;
			minDim = dim;
		}
	}

	// Check opposing sides.
	bool sideFound = false;
	for (uint_fast8_t s = minDim; s < 6; s += 3) {
		const Geometry::side_t side = static_cast<Geometry::side_t>(s);
		const Geometry::Vec3 & sideNormal = Geometry::Helper::getNormal(side);
		const Geometry::Vec3 & sidePos = box.getCorner(Geometry::Helper::getCornerIndices(side)[0]);
		float result = camPos.planeTest(sidePos, sideNormal);
		if (result > 0) {
			// Camera on this side of the box.
			sideFound = true;
			frameTopLeft = box.getCorner(Geometry::Helper::getCornerIndices(side)[3]);
			frameBottomLeft = box.getCorner(Geometry::Helper::getCornerIndices(side)[0]);
			frameBottomRight = box.getCorner(Geometry::Helper::getCornerIndices(side)[1]);
			frameNormal = sideNormal;
			break;
		}
	}
	if (!sideFound) {
		WARN("Camera inside mirror.");
		return State::STATE_SKIPPED;
	}

	// Mirror camera position at plane.
	const Geometry::Vec3 camDist = camPos.getProjection(frameBottomLeft, frameNormal) - camPos;
	const Geometry::Vec3 mirrorPos = camPos + camDist * 2.0f;

	// - change orientation of the camera to orthogonally face the projection plane
	Geometry::SRT cameraSRT(mirrorPos, camDist, (frameTopLeft - frameBottomLeft));
	Util::Reference<CameraNode> camera = new CameraNode;
	camera->setRelTransformation(cameraSRT);

	// - calculate corners relative to the camera
	const Geometry::SRT invCameraSRT = cameraSRT.inverse();
	const Geometry::Vec3 frameTopLeft_rotated = invCameraSRT * frameTopLeft;
	const Geometry::Vec3 frameBottomRight_rotated = invCameraSRT * frameBottomRight;

	// - calculate and set the angles of the frustum
	// switch left/right, because we are looking from behind the mirror now
	const float distanceToPlane = frameTopLeft_rotated.getZ();
	const float leftAngle = -std::atan(frameBottomRight_rotated.getX() / distanceToPlane);
	const float rightAngle = -std::atan(frameTopLeft_rotated.getX() / distanceToPlane);
	const float bottomAngle = -std::atan(frameBottomRight_rotated.getY() / distanceToPlane);
	const float topAngle = -std::atan(frameTopLeft_rotated.getY() / distanceToPlane);
	camera->setAngles(Geometry::Convert::radToDeg(leftAngle), Geometry::Convert::radToDeg(rightAngle),
					  Geometry::Convert::radToDeg(bottomAngle), Geometry::Convert::radToDeg(topAngle));

	const float near = camDist.length();
	const float far = near + 100.0f;
	camera->setNearFar(near, far);
	camera->setViewport(Geometry::Rect_i(0, 0, texSize, texSize));

	GroupNode * displayNode = rootNode;
	if(displayNode == nullptr) {
		displayNode = node->getParent();
		while(displayNode->hasParent()) {
			displayNode = displayNode->getParent();
		}
	}

	if(camPos != lastCamPos) {
		// Only skip the rendering process here.
		// If the whole calculations would be skipped, the meta objects could not be displayed below.
		lastCamPos = camPos;
		context.addBeginFrameListener(MirrorRenderer(this, camera.get(), displayNode, node));
	}

	if (rp.getFlag(SHOW_META_OBJECTS)) {
		Rendering::RenderingContext & renderingContext = context.getRenderingContext();
		renderingContext.pushAndSetMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() );
		Rendering::drawVector(renderingContext, frameTopLeft, camera->getWorldOrigin(), Util::ColorLibrary::BLUE);
		Rendering::drawVector(renderingContext, frameBottomRight, camera->getWorldOrigin(), Util::ColorLibrary::BLUE);
		renderingContext.popMatrix_modelToCamera();

		camera->display(context, rp + USE_WORLD_MATRIX);
	}

	// ##### Activate mirror texture #####
	return mirrorTexture->enableState(context, node, rp);
}

void MirrorState::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	mirrorTexture->disableState(context, node, rp);
}

MirrorState * MirrorState::clone() const {
	// Implementation cannot be prevented.
	FAIL();
	return nullptr;
}

}
