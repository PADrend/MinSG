/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "NodeImporter.h"

#include "../Utils/NodeImporterUtils.h"
#include "../Utils/DescriptionUtils.h"

#include <COLLADAFWIWriter.h>

#include "../../../SceneManagement/SceneDescription.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Serialization/Serialization.h>

#include <Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-W#warnings)
COMPILER_WARN_OFF(-Wignored-qualifiers)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
COMPILER_WARN_OFF_GCC(-Wzero-as-null-pointer-constant)
COMPILER_WARN_OFF_GCC(-Wshadow)
COMPILER_WARN_OFF_GCC(-Wsign-promo)
#include <COLLADAFWMesh.h>
#include <COLLADAFWLight.h>
#include <COLLADAFWMeshPrimitive.h>
COMPILER_WARN_POP

#include <deque>
#include <vector>
#include <string>

namespace MinSG {
namespace LoaderCOLLADA {

bool lightCoreImporter(const COLLADAFW::Light * light, referenceRegistry_t & referenceRegistry) {
	auto lightDesc = new SceneManagement::DescriptionMap;

	lightDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
	switch(light->getLightType()) {
		case COLLADAFW::Light::DIRECTIONAL_LIGHT:
			lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_TYPE, SceneManagement::Consts::LIGHT_TYPE_DIRECTIONAL);
			break;
		case COLLADAFW::Light::SPOT_LIGHT:
			lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_TYPE, SceneManagement::Consts::LIGHT_TYPE_SPOT);
			break;

		case COLLADAFW::Light::POINT_LIGHT:
			lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_TYPE, SceneManagement::Consts::LIGHT_TYPE_POINT);
			break;

		case COLLADAFW::Light::AMBIENT_LIGHT:
			break;

		case COLLADAFW::Light::UNDEFINED:
		default:
			WARN("Undefined light type!");
			return false;
			break;
	}

	lightDesc->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_LIGHT);
	lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_CONSTANT_ATTENUATION, Util::StringUtils::toString(light->getConstantAttenuation()));
	lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_LINEAR_ATTENUATION, Util::StringUtils::toString(light->getLinearAttenuation()));
	lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_QUADRATIC_ATTENUATION, Util::StringUtils::toString(light->getQuadraticAttenuation()));
	lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_SPOT_CUTOFF, Util::StringUtils::toString(light->getFallOffAngle()));
	lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_SPOT_EXPONENT, Util::StringUtils::toString(light->getFallOffExponent()));

	std::ostringstream colorStream;
	colorStream << light->getColor().getRed() << " " << light->getColor().getGreen() << " " << light->getColor().getBlue() << " ";
	lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_AMBIENT, Util::StringUtils::toString(colorStream.str()));
	lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_DIFFUSE, Util::StringUtils::toString(colorStream.str()));
	lightDesc->setString(SceneManagement::Consts::ATTR_LIGHT_SPECULAR, Util::StringUtils::toString(colorStream.str()));

	referenceRegistry[light->getUniqueId()] = lightDesc;

	return true;
}

