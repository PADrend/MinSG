/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ImpostorFactory.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Nodes/CameraNodeOrtho.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/TextureState.h"
#include "../../Core/Transformations.h"

#include <Geometry/Box.h>
#include <Geometry/Frustum.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Tools.h>
#include <Geometry/Vec3.h>

#include <Rendering/FBO.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/MeshUtils/QuadtreeMeshBuilder.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/ShaderUtils.h>
#include <Rendering/Shader/Shader.h>

#include <Util/Graphics/Color.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Numeric.h>
#include <Util/References.h>

#include <cmath>
#include <cstdint>

using namespace Geometry;
using namespace Rendering;

namespace MinSG {
namespace ImpostorFactory {

GeometryNode * createReliefBoardForNode(FrameContext & frameContext, Node * node) {
	const Geometry::Rect projectedRect = frameContext.getProjectedRect(node);
	const Geometry::Vec3f position = frameContext.getCamera()->getWorldOrigin();
	// Negative direction for camera.
	const Geometry::Vec3f direction = position - node->getWorldBB().getCenter();

	Util::Reference<CameraNodeOrtho> cameraOrtho = new CameraNodeOrtho;
	cameraOrtho->setWorldOrigin(position);
	Transformations::rotateToWorldDir(*cameraOrtho.get(),direction);

	const Geometry::Frustum frustum = Geometry::calcEnclosingOrthoFrustum(node->getBB(), cameraOrtho->getWorldToLocalMatrix() * node->getWorldTransformationMatrix());
	cameraOrtho->setNearFar(frustum.getNear(), frustum.getFar());
	cameraOrtho->setClippingPlanes(frustum.getLeft(), frustum.getRight(), frustum.getBottom(), frustum.getTop());

	const float frustumWidth = frustum.getRight() - frustum.getLeft();
	const float frustumHeight = frustum.getTop() - frustum.getBottom();
	float width;
	float height;
	if (projectedRect.getWidth() > projectedRect.getHeight()) {
		width = projectedRect.getWidth();
		height = width * frustumHeight / frustumWidth;
	} else {
		height = projectedRect.getHeight();
		width = height * frustumWidth / frustumHeight;
	}
	if (width < 1.0f || height < 1.0f) {
		return nullptr;
	}
	cameraOrtho->setViewport(Geometry::Rect_i(0, 0, width, height));

	Rendering::RenderingContext & renderingContext = frameContext.getRenderingContext();

	// Set up FBO, and textures
	Util::Reference<FBO> fbo = new FBO();
	renderingContext.pushAndSetFBO(fbo.get());

	frameContext.pushAndSetCamera(cameraOrtho.get());

	// First pass: Render to normal texture
	Util::Reference<Texture> normalTexture = TextureUtils::createStdTexture(width, height, true);
	Util::Reference<Texture> depthTexture = TextureUtils::createDepthTexture(width, height);
	fbo->attachColorTexture(renderingContext, normalTexture.get());
	fbo->attachDepthTexture(renderingContext, depthTexture.get());

	Util::Reference<Shader> normalToColorShader = Rendering::ShaderUtils::createNormalToColorShader();
	renderingContext.pushAndSetShader(normalToColorShader.get());

	renderingContext.clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
	frameContext.displayNode(node, 0);

	renderingContext.popShader();
	normalTexture->downloadGLTexture(renderingContext);

	// First pass: Render to color texture
	Util::Reference<Texture> colorTexture = TextureUtils::createStdTexture(width, height, true);
	fbo->attachColorTexture(renderingContext, colorTexture.get());

	// Render the node
	renderingContext.clearScreen(Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f));
	frameContext.displayNode(node, 0);

	// Deactivate the FBO, and download the textures
	colorTexture->downloadGLTexture(renderingContext);
	depthTexture->downloadGLTexture(renderingContext);
	renderingContext.popFBO();

