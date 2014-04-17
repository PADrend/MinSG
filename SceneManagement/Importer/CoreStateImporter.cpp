/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CoreStateImporter.h"

#include "../SceneManager.h"
#include "../SceneDescription.h"
#include "../ImportFunctions.h"
#include "ImporterTools.h"

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/ListNode.h"

#include "../../Core/States/ShaderState.h"
#include "../../Core/States/ShaderUniformState.h"
#include "../../Core/States/TextureState.h"
#include "../../Core/States/TransparencyRenderer.h"
#include "../../Core/States/LightingState.h"
#include "../../Core/States/PolygonModeState.h"
#include "../../Core/States/AlphaTestState.h"
#include "../../Core/States/CullFaceState.h"
#include "../../Core/States/GroupState.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/BlendingState.h"
#include "../../Helper/Helper.h"

#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Serialization/Serialization.h>

#include <Util/Encoding.h>
#include <Util/IO/FileUtils.h>
#include <Util/IO/FileName.h>
#include <Util/JSON_Parser.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <vector>

namespace MinSG {
namespace SceneManagement {

static Rendering::Uniform importUniform(const NodeDescription & d) {

	try {
		const std::string dataType = d.getString(Consts::ATTR_SHADER_UNIFORM_TYPE);
		const std::string name = d.getString(Consts::ATTR_SHADER_UNIFORM_NAME);
		const std::string values = d.getString(Consts::ATTR_SHADER_UNIFORM_VALUES);

		if(dataType == Consts::SHADER_UNIFORM_TYPE_BOOL)
			return Rendering::Uniform(name, Util::StringUtils::toBools(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC2B)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC2B,Util::StringUtils::toBools(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC3B)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC3B,Util::StringUtils::toBools(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC4B)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC4B,Util::StringUtils::toBools(values));

		if(dataType == Consts::SHADER_UNIFORM_TYPE_INT)
			return Rendering::Uniform(name, Util::StringUtils::toInts(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC2I)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC2I,Util::StringUtils::toInts(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC3I)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC3I,Util::StringUtils::toInts(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC4I)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC4I,Util::StringUtils::toInts(values));

		if(dataType == Consts::SHADER_UNIFORM_TYPE_FLOAT)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_FLOAT, Util::StringUtils::toFloats(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC2F)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC2F,Util::StringUtils::toFloats(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC3F)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC3F,Util::StringUtils::toFloats(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_VEC4F)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_VEC4F,Util::StringUtils::toFloats(values));

		if(dataType == Consts::SHADER_UNIFORM_TYPE_MATRIX_3X3F)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_MATRIX_3X3F,Util::StringUtils::toFloats(values));
		if(dataType == Consts::SHADER_UNIFORM_TYPE_MATRIX_4X4F)
			return Rendering::Uniform(name, Rendering::Uniform::UNIFORM_MATRIX_4X4F,Util::StringUtils::toFloats(values));

		WARN("Unknown uniform dataType");
	} catch(const std::invalid_argument & e) {
		WARN(e.what());
	}
	return Rendering::Uniform();
}

