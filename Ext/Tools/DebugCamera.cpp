/*
	This file is part of the MinSG library.
	Copyright (C) 2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "DebugCamera.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/FBO.h>

#include <functional>

using namespace std::placeholders;
using namespace Rendering;

namespace MinSG {

DebugCamera::DebugCamera() = default;
DebugCamera::~DebugCamera() = default;

void DebugCamera::displayMesh(RenderingContext & rc, Mesh * mesh) {

	// Display for the "normal" camera.
	mesh->_display(rc, 0, mesh->isUsingIndexData() ? mesh->getIndexCount() : mesh->getVertexCount());

	if(rc.getColorBufferParameters().isAnyWritingEnabled() || rc.getDepthBufferParameters().isWritingEnabled()) {
		
		rc.pushFBO();
		rc.pushProjectionMatrix();
		rc.pushMatrix();
		rc.pushViewport();
		rc.pushScissor();
		
		Geometry::Matrix4x4 mod = rc.getMatrix();
		
		rc.setFBO(fbo.get());
		rc.setProjectionMatrix(debug->getFrustum().getProjectionMatrix());
		rc.setMatrix_cameraToWorld(debug->getWorldMatrix());
		rc.setMatrix(conversionMatrix * mod);
		rc.setViewport(debug->getViewport());
		if(debug->isScissorEnabled()) {
			rc.setScissor(Rendering::ScissorParameters(debug->getScissor()));
		} else {
			rc.setScissor(Rendering::ScissorParameters());
		}

		// Display for the "debug" camera.
		mesh->_display(rc, 0, mesh->isUsingIndexData() ? mesh->getIndexCount() : mesh->getVertexCount());

		rc.setMatrix_cameraToWorld(original->getWorldMatrix());
		
		rc.popFBO();
		rc.popProjectionMatrix();
		rc.popMatrix();
		rc.popViewport();
		rc.popScissor();

	}
}

void DebugCamera::enable(RenderingContext & rc, AbstractCameraNode * _debug, AbstractCameraNode * _original, FBO * _fbo) {
	debug = _debug;
	original = _original;
	fbo = _fbo;

	debug->updateFrustum();
	original->updateFrustum();

	conversionMatrix = debug->getWorldMatrix().inverse() * original->getWorldMatrix();

	rc.setDisplayMeshFn(std::bind(std::mem_fn(&DebugCamera::displayMesh), this, _1, _2));
}

void DebugCamera::disable(RenderingContext & rc) {
	rc.resetDisplayMeshFn();
	debug = nullptr;
	original = nullptr;
	fbo = nullptr;
	conversionMatrix.setIdentity();
}

}