	// create mesh
	Rendering::VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendColorRGBAByte();
	vertexDescription.appendNormalByte();
	Util::Reference<Mesh> mesh = MeshUtils::MeshBuilder::createMeshFromBitmaps(vertexDescription, depthTexture->getLocalBitmap(), colorTexture->getLocalBitmap(), normalTexture->getLocalBitmap());
	if (mesh.isNull()) {
		frameContext.popCamera();
		return nullptr;
	}
	const Matrix4x4 transMat = (renderingContext.getMatrix_cameraToClipping() * renderingContext.getMatrix_worldToCamera()).inverse();
	MeshVertexData & vd = mesh->openVertexData();
	Rendering::MeshUtils::transformCoordinates(vd, VertexAttributeIds::POSITION, transMat, 0, mesh->getVertexCount());
	vd.updateBoundingBox();
	{
		Util::Reference<Mesh> mesh2(MeshUtils::eliminateUnusedVertices(mesh.get()));
		mesh->_getIndexData().swap(mesh2->_getIndexData());
		mesh->_getVertexData().swap(mesh2->_getVertexData());
	}
	{
		Util::Reference<Mesh> mesh2(MeshUtils::eliminateLongTriangles(mesh.get(), 100.0f));
		mesh->_getIndexData().swap(mesh2->_getIndexData());
		mesh->_getVertexData().swap(mesh2->_getVertexData());
	}

	frameContext.popCamera();
	return new GeometryNode(mesh.get());
}

GeometryNode * createTexturedDepthMeshForNode(FrameContext & frameContext, Node * node) {
	const Geometry::Rect projectedRect = frameContext.getProjectedRect(node);
	const Geometry::Vec3f position = frameContext.getCamera()->getWorldOrigin();
	// Negative direction for camera.
	const Geometry::Vec3f direction = position - node->getWorldBB().getCenter();

	Util::Reference<CameraNodeOrtho> cameraOrtho = new CameraNodeOrtho;
	cameraOrtho->setWorldOrigin(position);
	Transformations::rotateToWorldDir(*cameraOrtho.get(),direction);

	const Geometry::Frustum frustum = Geometry::calcEnclosingOrthoFrustum(node->getBB(), cameraOrtho->getWorldToLocalMatrix() * node->getWorldTransformationMatrix());
	cameraOrtho->setNearFar(frustum.getNear(), frustum.getFar());
	cameraOrtho->setClippingPlanes(frustum.getLeft(), frustum.getRight(), frustum.getBottom(), frustum.getTop());

	const float frustumWidth = frustum.getRight() - frustum.getLeft();
	const float frustumHeight = frustum.getTop() - frustum.getBottom();
	float width;
	float height;
	if (projectedRect.getWidth() > projectedRect.getHeight()) {
		width = projectedRect.getWidth();
		height = width * frustumHeight / frustumWidth;
	} else {
		height = projectedRect.getHeight();
		width = height * frustumWidth / frustumHeight;
	}
	if (width < 1.0f || height < 1.0f) {
		return nullptr;
	}
	cameraOrtho->setViewport(Geometry::Rect_i(0, 0, width, height));

	Rendering::RenderingContext & renderingContext = frameContext.getRenderingContext();

	// Set up FBO, and textures
	Util::Reference<FBO> fbo = new FBO();
	renderingContext.pushAndSetFBO(fbo.get());

	Util::Reference<Texture> depthStencilTexture = TextureUtils::createDepthStencilTexture(width, height);

	frameContext.pushAndSetCamera(cameraOrtho.get());

	// First pass: Render to normal texture
	Util::Reference<Texture> normalTexture = TextureUtils::createStdTexture(width, height, true);
	fbo->attachColorTexture(renderingContext, normalTexture.get());
	fbo->attachDepthStencilTexture(renderingContext, depthStencilTexture.get());

	Util::Reference<Shader> normalToColorShader = Rendering::ShaderUtils::createNormalToColorShader();
	renderingContext.pushAndSetShader(normalToColorShader.get());

	renderingContext.clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
	frameContext.displayNode(node, 0);

	renderingContext.popShader();

	normalTexture->downloadGLTexture(renderingContext);

	// First pass: Render to color texture
	Util::Reference<Texture> colorTexture = TextureUtils::createStdTexture(width, height, true);
	fbo->attachColorTexture(renderingContext, colorTexture.get());
	fbo->attachDepthStencilTexture(renderingContext, depthStencilTexture.get());

	// Set stencil value to 255 for all rendered pixels
	Rendering::StencilParameters stencilParams;
	stencilParams.enable();
	stencilParams.setFunction(Rendering::Comparison::ALWAYS);
	stencilParams.setReferenceValue(255);
	stencilParams.setBitMask(255);
	stencilParams.setFailAction(Rendering::StencilParameters::REPLACE);
	stencilParams.setDepthTestFailAction(Rendering::StencilParameters::REPLACE);
	stencilParams.setDepthTestPassAction(Rendering::StencilParameters::REPLACE);
	renderingContext.pushAndSetStencil(stencilParams);

	// Render the node
	renderingContext.clearScreen(Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f));
	renderingContext.clearStencil(0);
	frameContext.displayNode(node, 0);

