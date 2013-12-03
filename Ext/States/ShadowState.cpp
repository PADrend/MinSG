/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShadowState.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/States/TextureState.h"
#include "../../Core/FrameContext.h"
#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>
#include <Geometry/Convert.h>
#include <Geometry/Definitions.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Plane.h>
#include <Geometry/Rect.h>
#include <Geometry/SRT.h>
#include <Geometry/Vec3.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/FBO.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace MinSG {

ShadowState::ShadowState(uint16_t textureSize) :
		texSize(textureSize),
		shadowTexture(new TextureState(Rendering::TextureUtils::createDepthTexture(texSize, texSize))),
		fbo(new Rendering::FBO),
		light(nullptr) {
	shadowTexture->setTextureUnit(6);
}

ShadowState::~ShadowState() {
}

State::stateResult_t ShadowState::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	if (light == nullptr) {
		return State::STATE_SKIPPED;
	}

	const Geometry::Vec3f camPos = light->getWorldPosition();
	const Geometry::Box & box = node->getWorldBB();
	const Geometry::Vec3f boxCenter = box.getCenter();

	// ##### Fit bounding box into frustum #####
	if(box.contains(camPos)) {
		return State::STATE_SKIPPED;
	}

	const Geometry::Vec3f camDir = (boxCenter - camPos).getNormalized();

	// Determine the normal of a box side to which the viewing direction is "most" orthogonal.
	Geometry::Vec3 orthoNormal = Geometry::Helper::getNormal(static_cast<Geometry::side_t>(0));
	float minAbsCosAngle = std::abs(orthoNormal.dot(camDir));
	for (uint_fast8_t s = 1; s < 6; ++s) {
		const Geometry::side_t side = static_cast<Geometry::side_t>(s);

		const Geometry::Vec3 & normal = Geometry::Helper::getNormal(side);
		const float absCosAngle = std::abs(normal.dot(camDir));

		if(absCosAngle < minAbsCosAngle) {
			minAbsCosAngle = absCosAngle;
			orthoNormal = normal;
		}
	}

	// Use "best" normal vector for up vector calculation.
	const Geometry::Vec3f camRight = camDir.cross(orthoNormal).normalize();
	const Geometry::Vec3f camUp = camRight.cross(camDir).normalize();

	Util::Reference<CameraNode> camera = new CameraNode;
	camera->setSRT(Geometry::SRT(camPos, -camDir, camUp));

	// Calculate minimum and maximum distance of all bounding box corners to camera.
	const Geometry::Plane camPlane(camPos, camDir);
	float minDistance = std::numeric_limits<float>::max();
	float maxDistance = 0.0f;
	// Calculate maximum angles in four directions between camera direction and all bounding box corners.
	float leftAngle = 0.0f;
	float rightAngle = 0.0f;
	float topAngle = 0.0f;
	float bottomAngle = 0.0f;
	const Geometry::Matrix4x4f cameraMatrix = camera->getWorldMatrix().inverse();
	for (uint_fast8_t c = 0; c < 8; ++c) {
		const Geometry::corner_t corner = static_cast<Geometry::corner_t>(c);
		const Geometry::Vec3f cornerPos = box.getCorner(corner);

		const float distance = camPlane.planeTest(cornerPos);
		minDistance = std::min(minDistance, distance);
		maxDistance = std::max(maxDistance, distance);

		const Geometry::Vec3f camSpacePoint = cameraMatrix.transformPosition(cornerPos);
		const float horizontalAngle = Geometry::Convert::radToDeg(std::atan(camSpacePoint.getX() / -camSpacePoint.getZ()));
		const float verticalAngle = Geometry::Convert::radToDeg(std::atan(-camSpacePoint.getY() / -camSpacePoint.getZ()));
		if(camSpacePoint.getX() < 0.0f) {
			leftAngle = std::min(leftAngle, horizontalAngle);
		} else {
			rightAngle = std::max(rightAngle, horizontalAngle);
		}
		if(camSpacePoint.getY() < 0.0f) {
			bottomAngle = std::max(bottomAngle, verticalAngle);
		} else {
			topAngle = std::min(topAngle, verticalAngle);
		}
	}

	// Make sure that the near plane is not behind the camera.
	minDistance = std::max(0.001f, minDistance);

	camera->setViewport(Geometry::Rect_i(0, 0, texSize, texSize));
	camera->setNearFar(minDistance, maxDistance);
	camera->setAngles(leftAngle, rightAngle, bottomAngle, topAngle);

	// ##### Create shadow texture #####
	// Disable color buffer.
	context.getRenderingContext().pushAndSetColorBuffer(Rendering::ColorBufferParameters(false, false, false, false));
	context.getRenderingContext().pushAndSetCullFace(Rendering::CullFaceParameters::CULL_BACK);
	context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::LESS));
	context.getRenderingContext().pushAndSetLighting(Rendering::LightingParameters(false));
	context.getRenderingContext().pushShader();
	context.getRenderingContext().setShader(nullptr);

	context.getRenderingContext().applyChanges();

	context.getRenderingContext().pushFBO();
	context.getRenderingContext().setFBO(fbo.get());
	// Bind texture to FBO.
	fbo->attachDepthTexture(context.getRenderingContext(), shadowTexture->getTexture());

	context.pushAndSetCamera(camera.get());

	// Clear depth buffer.
	context.getRenderingContext().clearDepth(1.0f);

	// Save matrices.
	const Geometry::Matrix4x4f lightProjectionMatrix = context.getRenderingContext().getProjectionMatrix();
	const Geometry::Matrix4x4f lightModelViewMatrix = context.getRenderingContext().getMatrix();

	context.getRenderingContext().pushAndSetPolygonOffset(Rendering::PolygonOffsetParameters(1.1f, 4.0f));

	context.displayNode(node, NO_STATES);

	context.getRenderingContext().popPolygonOffset();

	context.getRenderingContext().popFBO();