static bool importShaderState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SHADER || parent == nullptr)
		return false;

	Util::Reference<ShaderState> ss = new ShaderState;

	const NodeDescriptionList * children = dynamic_cast<const NodeDescriptionList *>(d.getValue(Consts::CHILDREN));

	ImporterTools::addAttributes(ctxt, children, ss.get());

	Rendering::Shader::flag_t usage = 0;
	std::vector<std::string> vsFiles;
	std::vector<std::string> gsFiles;
	std::vector<std::string> fsFiles;
	const std::string shaderName = d.getString(Consts::ATTR_SHADER_NAME);
	
	if(!shaderName.empty()){
		ss->setAttribute(Consts::STATE_ATTR_SHADER_NAME, Util::GenericAttribute::createString(shaderName)); // store name at state
			
		const auto location = ctxt.fileLocator.locateFile( Util::FileName(shaderName));
		if(location.first){
			try{
				std::unique_ptr<Util::GenericAttribute> sd( Util::JSON_Parser::parse( Util::FileUtils::getFileContents(location.second) ) );
				Util::GenericAttributeMap* shaderMap = dynamic_cast<Util::GenericAttributeMap*>(sd.get());
				if(!shaderMap)
					throw std::runtime_error("json map expected in "+location.second.toString());
				
				static const Util::StringIdentifier VS( Consts::DATA_TYPE_GLSL_VS );
				auto* vs = shaderMap->getValue<Util::GenericAttributeList>( VS );
				if(vs)
					for(auto& entry:*vs)
						vsFiles.emplace_back( entry ? entry->toString() : "?" );
					
				static const Util::StringIdentifier GS( Consts::DATA_TYPE_GLSL_GS );
				auto* gs = shaderMap->getValue<Util::GenericAttributeList>( GS );
				if(gs)
					for(auto& entry:*gs)
						gsFiles.emplace_back( entry ? entry->toString() : "?" );
					
				static const Util::StringIdentifier FS( Consts::DATA_TYPE_GLSL_FS );
				auto* fs = shaderMap->getValue<Util::GenericAttributeList>( FS );
				if(fs)
					for(auto& entry:*fs)
						fsFiles.emplace_back( entry ? entry->toString() : "?" );
				
				usage = shaderMap->getBool(Consts::ATTR_SHADER_USES_CLASSIC_GL) ? Rendering::Shader::USE_GL : 0 |
						shaderMap->getBool(Consts::ATTR_SHADER_USES_SG_UNIFORMS) ? Rendering::Shader::USE_UNIFORMS : 0;
				
			}catch(...){
				WARN("Error loading shader description '"+location.second.toString()+"'.");
				throw;
			}
		}else{
			WARN("Could not load shader description '"+shaderName+"'.");
		}
		
		
	}else{
		usage = (d.getString(Consts::ATTR_SHADER_USES_CLASSIC_GL)=="false" ? 0 : Rendering::Shader::USE_GL) |
				(d.getString(Consts::ATTR_SHADER_USES_SG_UNIFORMS)=="false" ? 0 : Rendering::Shader::USE_UNIFORMS);
	}

//const std::string dataType = 

	const auto data = ImporterTools::filterElements(Consts::TYPE_DATA, children);

	for(const auto & nd : data) {
		const std::string dataType = nd->getString(Consts::ATTR_DATA_TYPE);

		if(dataType == Consts::DATA_TYPE_GLSL_VS) {
			vsFiles.emplace_back(nd->getString(Consts::ATTR_SHADER_OBJ_FILENAME));
		} else if(dataType == Consts::DATA_TYPE_GLSL_GS) {
			gsFiles.emplace_back(nd->getString(Consts::ATTR_SHADER_OBJ_FILENAME));
		} else if(dataType == Consts::DATA_TYPE_GLSL_FS) {
			fsFiles.emplace_back(nd->getString(Consts::ATTR_SHADER_OBJ_FILENAME));
		} else if(dataType == Consts::DATA_TYPE_SHADER_UNIFORM) {
			Rendering::Uniform u = importUniform(*nd);
			if(!u.isNull())
				ss->setUniform(u);
		} else {
			WARN("Unknown shader data");
		}
	}

	initShaderState(ss.get(), vsFiles,gsFiles,fsFiles, usage,ctxt.fileLocator);

	ImporterTools::finalizeState(ctxt,ss.get(),d);
	parent->addState(ss.get());
	return true;
}

static bool importShaderUniformState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SHADER_UNIFORM || parent == nullptr)
		return false;

	auto sus = new ShaderUniformState();

	const NodeDescriptionList * children = dynamic_cast<const NodeDescriptionList *>(d.getValue(Consts::CHILDREN));

	ImporterTools::addAttributes(ctxt, children, sus);

	const auto data = ImporterTools::filterElements(Consts::TYPE_DATA, children);

	for(const auto & nd : data) {
		const std::string dataType = nd->getString(Consts::ATTR_DATA_TYPE);

		if(dataType == Consts::DATA_TYPE_SHADER_UNIFORM) {
			Rendering::Uniform u = importUniform(*nd);
			if(!u.isNull())
				sus->setUniform(u);
		} else {
			WARN("Unknown shader uniform data");
		}
	}
	ImporterTools::finalizeState(ctxt, sus, d);
	parent->addState(sus);
	return true;
}

static bool reuseState(ImportContext & ctxt, const std::string & /*stateType*/, const NodeDescription & d, Node * parent) {
	if(parent==nullptr || d.getString(Consts::TYPE) != Consts::TYPE_STATE || (ctxt.importOptions & IMPORT_OPTION_REUSE_EXISTING_STATES) == 0)
		return false;

	// special case if old states should be reused
	std::string id = d.getString(Consts::ATTR_STATE_ID);
	State * s = ctxt.sceneManager.getRegisteredState(id);
	if(s != nullptr) {
		ImporterTools::finalizeState(ctxt, s, d);
		parent->addState(s);
		std::cout <<"State reused:"<<id<<"\n"; // \todo this line should soon be removed when it is clear that the "reuse existing state" functionality works
		return true;
	}
	return false;
}

