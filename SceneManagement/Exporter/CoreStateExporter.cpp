/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CoreStateExporter.h"

#include "ExporterContext.h"
#include "ExporterTools.h"

#include "../SceneManager.h"
#include "../SceneDescription.h"

#include "../../Core/States/ShaderState.h"
#include "../../Core/States/ShaderUniformState.h"
#include "../../Core/States/TextureState.h"
#include "../../Core/States/GroupState.h"
#include "../../Core/States/PolygonModeState.h"
#include "../../Core/States/AlphaTestState.h"
#include "../../Core/States/CullFaceState.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/LightingState.h"
#include "../../Core/States/BlendingState.h"

#include "../../Core/States/TransparencyRenderer.h"

#include <Rendering/Shader/Uniform.h>
#include <Rendering/Texture/TextureUtils.h>

#include <Util/Serialization/Serialization.h>
#include <Util/IO/FileUtils.h>
#include <Util/Encoding.h>
#include <Util/Graphics/Bitmap.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <vector>

namespace MinSG {
namespace SceneManagement {

static NodeDescription createDescriptionForUniform(const Rendering::Uniform & u) {

	NodeDescription nd;
	nd.setString(Consts::TYPE, Consts::TYPE_DATA);
	nd.setString(Consts::ATTR_DATA_TYPE, Consts::DATA_TYPE_SHADER_UNIFORM);
	nd.setString(Consts::ATTR_SHADER_UNIFORM_NAME, u.getName());

	using Rendering::Uniform;
	const Uniform::dataType_t type = u.getType();
	switch(type) {
		case Uniform::UNIFORM_BOOL:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_BOOL);
			break;
		case Uniform::UNIFORM_VEC2B:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC2B);
			break;
		case Uniform::UNIFORM_VEC3B:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC3B);
			break;
		case Uniform::UNIFORM_VEC4B:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC4B);
			break;
		case Uniform::UNIFORM_INT:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_INT);
			break;
		case Uniform::UNIFORM_VEC2I:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC2I);
			break;
		case Uniform::UNIFORM_VEC3I:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC3I);
			break;
		case Uniform::UNIFORM_VEC4I:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC4I);
			break;
		case Uniform::UNIFORM_FLOAT:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_FLOAT);
			break;
		case Uniform::UNIFORM_VEC2F:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC2F);
			break;
		case Uniform::UNIFORM_VEC3F:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC3F);
			break;
		case Uniform::UNIFORM_VEC4F:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_VEC4F);
			break;
		case Uniform::UNIFORM_MATRIX_2X2F:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_MATRIX_2X2F);
			break;
		case Uniform::UNIFORM_MATRIX_3X3F:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_MATRIX_3X3F);
			break;
		case Uniform::UNIFORM_MATRIX_4X4F:
			nd.setString(Consts::ATTR_SHADER_UNIFORM_TYPE, Consts::SHADER_UNIFORM_TYPE_MATRIX_4X4F);
			break;
		default:
			WARN("unexpected case in switch statement");
	}

	std::stringstream s;
	switch(type) {
		case Uniform::UNIFORM_BOOL:
		case Uniform::UNIFORM_VEC2B:
		case Uniform::UNIFORM_VEC3B:
		case Uniform::UNIFORM_VEC4B: {
			const size_t numSingleValues = u.getDataSize() / sizeof(int32_t);
			const int32_t * ptr = reinterpret_cast<const int32_t *>(u.getData());
			for(size_t i=0; i<numSingleValues; ++i)
				s << " " << (ptr[i] ? 1 : 0);
			break;
		}
		case Uniform::UNIFORM_INT:
		case Uniform::UNIFORM_VEC2I:
		case Uniform::UNIFORM_VEC3I:
		case Uniform::UNIFORM_VEC4I: {
			const size_t numSingleValues = u.getDataSize() / sizeof(int32_t);
			const int32_t * ptr = reinterpret_cast<const int32_t *>(u.getData());
			for(size_t i=0; i<numSingleValues; ++i)
				s << " " << ptr[i];
			break;
		}

		case Uniform::UNIFORM_FLOAT:
		case Uniform::UNIFORM_VEC2F:
		case Uniform::UNIFORM_VEC3F:
		case Uniform::UNIFORM_VEC4F:
		case Uniform::UNIFORM_MATRIX_2X2F:
		case Uniform::UNIFORM_MATRIX_3X3F:
		case Uniform::UNIFORM_MATRIX_4X4F: {
			const size_t numSingleValues = u.getDataSize() / sizeof(float);
			const float * ptr = reinterpret_cast<const float *>(u.getData());
			for(size_t i=0; i<numSingleValues; ++i)
				s << " " << ptr[i];
			break;
		}
		default:
			WARN("unexpected case in switch statement");
	}
	nd.setString(Consts::ATTR_SHADER_UNIFORM_VALUES, s.str());
	return nd;
}