void triangulateQuadToTri(const COLLADAFW::MeshPrimitive & primitive, const uint32_t index, const bool hasNormals,
						  const bool hasColors, const bool hasUVCoords, std::vector<uint32_t> & indices) {

	indices.emplace_back(primitive.getPositionIndices()[index]);
	if(hasNormals) {
		indices.emplace_back(primitive.getNormalIndices()[index]);
	}
	if(hasColors) {
		indices.emplace_back(primitive.getColorIndices(0)->getIndices()[index]);
	}
	if(hasUVCoords) {
		indices.emplace_back(primitive.getUVCoordIndices(0)->getIndices()[index]);
	}

	indices.emplace_back(primitive.getPositionIndices()[index + 1]);
	if(hasNormals) {
		indices.emplace_back(primitive.getNormalIndices()[index + 1]);
	}
	if(hasColors) {
		indices.emplace_back(primitive.getColorIndices(0)->getIndices()[index + 1]);
	}
	if(hasUVCoords) {
		indices.emplace_back(primitive.getUVCoordIndices(0)->getIndices()[index + 1]);
	}

	indices.emplace_back(primitive.getPositionIndices()[index + 2]);
	if(hasNormals) {
		indices.emplace_back(primitive.getNormalIndices()[index + 2]);
	}
	if(hasColors) {
		indices.emplace_back(primitive.getColorIndices(0)->getIndices()[index + 2]);
	}
	if(hasUVCoords) {
		indices.emplace_back(primitive.getUVCoordIndices(0)->getIndices()[index + 2]);
	}

	indices.emplace_back(primitive.getPositionIndices()[index]);
	if(hasNormals) {
		indices.emplace_back(primitive.getNormalIndices()[index]);
	}
	if(hasColors) {
		indices.emplace_back(primitive.getColorIndices(0)->getIndices()[index]);
	}
	if(hasUVCoords) {
		indices.emplace_back(primitive.getUVCoordIndices(0)->getIndices()[index]);
	}

	indices.emplace_back(primitive.getPositionIndices()[index + 2]);
	if(hasNormals) {
		indices.emplace_back(primitive.getNormalIndices()[index + 2]);
	}
	if(hasColors) {
		indices.emplace_back(primitive.getColorIndices(0)->getIndices()[index + 2]);
	}
	if(hasUVCoords) {
		indices.emplace_back(primitive.getUVCoordIndices(0)->getIndices()[index + 2]);
	}

	indices.emplace_back(primitive.getPositionIndices()[index + 3]);
	if(hasNormals) {
		indices.emplace_back(primitive.getNormalIndices()[index + 3]);
	}
	if(hasColors) {
		indices.emplace_back(primitive.getColorIndices(0)->getIndices()[index + 3]);
	}
	if(hasUVCoords) {
		indices.emplace_back(primitive.getUVCoordIndices(0)->getIndices()[index + 3]);
	}
}

void extractPrimitive(const COLLADAFW::MeshPrimitive & primitive, const bool hasNormals, const bool hasColors, const bool hasUVCoords,
					  std::vector<uint32_t> & indices, uint32_t & triangleCount) {

	uint32_t indexPosition = 0;
	for(uint32_t i = 0; i < primitive.getGroupedVertexElementsCount(); ++i) {
		if(primitive.getGroupedVerticesVertexCount(i) == 3 || primitive.getGroupedVerticesVertexCount(i) == 0) {
			for(uint32_t j = 0; j < 3; ++j) {
				indices.emplace_back(primitive.getPositionIndices()[indexPosition + j]);
				if(hasNormals) {
					indices.emplace_back(primitive.getNormalIndices()[indexPosition + j]);
				}
				if(hasColors) {
					indices.emplace_back(primitive.getColorIndices(0)->getIndices()[indexPosition + j]);
				}
				if(hasUVCoords) {
					indices.emplace_back(primitive.getUVCoordIndices(0)->getIndices()[indexPosition + j]);
				}
			}
			indexPosition += 3;
			++triangleCount;
		} else if(primitive.getGroupedVerticesVertexCount(i) == 4) {
			triangulateQuadToTri(primitive, indexPosition, hasNormals, hasColors, hasUVCoords, indices);
			triangleCount += 2;
			indexPosition += 4;
		}
	}
}


