/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SHADERFACTORY_H
#define SHADERFACTORY_H

#include <map>
#include <vector>
#include <string>

namespace Util{ 
	class FileName;
	class StringIdentifier;
}

namespace MinSG{
	
class ShaderState;
	
namespace SceneManagement{

class ShaderFactory{
	
public:
	
	struct FeatureDescription{
		explicit FeatureDescription(const std::string featureName);
		const std::string featureId;
		const std::string classId;
		std::vector<Util::FileName> 	vs_file, gs_file, fs_file;
		std::vector<std::string> 		vs_code, gs_code, fs_code;
		std::vector<std::string> 		dependencies;
	};
	
	typedef std::map<std::string, FeatureDescription> featureMap_t;
	
	static const Util::StringIdentifier ATTR_SHADER_FEATURES;
	
    ShaderFactory();
	
    ~ShaderFactory();
	
	ShaderState * createShaderState(const std::vector<std::string> featureNames) const;
	
	void setFeature(const FeatureDescription & fd);
	
	bool unsetFeature(const std::string featureName);
	
private:
	
	featureMap_t featureMap;
	
	void collectDependencies(const std::string & featureName, featureMap_t & features) const;
	
};

} // namespace SceneManagement

} // namespace MinSG

#endif // SHADERFACTORY_H
