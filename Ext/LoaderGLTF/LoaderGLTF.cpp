/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "LoaderGLTF.h"
#include "../../SceneManagement/SceneDescription.h"
#include "../../SceneManagement/SceneManager.h"
#include "../../SceneManagement/ImportFunctions.h"
#include "../../SceneManagement/Importer/ImporterTools.h"
#include "../../SceneManagement/Exporter/ExporterTools.h"
#include "../../Core/Nodes/ListNode.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/VertexAccessor.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/RenderingContext/RenderingParameters.h>

#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/IO/FileLocator.h>
#include <Util/Macros.h>
#include <Util/Timer.h>
#include <Util/Resources/AttributeAccessor.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/Serialization/Serialization.h>

#include <Geometry/Matrix4x4.h>
#include <Geometry/Quaternion.h>
#include <Geometry/SRT.h>

#define REPEAT 10497
#define NEAREST 9728
#define NEAREST_MIPMAP_NEAREST 9984

#define TINYGLTF_IMPLEMENTATION
//#define TINYGLTF_NO_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_FS
#include <fstream>
#include <tiny_gltf.h>

#include <streambuf>
#include <istream>
#include <unordered_map>
#include <map>
#include <vector>

namespace MinSG {
namespace LoaderGLTF {
using namespace MinSG::SceneManagement;
using namespace Rendering;

//--------------------------

static const std::unordered_map<std::string, Util::StringIdentifier> remappedAttributeNames{
	{"POSITION", VertexAttributeIds::POSITION},
	{"NORMAL", VertexAttributeIds::NORMAL},
	{"TANGENT", VertexAttributeIds::TANGENT},
	{"COLOR_0", VertexAttributeIds::COLOR},
	{"TEXCOORD_0", VertexAttributeIds::TEXCOORD0},
	{"TEXCOORD_1", VertexAttributeIds::TEXCOORD1}
};

//--------------------------

struct membuf : std::streambuf {
	membuf(char const* base, size_t size) {
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
};

//--------------------------

struct imemstream : virtual membuf, std::istream {
	imemstream(char const* base, size_t size) : membuf(base, size), std::istream(static_cast<std::streambuf*>(this)) {}
};


//--------------------------

template<typename T>
inline std::string toString(const std::vector<T>& vec) {
	std::stringstream ss;
	for(const auto& v : vec)
		ss << " " << v;
	return ss.str();
}

//--------------------------

enum class AlphaMode : int32_t {
	Undefined = -1,
	Opaque = 0,
	Mask=1,
	Blend=2
};

inline AlphaMode decodeAlphaMode(const std::string& mode) {
	if(mode == "OPAQUE")
		return AlphaMode::Opaque;
	else if(mode == "MASK")
		return AlphaMode::Mask;
	else if(mode == "BLEND")
		return AlphaMode::Blend;
	else
		return AlphaMode::Undefined;
}

//--------------------------

static int32_t getAttributeIndexByName(tinygltf::Primitive& primitive, const std::string& name) {
	auto iterator = primitive.attributes.find(name);
	return iterator != primitive.attributes.end() ? iterator->second : -1;
}

Util::TypeConstant toTypeConstant(uint32_t componentType) {
	switch(componentType) {
		case TINYGLTF_COMPONENT_TYPE_BYTE: return Util::TypeConstant::INT8;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return Util::TypeConstant::UINT8;
		case TINYGLTF_COMPONENT_TYPE_SHORT: return Util::TypeConstant::INT16;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return Util::TypeConstant::UINT16;
		case TINYGLTF_COMPONENT_TYPE_INT: return Util::TypeConstant::INT32;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return Util::TypeConstant::UINT32;
		case TINYGLTF_COMPONENT_TYPE_FLOAT: return Util::TypeConstant::FLOAT;
		case TINYGLTF_COMPONENT_TYPE_DOUBLE: return Util::TypeConstant::DOUBLE;
		default: WARN_AND_RETURN("Unknown component type " + std::to_string(componentType), Util::TypeConstant::UINT8);
	}
}

//--------------------------

static Util::AttributeAccessor::Ref createAttributeAccessor(tinygltf::Model& model, int32_t accessorIndex) {
	if(accessorIndex < 0 || accessorIndex >= static_cast<int32_t>(model.accessors.size())) {
		return nullptr;
	}
	tinygltf::Accessor& accessor = model.accessors[accessorIndex];
	tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
	tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

	Util::TypeConstant componentType = toTypeConstant(accessor.componentType);
	uint32_t componentCount = tinygltf::GetNumComponentsInType(accessor.type);
	// Normalize vector types with integral types (byte, short, int)
	bool normalized = componentCount > 1 && componentType != Util::TypeConstant::FLOAT && componentType != Util::TypeConstant::DOUBLE;
	Util::AttributeFormat format({}, componentType, componentCount, normalized, 0, static_cast<uint32_t>(accessor.byteOffset));
	return Util::AttributeAccessor::create(buffer.data.data() + bufferView.byteOffset, static_cast<uint32_t>(bufferView.byteLength), format, static_cast<uint32_t>(bufferView.byteStride));
}

//--------------------------

static VertexDescription buildVertexDescription(const tinygltf::Model& model, const tinygltf::Primitive& primitive) {
	static const std::unordered_map<std::string, uint32_t> attributePriorities{
		{"POSITION", 0}, {"NORMAL", 1}, {"TANGENT", 2}, {"COLOR_0", 3}, {"TEXCOORD_0", 4}, {"TEXCOORD_1", 5}
	};

	// sort attribute names s.t. standard attributes are at the front
	using AttrEntry = std::pair<std::string,int>;
	std::vector<AttrEntry> names;
	for(const auto& attr : primitive.attributes)
		names.emplace_back(attr);
	std::sort(names.begin(), names.end(), [](const AttrEntry& attr0, const AttrEntry& attr1) {
		auto it0 = attributePriorities.find(attr0.first);
		auto it1 = attributePriorities.find(attr1.first);
		if(it0 == attributePriorities.end()) {
			return false;
		} else if(it1 == attributePriorities.end()) {
			return true;
		} else {
			return it0->second < it1->second;
		}
	});

	// create vertex description
	VertexDescription vd;
	for(const auto& attr : names) {
		const tinygltf::Accessor& accessor = model.accessors[attr.second];
		auto nameIt = remappedAttributeNames.find(attr.first);
		Util::StringIdentifier attrName = (nameIt != remappedAttributeNames.end()) ? nameIt->second : Util::StringIdentifier(attr.first);
		Util::TypeConstant componentType = toTypeConstant(accessor.componentType);
		uint32_t componentCount = tinygltf::GetNumComponentsInType(accessor.type);
		bool normalized = componentCount > 1 && componentType != Util::TypeConstant::FLOAT && componentType != Util::TypeConstant::DOUBLE;
		vd.appendAttribute(attrName, componentType, componentCount, normalized);
	}

	return vd;
}

//--------------------------

inline void addStringAttribute(const std::unique_ptr<DescriptionMap>& desc, const std::string& name, const std::string& value) {
	std::unique_ptr<DescriptionMap> attr(new DescriptionMap);
	attr->setString(Consts::TYPE, Consts::TYPE_ATTRIBUTE);
	attr->setString(Consts::ATTR_ATTRIBUTE_NAME, name);
	attr->setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_STRING);
	attr->setString(Consts::ATTR_ATTRIBUTE_VALUE, value);
	ExporterTools::addChildEntry(*desc.get(), std::move(attr));
}

//--------------------------

static std::unique_ptr<DescriptionMap> createUniformData(const std::string& type, const std::string& name, const std::string& data) {
	std::unique_ptr<DescriptionMap> nd(new DescriptionMap);
	nd->setString(Consts::TYPE, Consts::TYPE_DATA);
	nd->setString(Consts::ATTR_DATA_TYPE, Consts::DATA_TYPE_SHADER_UNIFORM);
	nd->setString(Consts::ATTR_SHADER_UNIFORM_NAME, name);
	nd->setString(Consts::ATTR_SHADER_UNIFORM_TYPE, type);
	nd->setString(Consts::ATTR_SHADER_UNIFORM_VALUES, data);
	return nd;
}

//--------------------------

class GLTFImportContext {
public:
	tinygltf::Model model;
	std::vector<Util::Reference<Rendering::Mesh>> meshes;
	std::vector<uint32_t> meshOffsets;
	std::vector<Util::Reference<Util::Bitmap>> images;
	std::vector<Util::Reference<Rendering::Texture>> textures;
	std::vector<bool> materialIsUsed;
	Util::FileLocator locator;
	bool hasBlendMaterial = false;

