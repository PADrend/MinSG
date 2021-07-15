/*
	This file is part of the MinSG library.
	Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef PbrMaterialState_H
#define PbrMaterialState_H

#include "../../Core/States/State.h"
#include <Rendering/Shader/Shader.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileLocator.h>
#include <Util/Graphics/Color.h>
#include <Geometry/Vec3.h>
#include <Geometry/Matrix3x3.h>

namespace Rendering {
class Texture;
class Shader;
class Uniform;
} // Rendering

namespace MinSG {

enum class PbrAlphaMode : int32_t {
	Undefined = -1,
	Opaque = 0,
	Mask=1,
	Blend=2
};

enum class PbrShadingModel : int32_t {
	MetallicRoughness = 0,
	Unlit
};

struct PbrBaseColor {
	Util::Color4f factor = {1.0f, 1.0f, 1.0f, 1.0f};
	Util::Reference<Rendering::Texture> texture;
	uint32_t texCoord = 0;
	uint8_t texUnit = 0;
	Geometry::Matrix3x3 texTransform;
};

struct PbrMetallicRoughness {
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	Util::Reference<Rendering::Texture> texture;
	uint32_t texCoord = 0;
	uint8_t texUnit = 1;
	Geometry::Matrix3x3 texTransform;
};

struct PbrNormal {
	float scale = 1.0f;
	Util::Reference<Rendering::Texture> texture;
	uint32_t texCoord = 0;
	uint8_t texUnit = 2;
	Geometry::Matrix3x3 texTransform;
};

struct PbrOcclusion {
	float strength = 1.0;
	Util::Reference<Rendering::Texture> texture;
	uint32_t texCoord = 0;
	uint8_t texUnit = 3;
	Geometry::Matrix3x3 texTransform;
};

struct PbrEmissive {
	Geometry::Vec3 factor = {0.0f, 0.0f, 0.0f};
	Util::Reference<Rendering::Texture> texture;
	uint32_t texCoord = 0;
	uint8_t texUnit = 4;
	Geometry::Matrix3x3 texTransform;
};

struct PbrMaterial {
	PbrBaseColor baseColor;
	PbrMetallicRoughness metallicRoughness;
	PbrNormal normal;
	PbrOcclusion occlusion;
	PbrEmissive emissive;
	PbrAlphaMode alphaMode = PbrAlphaMode::Opaque;
	float alphaCutoff = 0.5f;
	bool doubleSided = false;
	bool useSkinning = false;
	bool useIBL = true;
	bool receiveShadow = true;
	PbrShadingModel shadingModel = PbrShadingModel::MetallicRoughness;
};

/**
 * @brief Combined state for easier handling of PBR materials
 *  [PbrMaterialState] ---|> [State]
 * @ingroup states
 */
class PbrMaterialState : public State {
		PROVIDES_TYPE_NAME(PbrMaterialState)
	public:

		MINSGAPI PbrMaterialState();
		MINSGAPI virtual ~PbrMaterialState();

		// ---|> [State]
		MINSGAPI PbrMaterialState * clone() const override;

		void setMaterial(const PbrMaterial& mat) { material = mat; dirty = true; }
		const PbrMaterial& getMaterial() const { return material; }
		PbrMaterial& getMaterial() { dirty = true; return material; }

		Util::Color4f getBaseColorFactor() const { return material.baseColor.factor; }
		void setBaseColorFactor(const Util::Color4f& v) { material.baseColor.factor = v; }
		uint32_t getBaseColorTexCoord() const { return material.baseColor.texCoord; }
		void setBaseColorTexCoord(uint32_t v) { material.baseColor.texCoord = v; dirty = true; }
		uint32_t getBaseColorTexUnit() const { return material.baseColor.texUnit; }
		void setBaseColorTexUnit(uint32_t v) { material.baseColor.texUnit = static_cast<uint8_t>(v); dirty = true; }
		const Util::Reference<Rendering::Texture>& getBaseColorTexture() const { return material.baseColor.texture; }
		void setBaseColorTexture(const Util::Reference<Rendering::Texture>& v) { material.baseColor.texture = v; dirty = true; }
		void setBaseColorTexTransform(const Geometry::Matrix3x3& mat) { material.baseColor.texTransform = mat; }
		const Geometry::Matrix3x3& getBaseColorTexTransform() const { return material.baseColor.texTransform; }

		float getMetallicFactor() const { return material.metallicRoughness.metallicFactor; }
		void setMetallicFactor(float v) { material.metallicRoughness.metallicFactor = v; }
		float getRoughnessFactor() const { return material.metallicRoughness.roughnessFactor; }
		void setRoughnessFactor(float v) { material.metallicRoughness.roughnessFactor = v; }
		uint32_t getMetallicRoughnessTexCoord() const { return material.metallicRoughness.texCoord; }
		void setMetallicRoughnessTexCoord(uint32_t v) { material.metallicRoughness.texCoord = v; dirty = true; }
		uint32_t getMetallicRoughnessTexUnit() const { return material.metallicRoughness.texUnit; }
		void setMetallicRoughnessTexUnit(uint32_t v) { material.metallicRoughness.texUnit = static_cast<uint8_t>(v); dirty = true; }
		const Util::Reference<Rendering::Texture>& getMetallicRoughnessTexture() const { return material.metallicRoughness.texture; }
		void setMetallicRoughnessTexture(const Util::Reference<Rendering::Texture>& v) { material.metallicRoughness.texture = v; dirty = true; }
		void setMetallicRoughnessTexTransform(const Geometry::Matrix3x3& mat) { material.metallicRoughness.texTransform = mat; }
		const Geometry::Matrix3x3& getMetallicRoughnessTexTransform() const { return material.metallicRoughness.texTransform; }