//	// draw depth texture to screen for debugging
//	Rendering::TextureUtils::drawTextureToScreen(context.getRenderingContext(),Geometry::Rect_i(0,0,512,512),getTexture(),Geometry::Rect(0,0,1,1));

	context.getRenderingContext().popShader();
	context.getRenderingContext().popLighting();
	context.getRenderingContext().popDepthBuffer();
	context.getRenderingContext().popCullFace();
	context.getRenderingContext().popColorBuffer();

	context.popCamera();

	texMatrix.setIdentity();
	texMatrix.translate(0.5f, 0.5f, 0.5f);
	texMatrix.scale(0.5f);
	texMatrix *= lightProjectionMatrix;
	texMatrix *= lightModelViewMatrix;

	if (rp.getFlag(SHOW_META_OBJECTS)) {
		context.displayNode(camera.get(), rp);
	}

	Rendering::Shader * shader = context.getRenderingContext().getActiveShader();
	if(shader != nullptr) {
		shader->setUniform(context.getRenderingContext(), Rendering::Uniform("sg_shadowEnabled", true));
		shader->setUniform(context.getRenderingContext(), Rendering::Uniform("sg_shadowMatrix", texMatrix));
		shader->setUniform(context.getRenderingContext(), Rendering::Uniform("sg_shadowTexture", shadowTexture->getTextureUnit()));
		shader->setUniform(context.getRenderingContext(), Rendering::Uniform("sg_shadowTextureSize", texSize));
	}

	// ##### Activate shadow texture #####
	return shadowTexture->enableState(context, node, rp);
}

void ShadowState::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	Rendering::Shader * shader = context.getRenderingContext().getActiveShader();
	if(shader != nullptr) {
		shader->setUniform(context.getRenderingContext(), Rendering::Uniform("sg_shadowEnabled", false));
	}
	shadowTexture->disableState(context, node, rp);
}

ShadowState * ShadowState::clone() const {
	// Implementation cannot be prevented.
	FAIL();
	return nullptr;;
}

}
