/*
	This file is part of the MinSG library.
	Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "PbrMaterialState.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Shader/ShaderObjectInfo.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Draw.h>
#include <Rendering/FBO.h>
#include <Util/Hashing.h>

//--------------------

namespace MinSG {
using namespace Rendering;
using namespace Geometry;

//--------------------

static const Uniform::UniformName sg_pbrBaseColorFactor("sg_pbrBaseColorFactor");
static const Uniform::UniformName sg_pbrHasBaseColorTexture("sg_pbrHasBaseColorTexture");
static const Uniform::UniformName sg_pbrBaseColorTexCoord("sg_pbrBaseColorTexCoord");
static const Uniform::UniformName sg_pbrBaseColorTexTransform("sg_pbrBaseColorTexTransform");
static const Uniform::UniformName sg_pbrHasMetallicRoughnessTexture("sg_pbrHasMetallicRoughnessTexture");
static const Uniform::UniformName sg_pbrMetallicRoughnessTexCoord("sg_pbrMetallicRoughnessTexCoord");
static const Uniform::UniformName sg_pbrMetallicRoughnessTexTransform("sg_pbrMetallicRoughnessTexTransform");
static const Uniform::UniformName sg_pbrMetallicFactor("sg_pbrMetallicFactor");
static const Uniform::UniformName sg_pbrRoughnessFactor("sg_pbrRoughnessFactor");
static const Uniform::UniformName sg_pbrHasNormalTexture("sg_pbrHasNormalTexture");
static const Uniform::UniformName sg_pbrNormalTexCoord("sg_pbrNormalTexCoord");
static const Uniform::UniformName sg_pbrNormalTexTransform("sg_pbrNormalTexTransform");
static const Uniform::UniformName sg_pbrNormalScale("sg_pbrNormalScale");
static const Uniform::UniformName sg_pbrHasOcclusionTexture("sg_pbrHasOcclusionTexture");
static const Uniform::UniformName sg_pbrOcclusionTexCoord("sg_pbrOcclusionTexCoord");
static const Uniform::UniformName sg_pbrOcclusionTexTransform("sg_pbrOcclusionTexTransform");
static const Uniform::UniformName sg_pbrOcclusionStrength("sg_pbrOcclusionStrength");
static const Uniform::UniformName sg_pbrEmissiveFactor("sg_pbrEmissiveFactor");
static const Uniform::UniformName sg_pbrHasEmissiveTexture("sg_pbrHasEmissiveTexture");
static const Uniform::UniformName sg_pbrEmissiveTexCoord("sg_pbrEmissiveTexCoord");
static const Uniform::UniformName sg_pbrEmissiveTexTransform("sg_pbrEmissiveTexTransform");
static const Uniform::UniformName sg_pbrDoubleSided("sg_pbrDoubleSided");
static const Uniform::UniformName sg_pbrAlphaMode("sg_pbrAlphaMode");
static const Uniform::UniformName sg_pbrAlphaCutoff("sg_pbrAlphaCutoff");
static const Uniform::UniformName sg_pbrIOR("sg_pbrIOR");

//--------------------


//! [ctor]
PbrMaterialState::PbrMaterialState() : State() {
}

//--------------------

//! [dtor]
PbrMaterialState::~PbrMaterialState() = default;

//--------------------

PbrMaterialState * PbrMaterialState::clone() const {
	return new PbrMaterialState(*this);
}

//--------------------

bool PbrMaterialState::recreateShader(FrameContext& context) {
	auto& rc = context.getRenderingContext();
	
	// compute hash of values that might induce rebuild
	uint64_t hash = 0;
	Util::hash_combine(hash, material.shadingModel);
	Util::hash_combine(hash, material.alphaMode);
	Util::hash_combine(hash, material.doubleSided);
	Util::hash_combine(hash, material.useSkinning);
	Util::hash_combine(hash, material.useIBL);
	Util::hash_combine(hash, material.receiveShadow);
	Util::hash_combine(hash, material.baseColor.texture.get());
	Util::hash_combine(hash, material.metallicRoughness.texture.get());
	Util::hash_combine(hash, material.normal.texture.get());
	Util::hash_combine(hash, material.occlusion.texture.get());
	Util::hash_combine(hash, material.emissive.texture.get());
	Util::hash_combine(hash, material.baseColor.texUnit);
	Util::hash_combine(hash, material.metallicRoughness.texUnit);
	Util::hash_combine(hash, material.normal.texUnit);
	Util::hash_combine(hash, material.occlusion.texUnit);
	Util::hash_combine(hash, material.emissive.texUnit);
	Util::hash_combine(hash, material.baseColor.texCoord);
	Util::hash_combine(hash, material.metallicRoughness.texCoord);
	Util::hash_combine(hash, material.normal.texCoord);
	Util::hash_combine(hash, material.occlusion.texCoord);
	Util::hash_combine(hash, material.emissive.texCoord);

	if(lastHash != hash || !pbrShader) {
		auto vsFile = locator.locateFile(Util::FileName("shader/pbr/main.vert"));
		auto fsFile = locator.locateFile(Util::FileName("shader/pbr/main.frag"));
		WARN_AND_RETURN_IF(!vsFile.first, "Could not locate vertex shader file 'shader/pbr/main.vert'.", false);
		WARN_AND_RETURN_IF(!fsFile.first, "Could not locate fragment shader file 'shader/pbr/main.frag'.", false);
		// recreate shader
		auto vso = ShaderObjectInfo::loadVertex(vsFile.second);
		auto fso = ShaderObjectInfo::loadFragment(fsFile.second);
		
		switch(material.shadingModel) {
			case PbrShadingModel::Unlit:
				fso.addDefine("SHADING_MODEL_UNLIT", "");
				break;
			default:
				fso.addDefine("SHADING_MODEL_METALLIC_ROUGHNESS", "");
				break;
		}

		switch(material.alphaMode) {
			case PbrAlphaMode::Blend:
				fso.addDefine("ALPHA_MODE_BLEND", "");
				break;
			case PbrAlphaMode::Mask:
				fso.addDefine("ALPHA_MODE_MASK", "");
				break;
			default:
				fso.addDefine("ALPHA_MODE_OPAQUE", "");
				break;
		}

		if(material.doubleSided)
			fso.addDefine("DOUBLE_SIDED", "");
		if(material.useSkinning)
			vso.addDefine("USE_SKINNING", "");
		if(material.useIBL)
			fso.addDefine("USE_IBL", "");
		if(material.receiveShadow)
			fso.addDefine("RECEIVE_SHADOW", "");

		if(material.baseColor.texture) {
			fso.addDefine("HAS_BASECOLOR_TEXTURE", "");
			fso.addDefine("BASECOLOR_TEXCOORD", std::to_string(material.baseColor.texCoord));
			fso.addDefine("BASECOLOR_TEXUNIT", std::to_string(material.baseColor.texUnit));
			if(!material.baseColor.texture->getHasMipmaps())
				material.baseColor.texture->planMipmapCreation();
		}
		if(material.metallicRoughness.texture) {
			fso.addDefine("HAS_METALLICROUGHNESS_TEXTURE", "");
			fso.addDefine("METALLICROUGHNESS_TEXCOORD", std::to_string(material.metallicRoughness.texCoord));
			fso.addDefine("METALLICROUGHNESS_TEXUNIT", std::to_string(material.metallicRoughness.texUnit));
			if(!material.metallicRoughness.texture->getHasMipmaps())
				material.metallicRoughness.texture->planMipmapCreation();
		}
		if(material.normal.texture) {
			fso.addDefine("HAS_NORMAL_TEXTURE", "");
			fso.addDefine("NORMAL_TEXCOORD", std::to_string(material.normal.texCoord));
			fso.addDefine("NORMAL_TEXUNIT", std::to_string(material.normal.texUnit));
			if(!material.normal.texture->getHasMipmaps())
				material.normal.texture->planMipmapCreation();
		}
		if(material.occlusion.texture) {
			fso.addDefine("HAS_OCCLUSION_TEXTURE", "");
			fso.addDefine("OCCLUSION_TEXCOORD", std::to_string(material.occlusion.texCoord));
			fso.addDefine("OCCLUSION_TEXUNIT", std::to_string(material.occlusion.texUnit));
			if(!material.occlusion.texture->getHasMipmaps())
				material.occlusion.texture->planMipmapCreation();
		}
		if(material.emissive.texture) {
			fso.addDefine("HAS_EMISSIVE_TEXTURE", "");
			fso.addDefine("EMISSIVE_TEXCOORD", std::to_string(material.emissive.texCoord));
			fso.addDefine("EMISSIVE_TEXUNIT", std::to_string(material.emissive.texUnit));
			if(!material.emissive.texture->getHasMipmaps())
				material.emissive.texture->planMipmapCreation();
		}

		pbrShader = Shader::createShader();
		pbrShader->attachShaderObject(std::move(vso));
		pbrShader->attachShaderObject(std::move(fso));
		if(!pbrShader->init()) {
			pbrShader = nullptr;
			return false;
		}
	}

	lastHash = hash;
	dirty = false;
	return true;
}

//--------------------

//! ---|> [State]
State::stateResult_t PbrMaterialState::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	auto& rc = context.getRenderingContext();
	if(!pbrShader || dirty) {
		if(!recreateShader(context)) {
			deactivate();
			return State::STATE_SKIPPED;
		}
	}

	// if the current rendering channel is not the TRANSPARENCY_CHANNEL, try to pass the node to that channel.
	if(material.alphaMode == PbrAlphaMode::Blend && rp.getChannel() != FrameContext::TRANSPARENCY_CHANNEL) {
		RenderParam newRp(rp);
		newRp.setChannel(FrameContext::TRANSPARENCY_CHANNEL);
		if(context.displayNode(node, newRp)) {
			// successfully passed on to a renderer at that channel
			return State::STATE_SKIP_RENDERING;
		}
	}

	auto activeShader = pbrShader.get();
	if(rp.getFlag(NO_SHADING)) {
		activeShader = rc.getActiveShader();
	} else if(activeShader) {
		rc.pushAndSetShader(activeShader);
	}
	
	if(activeShader) {
		activeShader->setUniform(rc, Uniform(sg_pbrBaseColorFactor, material.baseColor.factor), false);
		activeShader->setUniform(rc, Uniform(sg_pbrBaseColorTexCoord, static_cast<int32_t>(material.baseColor.texCoord)), false);
		activeShader->setUniform(rc, Uniform(sg_pbrBaseColorTexTransform, material.baseColor.texTransform), false);
		activeShader->setUniform(rc, Uniform(sg_pbrMetallicRoughnessTexCoord, static_cast<int32_t>(material.metallicRoughness.texCoord)), false);
		activeShader->setUniform(rc, Uniform(sg_pbrMetallicRoughnessTexTransform, material.metallicRoughness.texTransform), false);
		activeShader->setUniform(rc, Uniform(sg_pbrMetallicFactor, material.metallicRoughness.metallicFactor), false);
		activeShader->setUniform(rc, Uniform(sg_pbrRoughnessFactor, material.metallicRoughness.roughnessFactor), false);
		activeShader->setUniform(rc, Uniform(sg_pbrNormalTexCoord, static_cast<int32_t>(material.normal.texCoord)), false);
		activeShader->setUniform(rc, Uniform(sg_pbrNormalTexTransform, material.normal.texTransform), false);
		activeShader->setUniform(rc, Uniform(sg_pbrNormalScale, material.normal.scale), false);
		activeShader->setUniform(rc, Uniform(sg_pbrOcclusionTexCoord, static_cast<int32_t>(material.occlusion.texCoord)), false);
		activeShader->setUniform(rc, Uniform(sg_pbrOcclusionTexTransform, material.occlusion.texTransform), false);
		activeShader->setUniform(rc, Uniform(sg_pbrOcclusionStrength, material.occlusion.strength), false);
		activeShader->setUniform(rc, Uniform(sg_pbrEmissiveTexCoord, static_cast<int32_t>(material.emissive.texCoord)), false);
		activeShader->setUniform(rc, Uniform(sg_pbrEmissiveTexTransform, material.emissive.texTransform), false);
		activeShader->setUniform(rc, Uniform(sg_pbrEmissiveFactor, material.emissive.factor), false);
		activeShader->setUniform(rc, Uniform(sg_pbrAlphaCutoff, material.alphaCutoff), false);
		activeShader->setUniform(rc, Uniform(sg_pbrIOR, material.ior), false);
	}

	if(material.baseColor.texture)
		rc.pushAndSetTexture(material.baseColor.texUnit, material.baseColor.texture.get());
	if(material.metallicRoughness.texture)
		rc.pushAndSetTexture(material.metallicRoughness.texUnit, material.metallicRoughness.texture.get());
	if(material.normal.texture)
		rc.pushAndSetTexture(material.normal.texUnit, material.normal.texture.get());
	if(material.occlusion.texture)
		rc.pushAndSetTexture(material.occlusion.texUnit, material.occlusion.texture.get());
	if(material.emissive.texture)
		rc.pushAndSetTexture(material.emissive.texUnit, material.emissive.texture.get());

	if(material.doubleSided) {
		rc.pushAndSetCullFace(CullFaceParameters());
	} else {
		rc.pushAndSetCullFace(CullFaceParameters::CULL_BACK);
	}

	rc.pushAlphaTest();
	rc.pushBlending();
	rc.pushDepthBuffer();
	switch(material.alphaMode) {
		case PbrAlphaMode::Mask: {
			rc.setAlphaTest(AlphaTestParameters(Comparison::LESS, material.alphaCutoff));
			break;
		}
		case PbrAlphaMode::Blend: {
			rc.setBlending(BlendingParameters(BlendingParameters::SRC_ALPHA, BlendingParameters::ONE_MINUS_SRC_ALPHA));
			rc.setDepthBuffer(DepthBufferParameters(true, false, Comparison::LESS));
			break;
		}
		default: break;
	}
	return State::STATE_OK;
}

//--------------------

//! ---|> [State]
void PbrMaterialState::doDisableState(FrameContext & context, Node * /*node*/, const RenderParam & rp) {
	auto& rc = context.getRenderingContext();

	if(pbrShader && !rp.getFlag(NO_SHADING)) {
		rc.popShader();
	}

	if(material.baseColor.texture)
		rc.popTexture(material.baseColor.texUnit);
	if(material.metallicRoughness.texture)
		rc.popTexture(material.metallicRoughness.texUnit);
	if(material.normal.texture)
		rc.popTexture(material.normal.texUnit);
	if(material.occlusion.texture)
		rc.popTexture(material.occlusion.texUnit);
	if(material.emissive.texture)
		rc.popTexture(material.emissive.texUnit);
	
	rc.popCullFace();
	rc.popAlphaTest();
	rc.popBlending();
	rc.popDepthBuffer();
}

//--------------------
}