	Util::Reference<Rendering::Mesh> createMesh(uint32_t meshId, uint32_t primitiveId);
	std::unique_ptr<DescriptionMap> initMeshNode(uint32_t meshId, uint32_t primitiveId);
	Util::Reference<Rendering::Texture> createTexture(uint32_t textureId);
	std::unique_ptr<DescriptionMap> createTextureDescription(uint32_t textureId, uint32_t textureUnit);
	std::unique_ptr<DescriptionMap> createMaterialOrRef(uint32_t materialId);
	std::unique_ptr<DescriptionMap> initGeometryNode(uint32_t nodeId, uint32_t meshId);
	std::unique_ptr<DescriptionMap> createNode(uint32_t nodeId);
	std::unique_ptr<DescriptionMap> createScene(uint32_t sceneId);

	bool loadFile(const Util::FileName& filename);
};

//--------------------------

static bool fileExists(const std::string& absFileName, void* userData) {
	return Util::FileUtils::isFile(Util::FileName(absFileName));
}

//--------------------------

static std::string expandFilePath(const std::string& fileName, void* userData) {
	return fileName;
}

//--------------------------

static bool readWholeFile(std::vector<unsigned char>* result, std::string* errorMsg, const std::string& filePath, void* userData) {
	auto data = Util::FileUtils::loadFile(Util::FileName(filePath));
	if(data.empty()) {
		(*errorMsg) += "Failed to load file '" + filePath + "'";
		return false;
	}
	result->swap(data);
	return true;
}

//--------------------------

static bool writeWholeFile(std::string* errorMsg, const std::string& filePath, const std::vector<unsigned char>&, void* userData) {
	WARN("Writing glTF files is currently not supported.");
	return false;
}

//--------------------------

/*
static bool loadImageData(tinygltf::Image* image, const int imageIdx, std::string* errorMsg, std::string* warningMsg, int reqWidth, int reqHeight, const unsigned char* bytes, int size, void* userData) {
	Util::Reference<Util::Bitmap> bitmap;
	std::string extension;
	if(!image->uri.empty()) {
		extension = tinygltf::GetFilePathExtension(image->uri);
	} else {
		extension = tinygltf::MimeToExt(image->mimeType);
	}
	imemstream stream(reinterpret_cast<const char*>(bytes), static_cast<size_t>(size));
	bitmap = Util::Serialization::loadBitmap(extension, stream);

	if(!bitmap) {
		(*errorMsg) += "Failed to load image " + std::to_string(imageIdx) + " '" + image->uri + "'";
		return false;
	}

	GLTFImportContext* context = reinterpret_cast<GLTFImportContext*>(userData);
	if(imageIdx >= context->images.size())
		context->images.resize(imageIdx+1);
	context->images[imageIdx] = bitmap;
	return true;
}
*/

//--------------------------

Util::Reference<Rendering::Mesh> GLTFImportContext::createMesh(uint32_t meshId, uint32_t primitiveId) {
	tinygltf::Mesh& gltfMesh = model.meshes[meshId];
	tinygltf::Primitive& primitive = gltfMesh.primitives[primitiveId];
	tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

	int32_t positionAccessor = getAttributeIndexByName(primitive, "POSITION");
	WARN_AND_RETURN_IF(positionAccessor < 0, "Primitive " + std::to_string(primitiveId) + " of mesh " + std::to_string(meshId) + " has no POSITION attribute.", nullptr);
	uint32_t vertexCount = static_cast<uint32_t>(model.accessors[positionAccessor].count);
	uint32_t indexCount = static_cast<uint32_t>(indexAccessor.count);

	VertexDescription vd = buildVertexDescription(model, primitive);
	Util::Reference<Rendering::Mesh> mesh = new Rendering::Mesh(vd, vertexCount, indexCount);
	auto& indexData = mesh->openIndexData();
	auto& vertexData = mesh->openVertexData();

	// create accessors
	std::vector<Util::AttributeAccessor::Ref> meshAccessors(vd.getNumAttributes());
	std::vector<Util::AttributeAccessor::Ref> gtlfAccessors(vd.getNumAttributes());
	for(uint32_t location=0; location<vd.getNumAttributes(); ++location) {
		meshAccessors[location] = Util::AttributeAccessor::create(vertexData.data(), vertexData.dataSize(), vd.getAttribute(location), vd.getSize());
	}
	for(const auto& attr : primitive.attributes) {
		auto nameIt = remappedAttributeNames.find(attr.first);
		Util::StringIdentifier attrName = (nameIt != remappedAttributeNames.end()) ? nameIt->second : Util::StringIdentifier(attr.first);
		uint32_t location = vd.getAttributeLocation(attrName);
		gtlfAccessors[location] = createAttributeAccessor(model, attr.second);
	}

	// copy all attribute data to mesh
	for(uint32_t index=0; index<vertexCount; ++index) {
		for(uint32_t location=0; location<vd.getNumAttributes(); ++location) {
			uint8_t* ptr = meshAccessors[location]->_ptr<uint8_t>(index);
			gtlfAccessors[location]->readRaw(index, ptr, vd.getAttribute(location).getDataSize());
		}
	}
	
	// read index data
	if(primitive.indices >= 0) {
		Util::AttributeAccessor::Ref indexAttributeAccessor = createAttributeAccessor(model, primitive.indices);
		for(uint32_t index=0; index<indexCount; ++index) {
			indexData[index] = indexAttributeAccessor->readValue<uint32_t>(index);
		}
	}
	mesh->setGLDrawMode(primitive.mode);
	vertexData.updateBoundingBox();
	vertexData.markAsChanged();
	indexData.markAsChanged();

	return mesh;
}

//--------------------------

Util::Reference<Rendering::Texture> GLTFImportContext::createTexture(uint32_t textureId) {
	const auto& gltfTexture = model.textures[textureId];
	const auto& image = model.images[gltfTexture.source];
	Util::AttributeFormat pixelFormat({}, toTypeConstant(image.pixel_type), static_cast<uint32_t>(image.component));

	Texture::Format format;
	format.glTextureType = TextureUtils::textureTypeToGLTextureType(TextureType::TEXTURE_2D);
	format.sizeY = static_cast<uint32_t>(image.width);
	format.sizeX = static_cast<uint32_t>(image.height);
	format.numLayers = 1;
	format.numSamples = 1;
	format.pixelFormat = TextureUtils::pixelFormatToGLPixelFormat(pixelFormat);

	if(gltfTexture.sampler >= 0) {
		const auto& sampler = model.samplers[gltfTexture.sampler];
		format.glWrapS = sampler.wrapS;
		format.glWrapT = sampler.wrapT;
		format.glWrapR = sampler.wrapR;
		format.linearMagFilter = sampler.magFilter >= 0 && sampler.magFilter != NEAREST;
		format.linearMinFilter = sampler.minFilter >= 0 && sampler.minFilter != NEAREST && sampler.minFilter != NEAREST_MIPMAP_NEAREST;
	} else {
		format.glWrapS = REPEAT;
		format.glWrapT = REPEAT;
		format.glWrapR = REPEAT;
		format.linearMagFilter = true;
		format.linearMinFilter = true;
	}

	Util::Reference<Texture> texture = new Texture(format);
	texture->allocateLocalData();
	const uint8_t * pixels = image.image.data();
	WARN_AND_RETURN_IF(texture->getDataSize() != static_cast<uint32_t>(image.image.size()), "Failed to create Texture. Image and texture size do not match.", nullptr);

	std::copy(pixels, pixels + image.image.size(), texture->getLocalData());
	// Flip the rows.
	/*const uint32_t rowSize = format.sizeY * pixelFormat.getDataSize();
	for (uint_fast16_t row = 0; row < format.sizeX; ++row) {
		const uint32_t offset = row * rowSize;
		const uint16_t reverseRow = format.sizeX - 1 - row;
		const uint32_t reverseOffset = reverseRow * rowSize;
		std::copy(pixels + reverseOffset, pixels + reverseOffset + rowSize, texture->getLocalData() + offset);
	}*/

	texture->dataChanged();
	return texture;
}

//--------------------------

std::unique_ptr<DescriptionMap> GLTFImportContext::createTextureDescription(uint32_t textureId, uint32_t textureUnit) {
	auto texture = textures[textureId];
	std::unique_ptr<DescriptionMap> state(new DescriptionMap);
	state->setString(Consts::TYPE, Consts::TYPE_STATE);
	state->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_TEXTURE);
	state->setString(Consts::ATTR_TEXTURE_UNIT, std::to_string(textureUnit));
	if(texture) {
		std::unique_ptr<DescriptionMap> dataDesc(new DescriptionMap);
		dataDesc->setString(Consts::ATTR_DATA_TYPE, "image");
		dataDesc->setValue(Consts::ATTR_TEXTURE_DATA, new Util::ReferenceAttribute<Rendering::Texture>(texture.get()));
		ExporterTools::addDataEntry(*state.get(), std::move(dataDesc));
	}
	return state;
}

//--------------------------

std::unique_ptr<DescriptionMap> GLTFImportContext::createMaterialOrRef(uint32_t materialId) {
	const auto& gltfMaterial = model.materials[materialId];
	// material name
  std::string name = gltfMaterial.name;
	if(name.empty())
		name = "material_" + std::to_string(materialId);

	std::unique_ptr<DescriptionMap> material(new DescriptionMap);
	material->setString(Consts::TYPE, Consts::TYPE_STATE);

	if(materialIsUsed[materialId]) {
		material->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_REFERENCE);
		material->setString(Consts::ATTR_REFERENCED_STATE_ID, name);
		return material;
	}
	materialIsUsed[materialId] = true;
	

