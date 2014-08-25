/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION

#include "VisibilitySubdivisionRenderer.h"
#include "VisibilityVector.h"
#include "../ValuatedRegion/ValuatedRegionNode.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/FrustumTest.h"

#ifdef MINSG_EXT_OUTOFCORE
#include "../OutOfCore/CacheManager.h"
#include "../OutOfCore/DataStrategy.h"
#include "../OutOfCore/OutOfCore.h"
#endif /* MINSG_EXT_OUTOFCORE */

#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Macros.h>

#include <algorithm>
#include <stdexcept>

namespace MinSG {
namespace VisibilitySubdivision {

static Rendering::Shader * getTDMShader() {
	static Util::Reference<Rendering::Shader> shader;
	const std::string vertexProgram("uniform mat4 sg_matrix_modelToClipping;\
										\
										attribute vec3 sg_Position;\
										attribute vec2 sg_TexCoord0;\
										\
										varying vec2 texCoord0;\
										\
										void main() {\
											texCoord0 = sg_TexCoord0;\
											gl_Position = sg_matrix_modelToClipping * vec4(sg_Position, 1.0);\
										}");
	const std::string fragmentProgram("varying vec2 texCoord0;\
										uniform sampler2D sg_texture0;\
										\
										void main() {\
											gl_FragColor = texture2D(sg_texture0, texCoord0);\
										}");
	if (shader.isNull()) {
		shader = Rendering::Shader::createShader(vertexProgram, fragmentProgram);
	}
	return shader.get();
}

static const VisibilityVector & getVV(const VisibilitySubdivisionRenderer::cell_ptr node) {
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

VisibilitySubdivisionRenderer::VisibilitySubdivisionRenderer() :
	State(), hold(false), debugOutput(false), maxRuntime(500000), viSu(nullptr), currentCell(nullptr), objects(), holdObjects(), 
			startRuntime(0), lastCamMatrix(), accumRenderingEnabled(false),
			displayTexturedDepthMeshes(false), depthMeshes(), textures(), polygonOffsetFactor(1.5f), polygonOffsetUnits(4.0f) {
}

VisibilitySubdivisionRenderer::VisibilitySubdivisionRenderer(const VisibilitySubdivisionRenderer & source) :
	State(source), hold(source.hold), debugOutput(source.debugOutput), maxRuntime(source.maxRuntime),
			viSu(source.viSu), currentCell(source.currentCell), objects(source.objects), holdObjects(source.holdObjects),
			startRuntime(source.startRuntime), lastCamMatrix(source.lastCamMatrix), accumRenderingEnabled(source.accumRenderingEnabled),
			displayTexturedDepthMeshes(source.displayTexturedDepthMeshes), depthMeshes(), textures(),
			polygonOffsetFactor(source.polygonOffsetFactor), polygonOffsetUnits(source.polygonOffsetUnits) {
}

VisibilitySubdivisionRenderer::~VisibilitySubdivisionRenderer() = default;

void VisibilitySubdivisionRenderer::setViSu(cell_ptr vrn) {
	viSu = vrn;
}

VisibilitySubdivisionRenderer * VisibilitySubdivisionRenderer::clone() const {
	return new VisibilitySubdivisionRenderer(*this);
}

State::stateResult_t VisibilitySubdivisionRenderer::doEnableState(
																	FrameContext & context,
																	Node *,
																	const RenderParam & rp) {
	if (rp.getFlag(SKIP_RENDERER)) {
		return State::STATE_SKIPPED;
	}

	if (viSu == nullptr) {
		// Invalid information. => Fall back to standard rendering.
		return State::STATE_SKIPPED;
	}

	// [AccumRendering]
	if(accumRenderingEnabled){
		// camera moved? -> start new frame
		const Geometry::Matrix4x4 & newCamMat=context.getCamera()->getWorldMatrix();
		if(newCamMat!=lastCamMatrix){
			lastCamMatrix = newCamMat;
			startRuntime=0;
		}
	}else{
		startRuntime=0;
	}
	// ----

	uint32_t renderedTriangles = 0;
	if (hold) {
		for (const auto & object : holdObjects) {
			if (debugOutput) {
				debugDisplay(renderedTriangles, object, context, rp);
			} else {
				context.displayNode(object, rp);
			}
		}
		return State::STATE_SKIP_RENDERING;
	}

	Geometry::Vec3 pos = context.getCamera()->getWorldPosition();
	bool refreshCache = false;
	// Check if cached cell can be used.
	if (currentCell == nullptr || !currentCell->getBB().contains(pos)) {
		currentCell = viSu->getNodeAtPosition(pos);
		refreshCache = true;
	}
	if (currentCell == nullptr || !currentCell->isLeaf()) {
		// Invalid information. => Fall back to standard rendering.
		return State::STATE_SKIPPED;
	}

	if (refreshCache) {
		try {
			const auto & vv = getVV(currentCell);

			const uint32_t maxIndex = vv.getIndexCount();
			objects.clear();
			objects.reserve(maxIndex);
			for(uint_fast32_t index = 0; index < maxIndex; ++index) {
				if(vv.getBenefits(index) == 0) {
					continue;
				}
				const VisibilityVector::costs_t costs = vv.getCosts(index);
				const VisibilityVector::benefits_t benefits = vv.getBenefits(index);
				const float score = static_cast<float>(costs) / static_cast<float>(benefits);
				objects.emplace_back(score, vv.getNode(index));
			}
		} catch(...) {
			// Invalid information. => Fall back to standard rendering.
			return State::STATE_SKIPPED;
		}

		if (displayTexturedDepthMeshes) {
#ifdef MINSG_EXT_OUTOFCORE
			for (const auto & depthMesh : depthMeshes) {
				OutOfCore::getCacheManager().setUserPriority(depthMesh.get(), 0);
			}
#endif /* MINSG_EXT_OUTOFCORE */
			depthMeshes.clear();
			depthMeshes.reserve(6);
			textures.clear();
			textures.reserve(6);

			const std::string dmDirectionStrings[6] = { "-dir_x1_y0_z0", "-dir_x-1_y0_z0",
														"-dir_x0_y1_z0", "-dir_x0_y-1_z0",
														"-dir_x0_y0_z1", "-dir_x0_y0_z-1" };
			for (auto & dmDirectionString : dmDirectionStrings) {
				Util::GenericAttribute * attrib = currentCell->getAttribute("DepthMesh" + dmDirectionString);
				if (attrib == nullptr) {
					continue;
				}
				Util::FileName dmMeshPath(attrib->toString());
				Util::Reference<Rendering::Mesh> dmMesh;
#ifdef MINSG_EXT_OUTOFCORE
				Util::GenericAttribute * bbAttrib = currentCell->getAttribute("DepthMesh" + dmDirectionString + "-bounds");
				if (bbAttrib == nullptr) {
					WARN("Found depth mesh with no bounding box.");
					continue;
				}

				std::vector<float> bbValues = Util::StringUtils::toFloats(bbAttrib->toString());
				FAIL_IF(bbValues.size() != 6);
				const Geometry::Box meshBB(Geometry::Vec3(bbValues[0], bbValues[1], bbValues[2]), bbValues[3], bbValues[4], bbValues[5]);

				dmMesh = OutOfCore::addMesh(dmMeshPath, meshBB);
#else /* MINSG_EXT_OUTOFCORE */
				dmMesh = Rendering::Serialization::loadMesh(dmMeshPath);
#endif /* MINSG_EXT_OUTOFCORE */
				depthMeshes.emplace_back(dmMesh);
				// Count the depth mesh here already.
				renderedTriangles += dmMesh->getPrimitiveCount();

				Util::GenericAttribute * texAttrib = currentCell->getAttribute("Texture" + dmDirectionString);
				if (texAttrib == nullptr) {
					continue;
				}
				Util::FileName texturePath(texAttrib->toString());
				Util::Reference<Rendering::Texture> texture = Rendering::Serialization::loadTexture(texturePath);
				if (texture.isNull()) {
					WARN("Loading texture for depth mesh failed.");
					continue;
				}
				textures.emplace_back(texture);
			}
		}
	}

	const Geometry::Frustum & frustum = context.getCamera()->getFrustum();
	holdObjects.clear();
	holdObjects.reserve(objects.size());

	std::sort(objects.begin(), objects.end());
	for(const auto & ratioObjectPair : objects) {
		object_ptr o = ratioObjectPair.second;
#ifdef MINSG_EXT_OUTOFCORE
		OutOfCore::getCacheManager().setUserPriority(o->getMesh(), 5);
#endif /* MINSG_EXT_OUTOFCORE */
		
		if (conditionalFrustumTest(frustum, o->getWorldBB(), rp)) {
			// [AccumRendering]
			// skip geometry rendered in the last frame
			if(renderedTriangles<startRuntime){
				renderedTriangles += o->getTriangleCount();
				continue;
			}
			// ----

			if (debugOutput) {
				debugDisplay(renderedTriangles, o, context, rp);
			} else {
				context.displayNode(o, rp);
				renderedTriangles += o->getTriangleCount();
			}
			holdObjects.push_back(o);
		}
		if (maxRuntime != 0 && renderedTriangles >= startRuntime+maxRuntime) {
			break;
		}
	}

	// Draw the textured depth meshes at the end.
	if (displayTexturedDepthMeshes) {
		context.getRenderingContext().pushAndSetPolygonOffset(Rendering::PolygonOffsetParameters(polygonOffsetFactor, polygonOffsetUnits));
		auto texIt = textures.cbegin();
		for (const auto & depthMesh : depthMeshes) {
			context.getRenderingContext().pushAndSetShader(getTDMShader());
			context.getRenderingContext().pushAndSetTexture(0,texIt->get());
			
			if (conditionalFrustumTest(frustum, depthMesh->getBoundingBox(), rp)) {
				context.displayMesh(depthMesh.get());
			}
			context.getRenderingContext().popTexture(0);
			context.getRenderingContext().popShader();
#ifdef MINSG_EXT_OUTOFCORE
			OutOfCore::getCacheManager().setUserPriority(depthMesh.get(), 2);
#endif /* MINSG_EXT_OUTOFCORE */
			++texIt;
		}
		context.getRenderingContext().popPolygonOffset();
	}
	// [AccumRendering]
	startRuntime=renderedTriangles;
	// ----
	return State::STATE_SKIP_RENDERING;
}

bool VisibilitySubdivisionRenderer::renderCellSubset(FrameContext & context, cell_ptr cell, uint32_t budgetBegin, uint32_t budgetEnd) {
	try {
		const auto & vv = getVV(cell);

		std::vector<std::pair<float, object_ptr>> displayObjects;
		const uint32_t maxIndex = vv.getIndexCount();
		for(uint_fast32_t index = 0; index < maxIndex; ++index) {
			if(vv.getBenefits(index) == 0) {
				continue;
			}
			const VisibilityVector::costs_t costs = vv.getCosts(index);
			const VisibilityVector::benefits_t benefits = vv.getBenefits(index);
			const float score = static_cast<float>(costs) / static_cast<float>(benefits);
			displayObjects.emplace_back(score, vv.getNode(index));
		}

		const Geometry::Frustum & frustum = context.getCamera()->getFrustum();
		uint32_t renderedTriangles = 0;
		std::sort(displayObjects.begin(), displayObjects.end());
		for(const auto & objectRatioPair : displayObjects) {
			object_ptr o = objectRatioPair.second;
			Geometry::Frustum::intersection_t result = frustum.isBoxInFrustum(o->getWorldBB());
			if (result == Geometry::Frustum::INSIDE || result == Geometry::Frustum::INTERSECT) {
				if(renderedTriangles >= budgetBegin) {
					context.displayNode(o, 0);
				}
				renderedTriangles += o->getTriangleCount();
			}

			if (budgetEnd != 0 && renderedTriangles >= budgetEnd) {
				break;
			}
		}
	} catch(...) {
		// Invalid information. => Fall back to standard rendering.
		return false;
	}
	return true;
}

void VisibilitySubdivisionRenderer::debugDisplay(uint32_t & renderedTriangles, object_ptr object, FrameContext & context, const RenderParam & rp) {
	const Util::Color4f specular(0.1f, 0.1f, 0.1f, 1.0f);


	const float ratio = static_cast<float> (renderedTriangles) / static_cast<float> (maxRuntime);

	// Gradient from light blue to red.
	Util::Color4f color(ratio, 0.5f - 0.5f * ratio, 1.0f - ratio, 1.0f);

	Rendering::MaterialParameters material;
	material.setAmbient(color);
	material.setDiffuse(color);
	material.setSpecular(specular);
	material.setShininess(64.0f);

	context.getRenderingContext().pushAndSetMaterial(material);

	context.displayNode(object, rp);

	context.getRenderingContext().popMaterial();

	renderedTriangles += object->getTriangleCount();
}

}
}

#endif // MINSG_EXT_VISIBILITY_SUBDIVISION
