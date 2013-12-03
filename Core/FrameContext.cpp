/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "FrameContext.h"

#include "Nodes/Node.h"
#include "Nodes/AbstractCameraNode.h"
#include "Statistics.h"
#include "../Helper/TextAnnotation.h"

#include <Geometry/Tools.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Rendering/TextRenderer.h>

#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/EmbeddedFont.h>
#include <Util/Graphics/FontRenderer.h>
#include <Util/Macros.h>

#include <stack>

using Geometry::Vec3;

namespace MinSG {

// -----------------------------------
// --- Main

FrameContext::FrameContext() : Util::ReferenceCounter<FrameContext>(),
		worldUpVector(0,1,0), worldFrontVector(0,0,1), worldRightVector(1,0,0),
		frameNumber(0),
		renderingContext(new Rendering::RenderingContext),
		statistics(new Statistics) {

	registerNodeRenderer(	DEFAULT_CHANNEL,
							[](FrameContext & context, Node * node, const RenderParam & rp) {
								node->display(context, rp);
								return NodeRendererResult::NODE_HANDLED;
							});
	registerNodeRenderer(	APPROXIMATION_CHANNEL, 
							[](FrameContext & context, Node * node, const RenderParam & /*rp*/) {
								Rendering::drawAbsBox(context.getRenderingContext(), node->getWorldBB(), Util::ColorLibrary::LIGHT_GREY);
								return NodeRendererResult::NODE_HANDLED;
							});

	const auto result = Util::EmbeddedFont::getFont();
	Util::Reference<Util::Bitmap> fontBitmap(result.first);
	textRenderer.reset(new Rendering::TextRenderer(*result.first.get(), result.second));
}

FrameContext::~FrameContext() {
}

// -----------------------------------
// --- Camera
void FrameContext::setCamera(AbstractCameraNode * newCamera) {
	if(newCamera == nullptr) {
		WARN("setCamera: Invalid camera");
		return;
	}

	camera = newCamera;
	
	camera->updateFrustum();

	const Geometry::Matrix4x4 & projectionMatrix = camera->getFrustum().getProjectionMatrix();
	// OVERSIZE_FRUSTUM: If you want to debug frustum culling, uncomment the next line.
	// projectionMatrix.scale(1.0f, 1.0f, 1.5f);

	renderingContext->setProjectionMatrix(projectionMatrix);
	renderingContext->setInverseCameraMatrix(camera->getWorldMatrix());
	renderingContext->resetMatrix();

	renderingContext->setViewport(camera->getViewport());

	if(camera->isScissorEnabled()) {
		renderingContext->setScissor(Rendering::ScissorParameters(camera->getScissor()));
	} else {
		renderingContext->setScissor(Rendering::ScissorParameters());
	}
}

void FrameContext::pushCamera() {
	cameraStack.push(camera);
}

void FrameContext::popCamera() {
	if (cameraStack.empty()) {
		WARN("popCamera: Empty camera stack");
		return;
	}
	setCamera(cameraStack.top().get());
	cameraStack.pop();
}

Vec3 FrameContext::convertWorldPosToScreenPos(const Vec3 & objPos) const {
	if (!hasCamera()) {
		WARN("No associated camera.");
		return Vec3();
	}
	const Geometry::Rect cameraViewport(getCamera()->getViewport());
	const auto transformation = renderingContext->getProjectionMatrix() * renderingContext->getCameraMatrix();
	Vec3 result = Geometry::project(objPos, transformation, cameraViewport);
	result.setY(cameraViewport.getHeight() - result.getY());
	return result;
}

//! // http://nehe.gamedev.net/data/articles/article.asp?article=13
Vec3 FrameContext::convertScreenPosToWorldPos(const Vec3 & screenPos) const {
	if (!hasCamera()) {
		WARN("No associated camera.");
		return Vec3();
	}
	const Geometry::Rect cameraViewport(getCamera()->getViewport());
	const auto transformation = renderingContext->getProjectionMatrix() * renderingContext->getCameraMatrix();
	return Geometry::unProject(Geometry::Vec3(screenPos.getX(), cameraViewport.getHeight() - screenPos.getY(), screenPos.getZ()),
								transformation, cameraViewport);
}

Geometry::Rect FrameContext::getProjectedRect(Node * node) const {
	return getProjectedRect(node, Geometry::Rect(renderingContext->getViewport()));
}

Geometry::Rect FrameContext::getProjectedRect(Node * node, const Geometry::Rect & screenRect) const {
	return Geometry::projectBox(node->getBB(), renderingContext->getCameraMatrix() * node->getWorldMatrix(), renderingContext->getProjectionMatrix(), screenRect);
}

// -----------------------------------
// --- Frame handling

void FrameContext::beginFrame(int _frameNumber/*=-1*/){
	
	if(_frameNumber<0){
		_frameNumber=this->frameNumber++;
	}
	getStatistics().beginFrame(_frameNumber);

	// Inform frame listeners.
	std::vector<FrameListenerFunction> tmpList;
	using std::swap;
	swap(tmpList,beginFrameListenerCallbacks);
	for(auto & callback : tmpList) {
		if(!callback(*this)) {
			beginFrameListenerCallbacks.push_back(callback);
		}
	}

}

void FrameContext::endFrame(bool waitForGlFinish/*=false*/){
	
	// Inform frame listeners.
	std::vector<FrameListenerFunction> tmpList;
	using std::swap;
	swap(tmpList,endFrameListenerCallbacks);
	for(auto & callback : tmpList) {
		if(!callback(*this)) {
			endFrameListenerCallbacks.push_back(callback);
		}
	}
	
	if(waitForGlFinish){
		getStatistics().pushEvent(Statistics::EVENT_TYPE_FRAME_END,0.5); // here means: last command
		renderingContext->finish();
	}
	getStatistics().endFrame();
}


void FrameContext::addBeginFrameListener(const FrameContext::FrameListenerFunction & listener) {
	beginFrameListenerCallbacks.push_back(listener);
}
void FrameContext::addEndFrameListener(const FrameContext::FrameListenerFunction & listener) {
	endFrameListenerCallbacks.push_back(listener);
}
// -----------------------------------
// Rendering

const Util::StringIdentifier FrameContext::DEFAULT_CHANNEL("DEFAULT_CHANNEL");
const Util::StringIdentifier FrameContext::TRANSPARENCY_CHANNEL("TRANSPARENCY_CHANNEL");
const Util::StringIdentifier FrameContext::APPROXIMATION_CHANNEL("APPROXIMATION_CHANNEL");

bool FrameContext::displayNode(Node * node, const RenderParam & rp) {
	renderingChannel_t & renderingChannel = renderingChannels[rp.getChannel()];

	for(const auto & renderer : renderingChannel.getElements()) {
		if(renderer(*this, node, rp) == NodeRendererResult::NODE_HANDLED) {
			return true;
		}
	}
	return false;
}

FrameContext::node_renderer_registration_t FrameContext::registerNodeRenderer(const Util::StringIdentifier & channelName, NodeRenderer renderer) {
	return renderingChannels[channelName].registerElement(std::move(renderer));
}

void FrameContext::unregisterNodeRenderer(const Util::StringIdentifier & channelName, node_renderer_registration_t handle) {
	renderingChannels[channelName].unregisterElement(std::move(handle));
}

void FrameContext::displayMesh(Rendering::Mesh * mesh) {
	renderingContext->displayMesh(mesh);

	const uint32_t primitiveCount = mesh->getPrimitiveCount();
	getStatistics().pushEvent(Statistics::EVENT_TYPE_GEOMETRY, primitiveCount);
	getStatistics().countMesh(*mesh, primitiveCount);
}

void FrameContext::displayMesh(Rendering::Mesh * mesh, uint32_t firstElement, uint32_t elementCount) {
	renderingContext->displayMesh(mesh, firstElement, elementCount);

	const uint32_t primitiveCount = mesh->getPrimitiveCount(elementCount);
	getStatistics().pushEvent(Statistics::EVENT_TYPE_GEOMETRY, primitiveCount);
	getStatistics().countMesh(*mesh, primitiveCount);
}

void FrameContext::showAnnotation(Node * node, const std::string & text, const int yPosOffset /* = 0 */,const bool showRectangle ) {
	static const Util::Color4f textColor(1, 1, 1, 1);
	static const Util::Color4f bgColor(0, 0, 0, 1);
	showAnnotation(node, text, yPosOffset, showRectangle, textColor, bgColor);
}
void FrameContext::showAnnotation(Node * node, const std::string & text, const int yPosOffset,const bool showRectangle, const Util::Color4f & textColor,const Util::Color4f & bgColor) {
	if(node == nullptr) {
		return;
	}

	const auto worldPos = node->getWorldBB().getCenter();
	const auto screenPos = convertWorldPosToScreenPos(worldPos);
	if(screenPos.z() > 1.0) {
		return;
	}

	if (!hasCamera()) {
		WARN("No associated camera.");
		return;
	}

	const auto & cameraViewport = getCamera()->getViewport();
	const auto projectedRect = getProjectedRect(node);

	// The rect has to be flipped vertically here, because the y coordinates are flipped in text draw mode.
	const Geometry::Rect_f flippedRect(projectedRect.getX(), 
									   cameraViewport.getHeight() - projectedRect.getMaxY(),
									   projectedRect.getWidth(),
									   projectedRect.getHeight());

	
	if(showRectangle){
		Rendering::enable2DMode(*renderingContext);
		renderingContext->pushAndSetBlending(Rendering::BlendingParameters());
		renderingContext->pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::LESS));
		renderingContext->pushAndSetLighting(Rendering::LightingParameters());
		renderingContext->pushAndSetLine(Rendering::LineParameters(1));
		renderingContext->applyChanges(true);

		Rendering::drawWireframeRect(*renderingContext, flippedRect, Util::ColorLibrary::LIGHT_GREY);
	
		renderingContext->popLine();
		renderingContext->popLighting();
		renderingContext->popDepthBuffer();
		renderingContext->popBlending();
		Rendering::disable2DMode(*renderingContext);
	}
	
	const int pinLength = flippedRect.getMinY() - screenPos.getY() - 10 - yPosOffset;

	TextAnnotation::displayText(*this,
								worldPos,
								Geometry::Vec2i(0, pinLength),
								2,
								bgColor,
								getTextRenderer(),
								text,
								textColor);
}

void FrameContext::setTextRenderer(const Rendering::TextRenderer & newTextRenderer) {
	textRenderer.reset(new Rendering::TextRenderer(newTextRenderer));
}

const Rendering::TextRenderer & FrameContext::getTextRenderer() const {
	return *(textRenderer.get());
}

}