	AlphaMode alphaMode = decodeAlphaMode(gltfMaterial.alphaMode);
	material->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_GROUP);
  material->setString(Consts::ATTR_STATE_ID, name);
	if(alphaMode == AlphaMode::Blend)
		hasBlendMaterial = true;
	
	// mark group state as PBR-Material
	{
		std::unique_ptr<DescriptionMap> pbrMaterialAttr(new DescriptionMap);
		pbrMaterialAttr->setString(Consts::TYPE, Consts::TYPE_ATTRIBUTE);
		pbrMaterialAttr->setString(Consts::ATTR_ATTRIBUTE_NAME, "pbr_material");
		pbrMaterialAttr->setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_BOOL);
		pbrMaterialAttr->setString(Consts::ATTR_ATTRIBUTE_VALUE, "true");
		ExporterTools::addChildEntry(*material.get(), std::move(pbrMaterialAttr));
	}
	
	{ // shader state
		std::unique_ptr<DescriptionMap> state(new DescriptionMap);
		state->setString(Consts::TYPE, Consts::TYPE_STATE);
		state->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SHADER);
		// TODO: use shader variants based on material
		state->setString(Consts::ATTR_SHADER_NAME, "pbr_default.shader");
		ExporterTools::addChildEntry(*material.get(), std::move(state));
	}

	// uniform state
	{
		std::unique_ptr<DescriptionMap> uniform(new DescriptionMap);
		uniform->setString(Consts::TYPE, Consts::TYPE_STATE);
		uniform->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SHADER_UNIFORM);

		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_VEC4F, "sg_pbrBaseColorFactor", toString(gltfMaterial.pbrMetallicRoughness.baseColorFactor)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_BOOL, "sg_pbrHasBaseColorTexture", std::to_string(gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_INT, "sg_pbrBaseColorTexCoord", std::to_string(gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_BOOL, "sg_pbrHasMetallicRoughnessTexture", std::to_string(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_INT, "sg_pbrMetallicRoughnessTexCoord", std::to_string(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_FLOAT, "sg_pbrMetallicFactor", std::to_string(gltfMaterial.pbrMetallicRoughness.metallicFactor)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_FLOAT, "sg_pbrRoughnessFactor", std::to_string(gltfMaterial.pbrMetallicRoughness.roughnessFactor)));

		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_BOOL, "sg_pbrHasNormalTexture", std::to_string(gltfMaterial.normalTexture.index >= 0)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_INT, "sg_pbrNormalTexCoord", std::to_string(gltfMaterial.normalTexture.texCoord)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_FLOAT, "sg_pbrNormalScale", std::to_string(gltfMaterial.normalTexture.scale)));

		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_BOOL, "sg_pbrHasOcclusionTexture", std::to_string(gltfMaterial.occlusionTexture.index >= 0)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_INT, "sg_pbrOcclusionTexCoord", std::to_string(gltfMaterial.occlusionTexture.texCoord)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_FLOAT, "sg_pbrOcclusionStrength", std::to_string(gltfMaterial.occlusionTexture.strength)));

		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_VEC3F, "sg_pbrEmissiveFactor", toString(gltfMaterial.emissiveFactor)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_BOOL, "sg_pbrHasEmissiveTexture", std::to_string(gltfMaterial.emissiveTexture.index >= 0)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_INT, "sg_pbrEmissiveTexCoord", std::to_string(gltfMaterial.emissiveTexture.texCoord)));
		
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_BOOL, "sg_pbrDoubleSided", std::to_string(gltfMaterial.doubleSided)));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_INT, "sg_pbrAlphaMode", std::to_string(static_cast<int32_t>(alphaMode))));
		ExporterTools::addDataEntry(*uniform, createUniformData(Consts::SHADER_UNIFORM_TYPE_FLOAT, "sg_pbrAlphaCutoff", std::to_string(gltfMaterial.alphaCutoff)));
		ExporterTools::addChildEntry(*material.get(), std::move(uniform));
	}

	// twosided
	if(gltfMaterial.doubleSided) {
		std::unique_ptr<DescriptionMap> state(new DescriptionMap);
		state->setString(Consts::TYPE, Consts::TYPE_STATE);
		state->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_CULL_FACE);
		state->setString(Consts::ATTR_CULL_FACE, "DISABLED");
		ExporterTools::addChildEntry(*material.get(), std::move(state));
	}

	switch(alphaMode) {
		case AlphaMode::Mask: {
			// alpha test state
			std::unique_ptr<DescriptionMap> state(new DescriptionMap);
			state->setString(Consts::TYPE, Consts::TYPE_STATE);
			state->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_ALPHA_TEST);
			state->setString(Consts::ATTR_ALPHA_TEST_MODE, Comparison::functionToString(Comparison::LESS));
			state->setString(Consts::ATTR_ALPHA_REF_VALUE, std::to_string(gltfMaterial.alphaCutoff));
			ExporterTools::addChildEntry(*material.get(), std::move(state));
			break;
		}
		case AlphaMode::Blend: {
			// blend state
			std::unique_ptr<DescriptionMap> state(new DescriptionMap);
			state->setString(Consts::TYPE, Consts::TYPE_STATE);
			state->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_BLENDING);
			state->setString(Consts::ATTR_BLEND_FUNC_SRC, BlendingParameters::functionToString(BlendingParameters::SRC_ALPHA));
			state->setString(Consts::ATTR_BLEND_FUNC_DST, BlendingParameters::functionToString(BlendingParameters::ONE_MINUS_SRC_ALPHA));
			state->setString(Consts::ATTR_BLEND_DEPTH_MASK, std::to_string(false));
			ExporterTools::addChildEntry(*material.get(), std::move(state));
			break;
		}
		default: break;
	}

	// textures
	if(gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0) {
		auto state = createTextureDescription(gltfMaterial.pbrMetallicRoughness.baseColorTexture.index, 0);
		ExporterTools::addChildEntry(*material.get(), std::move(state));
	}
	if(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
		auto state = createTextureDescription(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index, 1);
		ExporterTools::addChildEntry(*material.get(), std::move(state));
	}
	if(gltfMaterial.normalTexture.index >= 0) {
		auto state = createTextureDescription(gltfMaterial.normalTexture.index, 2);
		ExporterTools::addChildEntry(*material.get(), std::move(state));
	}
	if(gltfMaterial.occlusionTexture.index >= 0) {
		auto state = createTextureDescription(gltfMaterial.occlusionTexture.index, 3);
		ExporterTools::addChildEntry(*material.get(), std::move(state));
	}
	if(gltfMaterial.emissiveTexture.index >= 0) {
		auto state = createTextureDescription(gltfMaterial.emissiveTexture.index, 4);
		ExporterTools::addChildEntry(*material.get(), std::move(state));
	}

	return material;
}