static void describeShaderState(ExporterContext &,NodeDescription & desc,State * state) {
	desc.setString(Consts::ATTR_NODE_TYPE, Consts::STATE_TYPE_SHADER);
	auto shaderState = dynamic_cast<ShaderState *>(state);

	for(const auto & uniformEntry : shaderState->getUniforms())
		ExporterTools::addDataEntry(desc,std::move(createDescriptionForUniform(uniformEntry.second)));

	// The shader is either described by a shader name OR by the list of files and properties
	Util::GenericAttribute* shaderNameAttr = state->getAttribute(Consts::STATE_ATTR_SHADER_NAME);
	if(shaderNameAttr&&!shaderNameAttr->toString().empty()){
		desc.setString(Consts::ATTR_SHADER_NAME, shaderNameAttr->toString());
	}else{
		// add shader usage type
		const bool usesGL = shaderState->getShader() ? shaderState->getShader()->usesClassicOpenGL() : true;
		desc.setString(Consts::ATTR_SHADER_USES_CLASSIC_GL , usesGL ? "true" : "false");

		const bool usesUniforms = shaderState->getShader() ? shaderState->getShader()->usesSGUniforms() : true;
		desc.setString(Consts::ATTR_SHADER_USES_SG_UNIFORMS , usesUniforms ? "true" : "false");

		// The shader's files are stored as attributes:  { Consts::STATE_ATTR_SHADER_FILES : [ fileDescriptor* ] } 
		Util::GenericAttributeList * shaderFilesAttribute = dynamic_cast<Util::GenericAttributeList *>(state->getAttribute(Consts::STATE_ATTR_SHADER_FILES));
		if(shaderFilesAttribute){
			for(const auto & a : *shaderFilesAttribute){
				const auto a2 = dynamic_cast<const NodeDescription*>(a.get());
				if(a2){
					std::unique_ptr<NodeDescription> fileDescription(a2->clone());
					ExporterTools::addDataEntry(desc,std::move(*fileDescription));
				}
			}
		}

	}
	

}

static void describeTextureState(ExporterContext & /*ctxt*/,NodeDescription & desc,State * state) {
	auto ts = dynamic_cast<TextureState *>(state);
	
	desc.setString(Consts::ATTR_STATE_TYPE,Consts::STATE_TYPE_TEXTURE);
	if(ts->getTextureUnit()!=0)
		desc.setString(Consts::ATTR_TEXTURE_UNIT, Util::StringUtils::toString(ts->getTextureUnit()));

	Rendering::Texture * texture = ts->getTexture();
	if(texture!=nullptr) {
		Util::FileName texFilename(texture->getFileName());

		if(texFilename.empty()) {
			auto bitmap = Rendering::TextureUtils::createBitmapFromLocalTexture(texture);

			std::ostringstream stream;
			if(bitmap.isNotNull() && Util::Serialization::saveBitmap(*bitmap.get(), "png", stream)) {
				const std::string streamString = stream.str();
				const std::string encodedData = Util::encodeBase64(std::vector<uint8_t>(streamString.begin(), streamString.end()));

				NodeDescription dataDesc;
				dataDesc.setString(Consts::ATTR_DATA_TYPE,"image");
				dataDesc.setString(Consts::ATTR_DATA_ENCODING,"base64");
				dataDesc.setString(Consts::ATTR_DATA_FORMAT,"png");
				dataDesc.setString(Consts::DATA_BLOCK,encodedData);
				ExporterTools::addDataEntry(desc, std::move(dataDesc));
			}
		} else {
//			// make path to texture relative to scene (if mesh lies below the scene)
//			Util::FileUtils::makeRelativeIfPossible(ctxt.sceneFile, texFilename);
			

			NodeDescription dataDesc;
			dataDesc.setString(Consts::ATTR_DATA_TYPE,"image");
			dataDesc.setString(Consts::ATTR_TEXTURE_FILENAME, texFilename.toShortString());
			ExporterTools::addDataEntry(desc, std::move(dataDesc));

		}
	}
}

static void describeBlendingState(ExporterContext &,NodeDescription & desc,State * state) {
	auto n=dynamic_cast<BlendingState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_BLENDING);
	desc.setString(Consts::ATTR_BLEND_EQUATION, Rendering::BlendingParameters::equationToString(n->getParameters().getBlendEquationRGB()));
	desc.setString(Consts::ATTR_BLEND_FUNC_SRC, Rendering::BlendingParameters::functionToString(n->getParameters().getBlendFuncSrcRGB()));
	desc.setString(Consts::ATTR_BLEND_FUNC_DST, Rendering::BlendingParameters::functionToString(n->getParameters().getBlendFuncDstRGB()));
	desc.setString(Consts::ATTR_BLEND_CONST_ALPHA, Util::StringUtils::toString(n->getParameters().getBlendColor().getA()));
	desc.setString(Consts::ATTR_BLEND_DEPTH_MASK, Util::StringUtils::toString(n->getBlendDepthMask()));
}