bool nodeCoreImporter(const COLLADAFW::Geometry * geometry, referenceRegistry_t & referenceRegistry) {
	if(geometry->getType() != COLLADAFW::Geometry::GEO_TYPE_MESH) {
		return false;
	}

	auto sourceMesh = dynamic_cast<const COLLADAFW::Mesh *>(geometry);
	const COLLADAFW::MeshVertexData & pos = sourceMesh->getPositions();

	uint32_t maxOffset = 0;
	VertexPart vp;
	vp.type = VertexPart::POSITION;
	vp.stride = 3;
	vp.indexOffset = maxOffset;
	++maxOffset;

	{
		const auto & floatValues = *pos.getFloatValues();
		for(size_t i = 0; i < floatValues.getCount(); ++i) {
			vp.data.emplace_back(floatValues[i]);
		}
	}

	std::deque<VertexPart> queue;
	queue.emplace_back(vp);

	bool hasNormals = sourceMesh->hasNormals();
	if(hasNormals) {
		VertexPart np;
		np.type = VertexPart::NORMAL;
		const COLLADAFW::MeshVertexData & normalData = sourceMesh->getNormals();
		const auto & floatValues = *normalData.getFloatValues();
		for(size_t i = 0; i < floatValues.getCount(); ++i) {
			np.data.emplace_back(floatValues[i]);
		}
		np.indexOffset = maxOffset;
		maxOffset++;

		int normalStride = normalData.getStride(0);
		if(normalStride == 0) {
			normalStride = 3;
		}
		np.stride = normalStride;

		queue.emplace_back(np);
	}

	const COLLADAFW::MeshVertexData & colors = sourceMesh->getColors();
	bool hasColors = colors.getLength(0) > 0 ? true : false;
	if(hasColors) {
		VertexPart cp;
		cp.type = VertexPart::COLOR;
		const auto & floatValues = *colors.getFloatValues();
		for(uint32_t i = 0; i < floatValues.getCount(); ++i) {
			cp.data.emplace_back(floatValues[i]);
		}

		cp.stride = colors.getStride(0);
		if(cp.stride == 0) {
			cp.stride = 3;
		}

		cp.indexOffset = maxOffset;
		++maxOffset;
	}

	const COLLADAFW::MeshVertexData & uvCoords = sourceMesh->getUVCoords();
	bool hasUVCoords = uvCoords.getLength(0) > 0 ? true : false;
	if(hasUVCoords) {
		VertexPart uvp;
		uvp.type = VertexPart::TEXCOORD;
		const auto & floatValues = *uvCoords.getFloatValues();
		for(uint32_t i = 0; i < floatValues.getCount(); ++i) {
			uvp.data.emplace_back(floatValues[i]);
		}
		uvp.indexOffset = maxOffset;
		++maxOffset;

		uvp.stride = uvCoords.getStride(0);
		if(uvp.stride != 0) {
			queue.emplace_back(uvp);
		}
	}

	auto geometries = new SceneManagement::DescriptionMap;
	geometries->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
	geometries->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_LIST);
	const auto & meshPrimitives = sourceMesh->getMeshPrimitives();
	for(uint32_t i = 0; i < meshPrimitives.getCount(); ++i) {
		uint32_t triangleCount = 0;
		std::vector<uint32_t> indices;

		extractPrimitive(*meshPrimitives[i], hasNormals, hasColors, hasUVCoords, indices, triangleCount);

		std::vector<uint32_t> vertexIds;
		Rendering::Mesh * targetMesh = createSingleMesh(queue, indices, triangleCount, maxOffset, vertexIds);

		auto desc = new SceneManagement::DescriptionMap;
		desc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_DATA);
		desc->setString(SceneManagement::Consts::ATTR_DATA_TYPE, "mesh");

		desc->setValue(SceneManagement::Consts::ATTR_MESH_DATA, new Rendering::Serialization::MeshWrapper_t(targetMesh));
		desc->setValue(LoaderCOLLADA::Consts::DAE_MESH_VERTEXORDER, new SceneManagement::uint32VecWrapper_t(vertexIds));

		auto geoDataList = new SceneManagement::DescriptionArray;
		geoDataList->push_back(desc);

		auto geoNode = new SceneManagement::DescriptionMap();
		geoNode->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
		geoNode->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_GEOMETRY);

		geoNode->setValue(SceneManagement::Consts::CHILDREN, geoDataList);

		geoNode->setValue(LoaderCOLLADA::Consts::DAE_SUB_MATERIALID, new MaterialReference(meshPrimitives[i]->getMaterialId()));
		addToMinSGChildren(geometries, geoNode);
	}
	referenceRegistry[geometry->getUniqueId()] = geometries;

	return true;
}


}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