static bool importTextureState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_TEXTURE || parent == nullptr)
		return false;

	const auto dataDescList = ImporterTools::filterElements(Consts::TYPE_DATA,
															dynamic_cast<const NodeDescriptionList *>(d.getValue(Consts::CHILDREN)));

	if (dataDescList.empty()) {// No data block given -> TextureState intentionally without Texture
		auto ts = new TextureState;
		ImporterTools::finalizeState(ctxt, ts, d);
		parent->addState(ts);
		return true;
	}

	if(dataDescList.size() != 1) {
		WARN(std::string("TextureState needs one data description. Got ")
			 + Util::StringUtils::toString(dataDescList.size()));
		return false;
	}

	// filename
	const NodeDescription * dataDesc = dataDescList.front();

	// type
	const std::string dataType = dataDesc->getString(Consts::ATTR_DATA_TYPE);
	if(!Util::StringUtils::beginsWith(dataType.c_str(),"image")) {
		WARN(std::string("createTextureState: Unknown data type '")+dataType+"' (still trying to load the file though...)");
	}

	// textureUnit
	const int textureUnit = Util::StringUtils::toNumber<int>(d.getString(Consts::ATTR_TEXTURE_UNIT, "0"));

	TextureState * ts = nullptr;
	const Util::FileName fileName(dataDesc->getString(Consts::ATTR_TEXTURE_FILENAME));
	if(fileName.empty()) {
		// Load image data from a Base64 encoded block.
		if(dataDesc->getString(Consts::ATTR_DATA_ENCODING) != Consts::DATA_ENCODING_BASE64) {
			WARN("Unknown data block encoding.");
			return nullptr;
		}
		const std::vector<uint8_t> rawData = Util::decodeBase64(dataDesc->getString(Consts::DATA_BLOCK));
		Rendering::Texture * t = Rendering::Serialization::loadTexture(
										dataDesc->getString(Consts::ATTR_DATA_FORMAT,"png"), std::string(rawData.begin(), rawData.end()),true,false);
		ts = new TextureState(t);
	} else {
		const auto location = ctxt.fileLocator.locateFile( fileName );
		ts = createTextureState(location.first ? location.second : fileName,
						 true,
						 false,
						 textureUnit,
						 (ctxt.importOptions & IMPORT_OPTION_USE_TEXTURE_REGISTRY) > 0 ? &ctxt.getTextureRegistry() : nullptr);
						 
		// set original filename
		if(ts&&ts->getTexture())
			ts->getTexture()->setFileName(fileName);
	}
	ImporterTools::finalizeState(ctxt, ts, d);
	parent->addState(ts);
	return true;
}

static bool importReference(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_REFERENCE || parent == nullptr)
		return false;

	std::string refId = d.getString(Consts::ATTR_REFERENCED_STATE_ID);
	State * state = ctxt.sceneManager.getRegisteredState(refId);

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importTransparencyRenderer(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_TRANSPARENCY_RENDERER || parent == nullptr)
		return false;

	auto state = new TransparencyRenderer;
	state->setUsePremultipliedAlpha(d.getString(Consts::ATTR_TRANSPARENY_USE_PREMULTIPLIED_ALPHA,"true") == "true");

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importLightingState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_LIGHTING_STATE || parent == nullptr)
		return false;

	const auto lightId = d.getString(Consts::ATTR_LIGHTING_LIGHT_ID);
	auto state = new LightingState;
	// Hand the local variables by copy to the lambda
	ctxt.addFinalizingAction([state, lightId](ImportContext & importerContext) {
		std::cout << "Info: Assigning LightningState to Light: " << lightId << " ... ";
		LightNode * ln = dynamic_cast<LightNode *>(importerContext.sceneManager.getRegisteredNode(lightId));
		if(ln != nullptr) {
			state->setLight(ln);
			std::cout << "ok.\n";
		} else {
			std::cout << " not found!\n";
		}
	});

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importPolygonModeState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_POLYGON_MODE || parent == nullptr)
		return false;

	auto state = new PolygonModeState();
	if(d.contains(Consts::ATTR_POLYGON_MODE_OLD)) {
		WARN("StdImporter: createPolygonMode: your scene file is out of date, please save a new copy.");
		state->changeParameters().setMode(Rendering::PolygonModeParameters::glToMode(Util::StringUtils::toNumber<uint32_t>(d.getString(Consts::ATTR_POLYGON_MODE_OLD))));
	} else {
		state->changeParameters().setMode(static_cast<Rendering::PolygonModeParameters::polygonModeMode_t>(d.getUInt(Consts::ATTR_POLYGON_MODE, state->getParameters().getMode())));
	}

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importGroupState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_GROUP || parent == nullptr)
		return false;

	Util::Reference<GroupState> state = new GroupState();

	// add the states to a dummy node, as states can not be created explicitly from a description.
	Util::Reference<Node> dummyStateContainer = new ListNode();
	ImporterTools::finalizeNode(ctxt, dummyStateContainer.get(),d);

	const Node::stateList_t * stateList = dummyStateContainer->getStateListPtr();

	if(stateList!=nullptr) {
		for(const auto & entry : *stateList) {
			state->addState(entry.first.get());
		}
	}

	ImporterTools::finalizeState(ctxt, state.get(), d);
	parent->addState(state.get());
	return true;
}