	// Deactivate the FBO, and download the textures
	colorTexture->downloadGLTexture(renderingContext);
	depthStencilTexture->downloadGLTexture(renderingContext);
	renderingContext.popStencil();
	renderingContext.popFBO();

	Util::Reference<Util::PixelAccessor> depthAccessor = TextureUtils::createDepthPixelAccessor(renderingContext, *depthStencilTexture.get());
	Util::Reference<Util::PixelAccessor> stencilAccessor = TextureUtils::createStencilPixelAccessor(renderingContext, *depthStencilTexture.get());
	Util::Reference<Util::PixelAccessor> normalAccessor = TextureUtils::createColorPixelAccessor(renderingContext, *normalTexture.get());

	Rendering::VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendTexCoord();
	vertexDescription.appendNormalByte();
	std::deque<Rendering::MeshUtils::QuadtreeMeshBuilder::split_function_t> splitFunctions;
	splitFunctions.push_back(Rendering::MeshUtils::QuadtreeMeshBuilder::DepthSplitFunction(depthAccessor, 0.2f));
	splitFunctions.push_back(Rendering::MeshUtils::QuadtreeMeshBuilder::StencilSplitFunction(stencilAccessor));
	Util::Reference<Rendering::Mesh> mesh = Rendering::MeshUtils::QuadtreeMeshBuilder::createMesh(vertexDescription,
											depthAccessor.get(),
											nullptr,
											normalAccessor.get(),
											stencilAccessor.get(),
											Rendering::MeshUtils::QuadtreeMeshBuilder::MultipleSplitFunction(splitFunctions));
	if (mesh == nullptr) {
		frameContext.popCamera();
		return nullptr;
	}

	// Determine the z range.
	const Geometry::Box & meshBB = mesh->getBoundingBox();
	if (Util::Numeric::equal(meshBB.getMinZ(), 1.0f) && Util::Numeric::equal(meshBB.getMaxZ(), 1.0f)) {
		// Depth texture consisted only of the far plane.
		frameContext.popCamera();
		return nullptr;
	}

	const uint32_t meshTriangles = mesh->getIndexCount() / 3;
	if (meshTriangles == 0) {
		frameContext.popCamera();
		return nullptr;
	}

	const Geometry::Matrix4x4f transMat = (renderingContext.getMatrix_cameraToClipping() * renderingContext.getMatrix_worldToCamera()).inverse();
	MeshVertexData & vertexData = mesh->openVertexData();
	Rendering::MeshUtils::transformCoordinates(vertexData, VertexAttributeIds::POSITION, transMat, 0, mesh->getVertexCount());
	vertexData.updateBoundingBox();

	frameContext.popCamera();
	GeometryNode * geoNode = new GeometryNode(mesh.get());
	geoNode->addState(new TextureState(colorTexture.get()));
	Rendering::MaterialParameters materialParams;
	materialParams.setAmbient(Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f));
	materialParams.setDiffuse(Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f));
	materialParams.setSpecular(Util::Color4f(0.0f, 0.0f, 0.0f, 1.0f));
	materialParams.setShininess(128.0f);
	geoNode->addState(new MaterialState(materialParams));
	return geoNode;
}

}
}
