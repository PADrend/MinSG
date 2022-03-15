/*
	This file is part of the MinSG library extension TwinPartitions.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_TWIN_PARTITIONS

#include "TwinPartitionsRenderer.h"
#include "PartitionsData.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/FrustumTest.h"

// #ifdef MINSG_EXT_OUTOFCORE
// #include "../OutOfCore/CacheManager.h"
// #include "../OutOfCore/OutOfCore.h"
// #endif /* MINSG_EXT_OUTOFCORE */

#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Draw.h>

#include <Util/Graphics/ColorLibrary.h>
#include <Util/Macros.h>

#include <fstream>

namespace MinSG {
namespace TwinPartitions {

static Rendering::Shader * getTDMShader() {
	static Rendering::Shader * shader = nullptr;
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
	if (shader == nullptr) {
		shader = Rendering::Shader::createShader(vertexProgram, fragmentProgram);
	}
	return shader;
}

static const uint32_t INVALID_CELL = std::numeric_limits<uint32_t>::max();

TwinPartitionsRenderer::TwinPartitionsRenderer(PartitionsData * partitions) :
	State(), data(partitions), textures(), currentCell(INVALID_CELL), maxRuntime(100000), polygonOffsetFactor(1.5f), polygonOffsetUnits(4.0f), drawTexturedDepthMeshes(true) {
}

TwinPartitionsRenderer::~TwinPartitionsRenderer() = default;

State::stateResult_t TwinPartitionsRenderer::doEnableState(FrameContext & context, Node *, const RenderParam & rp) {
	if (rp.getFlag(SKIP_RENDERER)) {
		return State::STATE_SKIPPED;
	}

	if (data == nullptr) {
		// Invalid information. => Fall back to standard rendering.
		return State::STATE_SKIPPED;
	}

	const Geometry::Vec3f pos = context.getCamera()->getWorldOrigin();
	// Check if cached cell can be used.
	if (currentCell == INVALID_CELL || !data->cells[currentCell].bounds.contains(pos)) {
		// Search the cell that contains the camera position.
		bool cellFound = false;
		const uint32_t cellCount = static_cast<uint32_t>(data->cells.size());
		for (uint_fast32_t cell = 0; cell < cellCount; ++cell) {
			if (data->cells[cell].bounds.contains(pos)) {
// #ifdef MINSG_EXT_OUTOFCORE
// 				// Set new priorities if the current cell changes.
// 				if (currentCell != cell) {
// 					// Low priority for old depth mesh.
// 					if (currentCell != INVALID_CELL) {
// 						for(std::vector<uint32_t>::const_iterator it = data->cells[currentCell].surroundingIds.begin(); it != data->cells[currentCell].surroundingIds.end(); ++it) {
// 							Rendering::Mesh * depthMesh = data->depthMeshes[*it].get();
// 							OutOfCore::getCacheManager().setUserPriority(depthMesh, 0);
// 						}
// 					}
// 					// High priority for new depth mesh.
// 					{
// 						for(std::vector<uint32_t>::const_iterator it = data->cells[cell].surroundingIds.begin(); it != data->cells[cell].surroundingIds.end(); ++it) {
// 							Rendering::Mesh * depthMesh = data->depthMeshes[*it].get();
// 							OutOfCore::getCacheManager().setUserPriority(depthMesh, 20);
// 						}
// 					}
// 					// Low priority for meshes visible from the old cell.
// 					if (currentCell != INVALID_CELL) {
// 						const uint32_t oldVisibleSetIndex = data->cells[currentCell].visibleSetId;
// 						const PartitionsData::visible_set_t & oldVisibleSet = data->visibleSets[oldVisibleSetIndex];
// 						for (PartitionsData::visible_set_t::const_iterator it = oldVisibleSet.begin(); it != oldVisibleSet.end(); ++it) {
// 							Rendering::Mesh * mesh = data->objects[it->second].get();
// 							OutOfCore::getCacheManager().setUserPriority(mesh, 0);
// 						}
// 					}
// 					// High priority for meshes visible from the new cell.
// 					{
// 						const uint32_t visibleSetIndex = data->cells[cell].visibleSetId;
// 						const PartitionsData::visible_set_t & visibleSet = data->visibleSets[visibleSetIndex];
// 						for (PartitionsData::visible_set_t::const_iterator it = visibleSet.begin(); it != visibleSet.end(); ++it) {
// 							Rendering::Mesh * mesh = data->objects[it->second].get();
// 							OutOfCore::getCacheManager().setUserPriority(mesh, 10);
// 						}
// 					}
// 				}
// #endif /* MINSG_EXT_OUTOFCORE */
				// Load new textures.
				textures.clear();
				if(drawTexturedDepthMeshes) {
					textures.reserve(6);
					for(auto & elem : data->cells[cell].surroundingIds) {
						const std::string & textureFile = data->textureFiles[elem];
						Util::Reference<Rendering::Texture> texture = Rendering::Serialization::loadTexture(Util::FileName(textureFile));
						if(texture == nullptr) {
							WARN("Failed to load texture for depth mesh.");
						} else {
							textures.push_back(texture);
						}
					}
				}

				currentCell = cell;
				cellFound = true;
				break;
			}
		}
		if (!cellFound) {
			// Camera outside view space.
			currentCell = INVALID_CELL;
			return State::STATE_SKIPPED;
		}
	}

	uint32_t renderedTriangles = 0;

	const uint32_t visibleSetIndex = data->cells[currentCell].visibleSetId;
	const PartitionsData::visible_set_t & visibleSet = data->visibleSets[visibleSetIndex];

	if(drawTexturedDepthMeshes) {
		for(auto & elem : data->cells[currentCell].surroundingIds) {
			Rendering::Mesh * depthMesh = data->depthMeshes[elem].get();
			// Count the depth meshes here already.
			renderedTriangles += depthMesh->getPrimitiveCount();
		}
	}
	const Geometry::Frustum & frustum = context.getCamera()->getFrustum();

	/*float minDist = 9999999.0f;
	float maxDist = 0.0f;
	float prioSum = 0.0f;
	uint_fast32_t objectCounter = 0;
	for (PartitionsData::visible_set_t::const_iterator it = visibleSet.begin(); it != visibleSet.end(); ++it) {
		Rendering::Mesh * mesh = data->objects[it->second].get();
		float dist = mesh->getBoundingBox().getDistance(pos);
		minDist = std::min(dist, minDist);
		maxDist = std::max(dist, maxDist);
		if(objectCounter < 10) {
			prioSum += it->first;
			++objectCounter;
		}
	}

	std::ofstream output("twinOutput.tsv", ios_base::out | ios_base::app);
	output << currentCell << '\t' << data->cells[currentCell].bounds.getDiameter() << '\t' << (data->cells[currentCell].bounds.getCenter() - pos).length() << '\t'
	 << visibleSet.begin()->first << '\t'
	 << visibleSet.rbegin()->first << '\t'
	 << minDist << '\t' << maxDist << '\t' << prioSum << '\t';
	for(std::vector<uint32_t>::const_iterator it = data->cells[currentCell].surroundingIds.begin(); it != data->cells[currentCell].surroundingIds.end(); ++it) {
		Rendering::Mesh * depthMesh = data->depthMeshes[*it].get();
		const Geometry::Box & depthMeshBB = depthMesh->getBoundingBox();

		const Geometry::Vec3f originalDirection = (depthMeshBB.getCenter() - data->cells[currentCell].bounds.getCenter()).normalize();
		const Geometry::Vec3f currentDirection = (depthMeshBB.getCenter() - pos).normalize();
		const float angle = currentDirection.dot(originalDirection);
		const bool visible = (frustum.isBoxInFrustum(depthMeshBB) != Frustum::OUTSIDE);
		output << '\t';
		if(visible) {

			output << angle ;
		} else {
				output << "NA";
		}
	}
	for(uint_fast32_t i = data->cells[currentCell].surroundingIds.size(); i < 6; ++i) {
		output << "\tNA";
	}
	output << std::endl;
	output.close();*/


	for (auto & elem : visibleSet) {
		Rendering::Mesh * mesh = data->objects[elem.second].get();
		if (conditionalFrustumTest(frustum, mesh->getBoundingBox(), rp)) {
			context.displayMesh(mesh);
			renderedTriangles += mesh->getPrimitiveCount();
		}
		if(rp.getFlag(BOUNDING_BOXES)) {
			Rendering::drawWireframeBox(context.getRenderingContext(), mesh->getBoundingBox(), Util::ColorLibrary::RED);
		}
		if (maxRuntime != 0 && renderedTriangles >= maxRuntime) {
			break;
		}
	}
	// Draw the textured depth meshes at the end.
	if(drawTexturedDepthMeshes) {
		context.getRenderingContext().pushAndSetPolygonOffset(Rendering::PolygonOffsetParameters(polygonOffsetFactor, polygonOffsetUnits));
		context.getRenderingContext().pushAndSetShader(getTDMShader());
		context.getRenderingContext().pushTexture(0);
		auto texIt = textures.begin();
		for(auto & elem : data->cells[currentCell].surroundingIds) {
			Rendering::Mesh * depthMesh = data->depthMeshes[elem].get();
			Rendering::Texture * texture = texIt->get();

			context.getRenderingContext().setTexture(0, texture);

			if (conditionalFrustumTest(frustum, depthMesh->getBoundingBox(), rp)) {
				context.displayMesh(depthMesh);
			}

			++texIt;
		}
		context.getRenderingContext().popTexture(0);
		context.getRenderingContext().popShader();
		context.getRenderingContext().popPolygonOffset();
	}

	return State::STATE_SKIP_RENDERING;
}

State * TwinPartitionsRenderer::clone() const {
	return nullptr;
}

}
}

#endif /* MINSG_EXT_TWIN_PARTITIONS */