//--------------------------

std::unique_ptr<DescriptionMap> GLTFImportContext::initMeshNode(uint32_t meshId, uint32_t primitiveId) {
	const auto& gltfMesh = model.meshes[meshId];
	const auto& gltfPrimitive = gltfMesh.primitives[primitiveId];

	auto offset = meshOffsets[meshId];
	auto mesh = meshes[offset + primitiveId];
	std::unique_ptr<DescriptionMap> node(new DescriptionMap);
	node->setString(Consts::TYPE, Consts::TYPE_NODE);
	node->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_GEOMETRY);

	std::unique_ptr<DescriptionMap> dataDesc(new DescriptionMap);
	dataDesc->setString(Consts::ATTR_DATA_TYPE, "mesh");
	dataDesc->setValue(Consts::ATTR_MESH_DATA, new Util::ReferenceAttribute<Rendering::Mesh>(mesh.get()));
	ExporterTools::addDataEntry(*node.get(), std::move(dataDesc));

	// add material state
	if(gltfPrimitive.material >= 0) {
		auto material = createMaterialOrRef(gltfPrimitive.material);
		ExporterTools::addChildEntry(*node.get(), std::move(material));
	}
	return node;
}

//--------------------------

std::unique_ptr<DescriptionMap> GLTFImportContext::initGeometryNode(uint32_t nodeId, uint32_t meshId) {
	const auto& gltfNode = model.nodes[nodeId];
	const auto& gltfMesh = model.meshes[meshId];
	std::unique_ptr<DescriptionMap> node;

	uint32_t primitiveCount = static_cast<uint32_t>(gltfMesh.primitives.size());
	if(primitiveCount > 1) {
		node.reset(new DescriptionMap);
		node->setString(Consts::TYPE, Consts::TYPE_NODE);
		node->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIST);
		// create one geometry node for each primitive
		for(uint32_t i=0; i<primitiveCount; ++i) {
			ExporterTools::addChildEntry(*node.get(), std::move(initMeshNode(meshId, i)));
		}
	} else {
		node = initMeshNode(meshId, 0);
	}
	return node;
}