static void describeCullFaceState(ExporterContext &,NodeDescription & desc,State * state) {
	auto s = dynamic_cast<CullFaceState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_CULL_FACE);
	std::string mode("INVALID");
	if(s->getParameters().isEnabled()) {
		switch(s->getParameters().getMode()) {
			case Rendering::CullFaceParameters::CULL_BACK:
				mode = "BACK";
				break;
			case Rendering::CullFaceParameters::CULL_FRONT:
				mode = "FRONT";
				break;
			case Rendering::CullFaceParameters::CULL_FRONT_AND_BACK:
				mode = "FRONT_AND_BACK";
				break;
			default:
				break;
		}
	} else {
		mode = "DISABLED";
	}
	desc.setString(Consts::ATTR_CULL_FACE, mode);
}

static void describeAlphaTestState(ExporterContext &,NodeDescription & desc,State * state) {
	auto n = dynamic_cast<AlphaTestState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_ALPHA_TEST);
	desc.setString(Consts::ATTR_ALPHA_TEST_MODE, Util::StringUtils::toString(static_cast<signed>(n->getParameters().getMode())));
	desc.setString(Consts::ATTR_ALPHA_REF_VALUE, Util::StringUtils::toString(n->getParameters().getReferenceValue()));
}

static void describeGroupState(ExporterContext & ctxt,NodeDescription & desc,State * state) {
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_GROUP);

	for(const auto & childState : dynamic_cast<GroupState *>(state)->getStates()) {
		std::unique_ptr<NodeDescription> stateDescription( ExporterTools::createDescriptionForState(ctxt, childState.get()) );
		if(stateDescription)
			ExporterTools::addChildEntry(desc,std::move(*stateDescription));
	}
}

static void describePolygonModeState(ExporterContext &,NodeDescription & desc,State * state) {
	auto n = dynamic_cast<PolygonModeState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_POLYGON_MODE);
	desc.setString(Consts::ATTR_POLYGON_MODE, Util::StringUtils::toString(static_cast<signed>(n->getParameters().getMode())));
}

static void describeLightingState(ExporterContext & ctxt,NodeDescription & desc,State * state) {
	auto ls = dynamic_cast<LightingState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_LIGHTING_STATE);
	LightNode * light = ls->getLight();
	if(light!=nullptr) {
		const std::string lightId=ctxt.sceneManager.getNameOfRegisteredNode(light);
		if(lightId.empty()) {
			WARN(std::string("LightingState references Light without id. "));
		} else {
			desc.setString(Consts::ATTR_LIGHTING_LIGHT_ID,lightId);
		}
	}
}

static void describeMaterialState(ExporterContext &,NodeDescription & desc,State * state) {
	auto n = dynamic_cast<MaterialState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_MATERIAL);
	desc.setString(Consts::ATTR_MATERIAL_AMBIENT, Util::StringUtils::implode(n->getParameters().getAmbient().data(), n->getParameters().getAmbient().data() + 4, " "));
	desc.setString(Consts::ATTR_MATERIAL_DIFFUSE, Util::StringUtils::implode(n->getParameters().getDiffuse().data(), n->getParameters().getDiffuse().data() + 4, " "));
	desc.setString(Consts::ATTR_MATERIAL_SPECULAR, Util::StringUtils::implode(n->getParameters().getSpecular().data(), n->getParameters().getSpecular().data() + 4, " "));
	desc.setString(Consts::ATTR_MATERIAL_SHININESS, Util::StringUtils::toString(n->getParameters().getShininess()));

}

static void describeShaderUniformState(ExporterContext &,NodeDescription & desc,State * state) {
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SHADER_UNIFORM);
	for(const auto & uniformEntry : dynamic_cast<ShaderUniformState *>(state)->getUniforms()) 
		ExporterTools::addDataEntry(desc,std::move(createDescriptionForUniform(uniformEntry.second)));
}

static void describeTransparencyRenderer(ExporterContext &,NodeDescription & desc,State * state) {
	auto n = dynamic_cast<TransparencyRenderer *>(state);
	desc.setString(Consts::TYPE, Consts::TYPE_STATE);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_TRANSPARENCY_RENDERER);
	desc.setString(Consts::ATTR_TRANSPARENY_USE_PREMULTIPLIED_ALPHA, n->getUsePremultipliedAlpha() ? "true" : "false");
}

void initCoreStateExporter() {
	ExporterTools::registerStateExporter(AlphaTestState::getClassId(),&describeAlphaTestState);
	ExporterTools::registerStateExporter(BlendingState::getClassId(),&describeBlendingState);
	ExporterTools::registerStateExporter(CullFaceState::getClassId(),&describeCullFaceState);
	ExporterTools::registerStateExporter(GroupState::getClassId(),&describeGroupState);
	ExporterTools::registerStateExporter(LightingState::getClassId(),&describeLightingState);
	ExporterTools::registerStateExporter(MaterialState::getClassId(),&describeMaterialState);
	ExporterTools::registerStateExporter(PolygonModeState::getClassId(),&describePolygonModeState);
	ExporterTools::registerStateExporter(ShaderState::getClassId(),&describeShaderState);
	ExporterTools::registerStateExporter(ShaderUniformState::getClassId(),&describeShaderUniformState);
	ExporterTools::registerStateExporter(TextureState::getClassId(),&describeTextureState);
	ExporterTools::registerStateExporter(TransparencyRenderer::getClassId(),&describeTransparencyRenderer);
}

}
}