		float getNormalScale() const { return material.normal.scale; }
		void setNormalScale(float v) { material.normal.scale = v; }
		uint32_t getNormalTexCoord() const { return material.normal.texCoord; }
		void setNormalTexCoord(uint32_t v) { material.normal.texCoord = v; dirty = true; }
		uint32_t getNormalTexUnit() const { return material.normal.texUnit; }
		void setNormalTexUnit(uint32_t v) { material.normal.texUnit = static_cast<uint8_t>(v); dirty = true; }
		const Util::Reference<Rendering::Texture>& getNormalTexture() const { return material.normal.texture; }
		void setNormalTexture(const Util::Reference<Rendering::Texture>& v) { material.normal.texture = v; dirty = true; }
		void setNormalTexTransform(const Geometry::Matrix3x3& mat) { material.normal.texTransform = mat; }
		const Geometry::Matrix3x3& getNormalTexTransform() const { return material.normal.texTransform; }

		float getOcclusionStrength() const { return material.occlusion.strength; }
		void setOcclusionStrength(float v) { material.occlusion.strength = v; }
		uint32_t getOcclusionTexCoord() const { return material.occlusion.texCoord; }
		void setOcclusionTexCoord(uint32_t v) { material.occlusion.texCoord = v; dirty = true; }
		uint32_t getOcclusionTexUnit() const { return material.occlusion.texUnit; }
		void setOcclusionTexUnit(uint32_t v) { material.occlusion.texUnit = static_cast<uint8_t>(v); dirty = true; }
		const Util::Reference<Rendering::Texture>& getOcclusionTexture() const { return material.occlusion.texture; }
		void setOcclusionTexture(const Util::Reference<Rendering::Texture>& v) { material.occlusion.texture = v; dirty = true; }
		void setOcclusionTexTransform(const Geometry::Matrix3x3& mat) { material.occlusion.texTransform = mat; }
		const Geometry::Matrix3x3& getOcclusionTexTransform() const { return material.occlusion.texTransform; }

		Geometry::Vec3 getEmissiveFactor() const { return material.emissive.factor; }
		void setEmissiveFactor(const Geometry::Vec3& v) { material.emissive.factor = v; }
		uint32_t getEmissiveTexCoord() const { return material.emissive.texCoord; }
		void setEmissiveTexCoord(uint32_t v) { material.emissive.texCoord = v; dirty = true; }
		uint32_t getEmissiveTexUnit() const { return material.emissive.texUnit; }
		void setEmissiveTexUnit(uint32_t v) { material.emissive.texUnit = static_cast<uint8_t>(v); dirty = true; }
		const Util::Reference<Rendering::Texture>& getEmissiveTexture() const { return material.emissive.texture; }
		void setEmissiveTexture(const Util::Reference<Rendering::Texture>& v) { material.emissive.texture = v; dirty = true; }
		void setEmissiveTexTransform(const Geometry::Matrix3x3& mat) { material.emissive.texTransform = mat; }
		const Geometry::Matrix3x3& getEmissiveTexTransform() const { return material.emissive.texTransform; }

		PbrAlphaMode getAlphaMode() const { return material.alphaMode; }
		void setAlphaMode(PbrAlphaMode v) { material.alphaMode = v; dirty = true; }
		float getAlphaCutoff() const { return material.alphaCutoff; }
		void setAlphaCutoff(float v) { material.alphaCutoff = v; }
		bool getDoubleSided() const { return material.doubleSided; }
		void setDoubleSided(bool v) { material.doubleSided = v; dirty = true; }
		bool getUseSkinning() const { return material.useSkinning; }
		void setUseSkinning(bool v) { material.useSkinning = v; dirty = true; }
		bool getUseIBL() const { return material.useIBL; }
		void setUseIBL(bool v) { material.useIBL = v; dirty = true; }
		bool getReceiveShadow() const { return material.receiveShadow; }
		void setReceiveShadow(bool v) { material.receiveShadow = v; dirty = true; }
		PbrShadingModel getShadingModel() const { return material.shadingModel; }
		void setShadingModel(PbrShadingModel v) { material.shadingModel = v; dirty = true; }

		Rendering::Shader* getShader() const { return pbrShader.get(); }
		void addSearchPath(const std::string& path) { locator.addSearchPath(path); }
		void setSearchPaths(const std::vector<std::string>& paths) { locator.setSearchPaths(paths); }
	private:
		MINSGAPI stateResult_t doEnableState(FrameContext& context, Node* node, const RenderParam& rp) override;
		MINSGAPI void doDisableState(FrameContext& context, Node* node, const RenderParam& rp) override;
		bool recreateShader(FrameContext& context);

		PbrMaterial material;
		Util::FileLocator locator;
		Util::Reference<Rendering::Shader> pbrShader;
		bool dirty = true;
		uint64_t lastHash = 0;
};

}

#endif // PbrMaterialState_H