//--------------------------

std::unique_ptr<DescriptionMap> GLTFImportContext::createNode(uint32_t nodeId) {
	const auto& gltfNode = model.nodes[nodeId];
	std::unique_ptr<DescriptionMap> node;

	if(gltfNode.mesh >= 0) {
		if(gltfNode.children.empty()) {
			node = initGeometryNode(nodeId, static_cast<uint32_t>(gltfNode.mesh));
		} else {
			// create geometry node as child node
			node.reset(new DescriptionMap);
			node->setString(Consts::TYPE, Consts::TYPE_NODE);
			node->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIST);
			ExporterTools::addChildEntry(*node.get(), std::move(initGeometryNode(nodeId, static_cast<uint32_t>(gltfNode.mesh))));
		}
	} else {
		node.reset(new DescriptionMap);
		node->setString(Consts::TYPE, Consts::TYPE_NODE);
		node->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIST);
	}
	node->setString(Consts::ATTR_NODE_ID, gltfNode.name);
	
	// transformation
	if(!gltfNode.matrix.empty()) {
		node->setString(Consts::ATTR_MATRIX, toString(gltfNode.matrix));
	} else if(!gltfNode.scale.empty() && !(gltfNode.scale[0] == gltfNode.scale[1] && gltfNode.scale[1] == gltfNode.scale[2])) {
		// need to convert to matrix first
		Geometry::Matrix4x4 m;
		if(!gltfNode.translation.empty()) {
			m.translate(static_cast<float>(gltfNode.translation[0]), static_cast<float>(gltfNode.translation[1]), static_cast<float>(gltfNode.translation[2]));
		}
		if(!gltfNode.rotation.empty()) {
			Geometry::Quaternion q(static_cast<float>(gltfNode.rotation[0]), static_cast<float>(gltfNode.rotation[1]), static_cast<float>(gltfNode.rotation[2]), static_cast<float>(gltfNode.rotation[3]));
			m *= Geometry::Matrix4x4(q.toMatrix());
		}
		m *= Geometry::Matrix4x4::createScale(static_cast<float>(gltfNode.scale[0]), static_cast<float>(gltfNode.scale[1]), static_cast<float>(gltfNode.scale[2]));
		std::stringstream ss;
		ss << m;
		node->setString(Consts::ATTR_MATRIX, ss.str());
	} else {
		if(!gltfNode.translation.empty()) {
			node->setString(Consts::ATTR_SRT_POS, toString(gltfNode.translation));
		}
		if(!gltfNode.scale.empty()) {
			node->setString(Consts::ATTR_SRT_SCALE, std::to_string(gltfNode.scale[0]));
		}
		if(!gltfNode.rotation.empty()) {
			Geometry::Quaternion q(static_cast<float>(gltfNode.rotation[0]), static_cast<float>(gltfNode.rotation[1]), static_cast<float>(gltfNode.rotation[2]), static_cast<float>(gltfNode.rotation[3]));
			Geometry::Matrix3x3 r = q.toMatrix();
			{
				auto dir = r.getCol(Geometry::Matrix3x3::FRONT);
				std::stringstream ss;
				ss << dir;
				node->setString(Consts::ATTR_SRT_DIR, ss.str());
			}
			{
				auto up = r.getCol(Geometry::Matrix3x3::UP);
				std::stringstream ss;
				ss << up;
				node->setString(Consts::ATTR_SRT_UP, ss.str());
			}
		}
	}

	// child nodes
	for(uint32_t childId : gltfNode.children) {
		auto child = createNode(childId);
		ExporterTools::addChildEntry(*node.get(), std::move(child));
	}

	return node;
}

