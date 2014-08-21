/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "WriterMinSG.h"
#include "../SceneDescription.h"
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Macros.h>
#include <algorithm>
#include <memory>
#include <string>

namespace MinSG{
namespace SceneManagement{

using namespace Util;

//! (internal)
static void convertDescriptionToXML(std::ostream & out,const DescriptionMap & d,uint32_t indention/*=0*/){
	const std::string type = d.getString(Consts::TYPE);

	for(uint32_t i=0;i<indention;++i)
		out << "\t";
	out << "<"<<type;

	// insert attributes
	std::vector<std::pair<std::string,std::string>> attributes; // used for sorting by the attributes' key
	for(const auto & mapEntry : d) {
		const std::string key=mapEntry.first.toString();
		if(key.empty() || (key.at(0)=='_' && key.at(key.size()-1)=='_')) // skip description meta keys like  _DATA_, _CHILDREN_, ...
				continue;
		attributes.emplace_back(key,mapEntry.second->toString());
	}
	std::sort(attributes.begin(),attributes.end());
	for(const auto & a : attributes)
		out << " "<<a.first<<"=\""<<StringUtils::replaceAll(a.second,  "\"","&quot;")<<"\"";
	
	
	const DescriptionArray * children = dynamic_cast<DescriptionArray *>(d.getValue(Consts::CHILDREN));
	const GenericAttribute * dataBlock = d.getValue(Consts::DATA_BLOCK);
	if( dataBlock || (children && !children->empty()) ) {
		out<<">\n";
		if(children) {
			for(const auto & child : *children) {
				DescriptionMap * m=dynamic_cast<DescriptionMap *>(child.get());
				if(m)
					convertDescriptionToXML(out,*m,indention+1);
			}
		}
		if(dataBlock){
			for(uint32_t i=0;i<indention;++i) 
				out << "\t";
			out<<dataBlock->toString()<<"\n";
		}
		for(uint32_t i=0;i<indention;++i) 
			out << "\t";
		out << "</"<<type<<">\n";
	}else{
		out << "/>\n";
	}
}

//! (static)
bool WriterMinSG::save(std::ostream & out, const DescriptionMap & sceneDescription) {
	if(!out.good()) {
		WARN("Invalid stream.");
		return false;
	}
	out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
	convertDescriptionToXML(out, sceneDescription, 0);
	return true;
}

}
}
