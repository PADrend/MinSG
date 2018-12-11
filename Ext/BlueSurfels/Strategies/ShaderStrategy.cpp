/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "ShaderStrategy.h"

#include <MinSG/Core/FrameContext.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Ext/BlueSurfels/SurfelAnalysis.h>

#include <Util/GenericAttribute.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/StringUtils.h>

#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/ShaderObjectInfo.h>

#include <limits>
#include <algorithm>

namespace MinSG {
namespace BlueSurfels {
	
ShaderStrategy::ShaderStrategy() : AbstractSurfelStrategy(-10000) {}
	
bool ShaderStrategy::prepare(MinSG::FrameContext& context, MinSG::Node* node) {
	if(needsRefresh) 
		refreshShader();
	return false;
}

bool ShaderStrategy::beforeRendering(MinSG::FrameContext& context) {
	if(shader.isNotNull() && shader->getStatus() != Rendering::Shader::INVALID) {
		context.getRenderingContext().pushAndSetShader(shader.get());
		wasActive = true;
	}
	return false;
}

void ShaderStrategy::afterRendering(MinSG::FrameContext& context) {
	if(wasActive) {
		context.getRenderingContext().popShader();
		wasActive = false;
	}
}

void ShaderStrategy::refreshShader() {
	needsRefresh = false;	
	shader = nullptr;
	std::cout << "ShaderStrategy: refresh shader." << std::endl;
	std::string culling = surfelCulling ? "1" : "0";
	
	if(!getShaderVS().empty()) {
		auto file = getFileLocator().locateFile(Util::FileName(getShaderVS()));
		if(file.first) {
			if(shader.isNull()) shader = Rendering::Shader::createShader(Rendering::Shader::USE_UNIFORMS);
			auto so = Rendering::ShaderObjectInfo::loadVertex(file.second).addDefine("SURFEL_CULLING", culling);
			shader->attachShaderObject(std::move(so));
		} else {
			WARN("ShaderStrategy: could not find vertex shader '" + getShaderVS() + "'");
			shader = nullptr;
			return;
		}
	}
	
	if(!getShaderFS().empty()) {
		auto file = getFileLocator().locateFile(Util::FileName(getShaderFS()));
		if(file.first) {
			if(shader.isNull()) shader = Rendering::Shader::createShader(Rendering::Shader::USE_UNIFORMS);
			auto so = Rendering::ShaderObjectInfo::loadFragment(file.second).addDefine("SURFEL_CULLING", culling);
			shader->attachShaderObject(std::move(so));
		} else {
			WARN("ShaderStrategy: could not find fragment shader '" + getShaderFS() + "'");
			shader = nullptr;
			return;
		}
	}
	
	if(!getShaderGS().empty()) {
		auto file = getFileLocator().locateFile(Util::FileName(getShaderGS()));
		if(file.first) {
			if(shader.isNull()) shader = Rendering::Shader::createShader(Rendering::Shader::USE_UNIFORMS);
			auto so = Rendering::ShaderObjectInfo::loadGeometry(file.second).addDefine("SURFEL_CULLING", culling);
			shader->attachShaderObject(std::move(so));
		} else {
			WARN("ShaderStrategy: could not find geometry shader '" + getShaderGS() + "'");
			shader = nullptr;
			return;
		}
	}
	
}

// ------------------------------------------------------------------------
// Import/Export

static const Util::StringIdentifier ATTR_SHADER_VS("shader_vs");
static const Util::StringIdentifier ATTR_SHADER_FS("shader_fs");
static const Util::StringIdentifier ATTR_SHADER_GS("shader_gs");
static const Util::StringIdentifier ATTR_SEARCH_PATHS("searchPaths");

static void exportShaderStrategy(Util::GenericAttributeMap& desc, AbstractSurfelStrategy* _strategy) {
  const auto& strategy = dynamic_cast<ShaderStrategy*>(_strategy);
  desc.setString(ATTR_SHADER_VS, strategy->getShaderVS());
  desc.setString(ATTR_SHADER_FS, strategy->getShaderFS());
  desc.setString(ATTR_SHADER_GS, strategy->getShaderGS());
	auto searchPaths = strategy->getFileLocator().getSearchPaths();
  desc.setString(ATTR_SEARCH_PATHS, Util::StringUtils::implode(searchPaths.begin(), searchPaths.end(), ";"));	
}

static AbstractSurfelStrategy* importShaderStrategy(const Util::GenericAttributeMap* desc) {
  auto strategy = new ShaderStrategy;
  strategy->setShaderVS(desc->getString(ATTR_SHADER_VS, ""));
  strategy->setShaderFS(desc->getString(ATTR_SHADER_FS, ""));
  strategy->setShaderGS(desc->getString(ATTR_SHADER_GS, ""));	
	
	std::stringstream ss(desc->getString(ATTR_SEARCH_PATHS, ""));
	std::string searchPath;
	while(std::getline(ss, searchPath, ';'))
		strategy->getFileLocator().addSearchPath(searchPath);
	
  return strategy;
}

static bool importerRegistered = registerImporter(ShaderStrategy::getClassId(), &importShaderStrategy);
static bool exporterRegistered = registerExporter(ShaderStrategy::getClassId(), &exportShaderStrategy);

} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS