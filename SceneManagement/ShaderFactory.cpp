/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderFactory.h"

#include "../Core/States/ShaderState.h"

#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/ShaderObjectInfo.h>

#include <Util/IO/FileName.h>
#include <Util/StringIdentifier.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>

namespace MinSG{
	
namespace SceneManagement{
	
const Util::StringIdentifier ShaderFactory::ATTR_SHADER_FEATURES("shader_features");

ShaderFactory::FeatureDescription::FeatureDescription(const std::string featureName) : featureId(featureName), classId(featureName.substr(0,featureName.find('.'))) {
	size_t pos = featureName.find('.');
	if( pos == 0 || pos >= featureName.length()-1 || featureName.rfind('.' != pos))
		WARN(std::string("feature name has to be of the following form: \"class.feature\" but is ").append(featureName));
}

ShaderFactory::ShaderFactory(){}

ShaderFactory::~ShaderFactory(){}

void ShaderFactory::collectDependencies(const std::string & featureName, featureMap_t & features) const {
	const auto fdi = featureMap.find(featureName);
	if(fdi == featureMap.end()){
		WARN(std::string("shader feature not available: ").append(featureName));
		return;
	}
	for(const std::string & fn : fdi->second.dependencies)
		collectDependencies(fn, features);
	features.erase(fdi->second.classId);
	features.insert(std::make_pair(fdi->second.classId, fdi->second));
}

ShaderState * ShaderFactory::createShaderState(const std::vector<std::string> featureNames) const{
	
	featureMap_t features;
	for(const std::string & fn : featureNames)
		collectDependencies(fn, features);
	
	Util::Reference<Rendering::Shader> shader = Rendering::Shader::createShader(Rendering::Shader::USE_UNIFORMS); 
	
	for(const auto & fdi : features){
		const FeatureDescription & fd = fdi.second;
		for(const auto & code : fd.vs_code)
			shader->attachShaderObject(Rendering::ShaderObjectInfo::createVertex(code));
		for(const auto & code : fd.gs_code)
			shader->attachShaderObject(Rendering::ShaderObjectInfo::createGeometry(code));
		for(const auto & code : fd.fs_code)
			shader->attachShaderObject(Rendering::ShaderObjectInfo::createFragment(code));
		for(const auto & file : fd.vs_file)
			shader->attachShaderObject(Rendering::ShaderObjectInfo::loadVertex(file));
		for(const auto & file : fd.gs_file)
			shader->attachShaderObject(Rendering::ShaderObjectInfo::loadGeometry(file));
		for(const auto & file : fd.fs_file)
			shader->attachShaderObject(Rendering::ShaderObjectInfo::loadFragment(file));
	}
	
	Util::Reference<ShaderState> shaderState = new ShaderState(shader.get());
	
	auto gaList = new Util::GenericAttributeList();
	for(const std::string & s : featureNames)
		gaList->push_back(Util::GenericAttribute::createString(s));
	shaderState->setAttribute(ATTR_SHADER_FEATURES,gaList);
	
	return shaderState.detachAndDecrease();
}

void ShaderFactory::setFeature(const FeatureDescription & fd){
	if(unsetFeature(fd.featureId)){
		WARN(std::string("re-set existing shader feature: ").append(fd.featureId));
	}
	featureMap.insert(std::make_pair(fd.featureId, fd));
}

bool ShaderFactory::unsetFeature(const std::string featureName){
	return featureMap.erase(featureName) > 0;
}

} // namespace SceneManagement

} // namespace MinSG
