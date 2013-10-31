/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "NodeImporterUtils.h"

#include "../../../SceneManagement/SceneDescription.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>

#include <Rendering/MeshUtils/MeshUtils.h>

#include <Util/Macros.h>
#include <Util/StringIdentifier.h>

#include <vector>
#include <deque>

namespace MinSG {
namespace LoaderCOLLADA {

Rendering::Mesh * createSingleMesh(const std::deque<VertexPart> & vertexParts, const std::vector<uint32_t> & indices, const uint32_t triangleCount,
								   const uint16_t maxIndexOffset, std::vector<uint32_t> & vertexIds) {
	// Create mesh description and data.
	Rendering::VertexDescription vd;
	// Order for our vertex data in Mesh
	std::deque<VertexPart> orderedParts;

// 	uint32_t vertexSize = 0;

	// fill orderedParts and create corresponding vertexDescription
	for(auto & vertexPart : vertexParts) {
		if(vertexPart.type == VertexPart::POSITION) {
			const Rendering::VertexAttribute & attr = vd.getAttribute(Rendering::VertexAttributeIds::POSITION);
			if(!attr.empty()) {
				WARN("POSITION used multiple times.");
				return nullptr;
			}
			vd.appendPosition3D();
			// make sure positions stands at the beginning
			orderedParts.push_front(vertexPart);
// 			vertexSize = vertexPart.data.size();
		} else if(vertexPart.type == VertexPart::NORMAL) {
			const Rendering::VertexAttribute & attr = vd.getAttribute(Rendering::VertexAttributeIds::NORMAL);
			if(!attr.empty()) {
				WARN("NORMAL used multiple times.");
				return nullptr;
			}
			vd.appendNormalFloat();
			orderedParts.push_back(vertexPart);
		} else if(vertexPart.type == VertexPart::COLOR) {
			const Rendering::VertexAttribute & attr = vd.getAttribute(Rendering::VertexAttributeIds::COLOR);
			if(!attr.empty()) {
				WARN("COLOR used multiple times.");
				return nullptr;
			}
			vd.appendFloatAttribute(Rendering::VertexAttributeIds::COLOR, vertexPart.stride);
			orderedParts.push_back(vertexPart);

		} else if(vertexPart.type == VertexPart::TEXCOORD) {
			bool inserted = false;
			for(uint_fast8_t i = 0; i < 8; ++i) {
				const Util::StringIdentifier texCoordId = Rendering::VertexAttributeIds::getTextureCoordinateIdentifier(i);
				const Rendering::VertexAttribute & attr = vd.getAttribute(texCoordId);
				if(attr.empty()) {
					vd.appendFloatAttribute(texCoordId, 2);
					orderedParts.push_back(vertexPart);
					inserted = true;
					break;
				}
			}
			if(!inserted) {
				WARN("Limit of TEXCOORD definitions exceeded (skipped).");
			}
		} else {
			WARN("Unknown type (skipped).");
		}
	}

	// create mesh
	auto mesh = new Rendering::Mesh();
	Rendering::MeshIndexData & iData = mesh->openIndexData();
	iData.allocate(3 * triangleCount);

	Rendering::MeshVertexData & vData = mesh->openVertexData();
	vData.allocate(3 * triangleCount, vd); // to simplify the creation process, each index points first poins to its own vertex.
	uint32_t * indexData = iData.data();
	for(uint32_t i = 0; i < 3 * triangleCount; ++i) {
		float * vertexData = reinterpret_cast<float *>(vData[i]);
		for(auto & part : orderedParts) {

			uint32_t pos = indices[i * maxIndexOffset + part.indexOffset] * part.stride;
			if(part.type == VertexPart::POSITION) {
				vertexIds.emplace_back(pos / 3);
			}
			for(uint_fast8_t v = 0; v < part.stride; ++v) {
				*vertexData = part.data[pos];
				++vertexData;
				++pos;
			}
		}

		*indexData = i;
		++indexData;
	}

	iData.updateIndexRange();
	vData.updateBoundingBox();

	return mesh;
}

}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