//--------------------------

std::unique_ptr<DescriptionMap> GLTFImportContext::createScene(uint32_t sceneId) {
	const auto& gltfScene = model.scenes[sceneId];
	std::unique_ptr<DescriptionMap> scene(new DescriptionMap);
	scene->setString(Consts::TYPE, Consts::TYPE_NODE);
	scene->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIST);
	scene->setString(Consts::ATTR_NODE_ID, gltfScene.name);
	addStringAttribute(scene, "META_NAME", gltfScene.name);
	addStringAttribute(scene, "META_LICENSE", model.asset.copyright);
	addStringAttribute(scene, "META_NOTE", model.asset.generator);

	for(uint32_t node : gltfScene.nodes) {
		ExporterTools::addChildEntry(*scene.get(), createNode(node));
	}	
	
	if(hasBlendMaterial) { // transparency render state
		std::unique_ptr<DescriptionMap> state(new DescriptionMap);
		state->setString(Consts::TYPE, Consts::TYPE_STATE);
		state->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_TRANSPARENCY_RENDERER);
		ExporterTools::addChildEntry(*scene.get(), std::move(state));
	}

	return scene;
}

//--------------------------

bool GLTFImportContext::loadFile(const Util::FileName& filename) {
	tinygltf::TinyGLTF loader;
	std::string errorMsg;
	std::string warningMsg;
	locator.addSearchPath(filename.getFSName() + "://" + filename.getDir());
	
	tinygltf::FsCallbacks fsCallbacks;
	fsCallbacks.FileExists = &fileExists;
	fsCallbacks.ExpandFilePath = &expandFilePath;
	fsCallbacks.ReadWholeFile = &readWholeFile;
	fsCallbacks.WriteWholeFile = &writeWholeFile;
	fsCallbacks.user_data = this;
	loader.SetFsCallbacks(fsCallbacks);
	//loader.SetImageLoader(&loadImageData, this);

	bool success = false;
	if(filename.getEnding() == "glb" || filename.getEnding() == "GLB") {
		success = loader.LoadBinaryFromFile(&model, &errorMsg, &warningMsg, filename.getPath());
	} else {
		success = loader.LoadASCIIFromFile(&model, &errorMsg, &warningMsg, filename.getPath());
	}
	
	WARN_IF(!warningMsg.empty(), warningMsg);
	WARN_IF(!errorMsg.empty(), errorMsg);
	WARN_AND_RETURN_IF(!success, "Failed to parse glTF", false);

	// create meshes
	for(uint32_t meshId=0; meshId<model.meshes.size(); ++meshId) {
		meshOffsets.emplace_back(static_cast<uint32_t>(meshes.size()));
		for(uint32_t primitiveId=0; primitiveId<model.meshes[meshId].primitives.size(); ++primitiveId) {
			meshes.emplace_back(createMesh(meshId, primitiveId));
		}
	}

	// create textures
	textures.resize(model.textures.size());
	for(uint32_t textureId=0; textureId<model.textures.size(); ++textureId) {
		textures[textureId] = createTexture(textureId);
	}

	// init material counts
	materialIsUsed.resize(model.materials.size());

	return true;
}