static bool importAlphaTestState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_ALPHA_TEST || parent == nullptr)
		return false;

	auto state = new AlphaTestState();

	if(d.contains(Consts::ATTR_ALPHA_TEST_MODE_OLD)) {
		WARN("StdImporter: createAlphaTest: your scene file is out of date, please save a new copy.");
		state->changeParameters().setReferenceValue(d.getFloat(Consts::ATTR_ALPHA_REF_VALUE_OLD));
		state->changeParameters().setMode(Rendering::Comparison::glToFunction(Util::StringUtils::toNumber<uint32_t>(d.getString(Consts::ATTR_ALPHA_TEST_MODE_OLD))));
	} else {
		state->changeParameters().setReferenceValue(d.getFloat(Consts::ATTR_ALPHA_REF_VALUE, state->getParameters().getReferenceValue()));
		state->changeParameters().setMode(static_cast<Rendering::Comparison::function_t>(d.getUInt(Consts::ATTR_ALPHA_TEST_MODE, state->getParameters().getMode())));
	}

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importCullFaceState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_CULL_FACE || parent == nullptr)
		return false;

	auto state = new CullFaceState();
	const std::string mode = d.getString(Consts::ATTR_CULL_FACE);
	if(mode == "front" || mode == "FRONT") {
		state->changeParameters().setMode(Rendering::CullFaceParameters::CULL_FRONT);
		state->changeParameters().enable();
	} else if(mode == "back" || mode == "BACK") {
		state->changeParameters().setMode(Rendering::CullFaceParameters::CULL_BACK);
		state->changeParameters().enable();
	} else if(mode == "FRONT_AND_BACK") {
		state->changeParameters().setMode(Rendering::CullFaceParameters::CULL_FRONT_AND_BACK);
		state->changeParameters().enable();
	} else if(mode == "DISABLED" || mode == "none" || mode == "0") { // The "0" is for compatibility with old scenes
		state->changeParameters().disable();
	} else {
		WARN("createCullFace: Invalid mode.");
		return nullptr;
	}

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importMaterialState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_MATERIAL || parent == nullptr)
		return false;

	auto state = new MaterialState();
	std::string strAmbient = d.getString(Consts::ATTR_MATERIAL_AMBIENT);
	if(!strAmbient.empty()) {
		std::vector<float> vecAmbient = Util::StringUtils::toFloats(strAmbient);
		FAIL_IF(vecAmbient.size() != 4);
		state->changeParameters().setAmbient(Util::Color4f(vecAmbient[0], vecAmbient[1], vecAmbient[2], vecAmbient[3]));
	}
	std::string strDiffuse = d.getString(Consts::ATTR_MATERIAL_DIFFUSE);
	if(!strDiffuse.empty()) {
		std::vector<float> vecDiffuse = Util::StringUtils::toFloats(strDiffuse);
		FAIL_IF(vecDiffuse.size() != 4);
		state->changeParameters().setDiffuse(Util::Color4f(vecDiffuse[0], vecDiffuse[1], vecDiffuse[2], vecDiffuse[3]));
	}
	std::string strSpecular = d.getString(Consts::ATTR_MATERIAL_SPECULAR);
	if(!strSpecular.empty()) {
		std::vector<float> vecSpecular = Util::StringUtils::toFloats(strSpecular);
		FAIL_IF(vecSpecular.size() != 4);
		state->changeParameters().setSpecular(Util::Color4f(vecSpecular[0], vecSpecular[1], vecSpecular[2], vecSpecular[3]));
	}
	std::string strShininess = d.getString(Consts::ATTR_MATERIAL_SHININESS);
	if(!strShininess.empty()) {
		float shininess = Util::StringUtils::toNumber<float>(strShininess);
		state->changeParameters().setShininess(shininess);
	}


	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importBlendingState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_BLENDING || parent == nullptr)
		return false;

	auto state = new BlendingState();

	std::string s = d.getString(Consts::ATTR_BLEND_EQUATION);
	if(!s.empty()) {
		if(isdigit(s[0])) {
			WARN("StdImporter: createBlending: Your scene file is out of date. Please save a new copy.");
			state->changeParameters().setBlendEquation(Rendering::BlendingParameters::glToEquation(Util::StringUtils::toNumber<uint32_t>(s)));
		} else {
			state->changeParameters().setBlendEquation(Rendering::BlendingParameters::stringToEquation(s));
		}
	}
	s = d.getString(Consts::ATTR_BLEND_FUNC_SRC);
	if(!s.empty()) {
		if(isdigit(s[0])) {
			WARN("StdImporter: createBlending: Your scene file is out of date. Please save a new copy.");
			state->changeParameters().setBlendFuncSrcAlpha(Rendering::BlendingParameters::glToFunction(Util::StringUtils::toNumber<uint32_t>(s)));
			state->changeParameters().setBlendFuncSrcRGB(Rendering::BlendingParameters::glToFunction(Util::StringUtils::toNumber<uint32_t>(s)));
		} else {
			state->changeParameters().setBlendFuncSrcAlpha(Rendering::BlendingParameters::stringToFunction(s));
			state->changeParameters().setBlendFuncSrcRGB(Rendering::BlendingParameters::stringToFunction(s));
		}
	}
	s = d.getString(Consts::ATTR_BLEND_FUNC_DST);
	if(!s.empty()) {
		if(isdigit(s[0])) {
			WARN("StdImporter: createBlending: Your scene file is out of date. Please save a new copy.");
			state->changeParameters().setBlendFuncDstAlpha(Rendering::BlendingParameters::glToFunction(Util::StringUtils::toNumber<uint32_t>(s)));
			state->changeParameters().setBlendFuncDstRGB(Rendering::BlendingParameters::glToFunction(Util::StringUtils::toNumber<uint32_t>(s)));
		} else {
			state->changeParameters().setBlendFuncDstAlpha(Rendering::BlendingParameters::stringToFunction(s));
			state->changeParameters().setBlendFuncDstRGB(Rendering::BlendingParameters::stringToFunction(s));
		}
	}
	s = d.getString(Consts::ATTR_BLEND_CONST_ALPHA);
	if(!s.empty()) {
		state->changeParameters().setBlendColor(Util::Color4f(0.0f, 0.0f, 0.0f, Util::StringUtils::toNumber<float>(s)));
	}
	s = d.getString(Consts::ATTR_BLEND_DEPTH_MASK);
	if(!s.empty()) {
		state->setBlendDepthMask(Util::StringUtils::toBool(s));
	}

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

//! template for new importers
// static bool importXY(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
//  if(stateType != Consts::STATE_TYPE_XY) // check parent != nullptr is done by SceneManager
//      return false;
//
//  XY * state = new XY;
//
//  //TODO
//
//
//  ImporterTools::finalizeState(ctxt, state, d);
//  parent->addState(state);
//  return true;
// }

void initCoreStateImporter() {

	{
		//! have to be called in this order as first importers
		ImporterTools::registerStateImporter(&reuseState);
		ImporterTools::registerStateImporter(&importReference);
	}

	ImporterTools::registerStateImporter(&importShaderState);
	ImporterTools::registerStateImporter(&importShaderUniformState);
	ImporterTools::registerStateImporter(&importTextureState);
	ImporterTools::registerStateImporter(&importTransparencyRenderer);
	ImporterTools::registerStateImporter(&importLightingState);
	ImporterTools::registerStateImporter(&importPolygonModeState);
	ImporterTools::registerStateImporter(&importGroupState);
	ImporterTools::registerStateImporter(&importAlphaTestState);
	ImporterTools::registerStateImporter(&importCullFaceState);
	ImporterTools::registerStateImporter(&importMaterialState);
	ImporterTools::registerStateImporter(&importBlendingState);
}

}
}