//--------------------------

const SceneManagement::DescriptionMap* loadScene(const Util::FileName & fileName, const importOption_t importOptions) {
	GLTFImportContext importer;
	if(!importer.loadFile(fileName))
		return nullptr;
	
	std::unique_ptr<DescriptionMap> scene(new DescriptionMap);
	scene->setString(Consts::TYPE, Consts::TYPE_SCENE);
	
	for(uint32_t sceneId=0; sceneId<importer.model.scenes.size(); ++sceneId) {
		ExporterTools::addChildEntry(*scene.get(), importer.createScene(sceneId));
	}
	return scene.release();
}

//--------------------------

std::vector<Util::Reference<Node>> loadGLTFScene(SceneManagement::SceneManager& sm, const Util::FileName & fileName, const importOption_t importOptions) {
	auto importContext = SceneManagement::createImportContext(sm, importOptions);
	return loadGLTFScene(importContext, fileName);
}

//--------------------------

std::vector<Util::Reference<Node>> loadGLTFScene(SceneManagement::ImportContext& importContext, const Util::FileName & fileName) {
	importContext.setFileName(fileName);

	// parse glTF and create description
	bool found;
	Util::FileName fullPath;
	std::tie(found, fullPath) = importContext.fileLocator.locateFile(fileName);
	if(!found) {
		WARN("Assimp: Could not find file '" + fileName.toString() + "'.");
		return {};
	}
	std::unique_ptr<const DescriptionMap> sceneDescription(loadScene(fullPath, importContext.importOptions));

	// create MinSG scene tree from description with dummy root node
	Util::Reference<ListNode> dummyNode=new ListNode;
	importContext.setRootNode(dummyNode.get());

	ImporterTools::buildSceneFromDescription(importContext, sceneDescription.get());
	
	// detach nodes from dummy root node
	std::vector<Util::Reference<Node>> nodes;
	const auto nodesTmp = getChildNodes(dummyNode.get());
	for(const auto & node : nodesTmp) {
		nodes.push_back(node);
		node->removeFromParent();
	}
	importContext.setRootNode(nullptr);
	return nodes;
}

} // LoaderGLTF
} // MinSG