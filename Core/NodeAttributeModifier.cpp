/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "NodeAttributeModifier.h"
#include <sstream>
#include <iostream>

namespace MinSG{

namespace NodeAttributeModifier{

Util::StringIdentifier create( const std::string& mainKey, uint32_t flags ){
	if( flags==DEFAULT_ATTRIBUTE ){ // default
		return Util::StringIdentifier(mainKey);
	}else{
		std::ostringstream s;
		s<<"$";
		if( (flags&COPY_TO_CLONES) == 0)
			s<<"c";
		if( (flags&COPY_TO_INSTANCES) != 0)
			s<<"I";
		if( (flags&SAVE_TO_FILE) == 0)
			s<<"s";
		s<<"$"<<mainKey;
		return Util::StringIdentifier(s.str());
	}
}
uint32_t getFlags( const std::string& str){
	if(!str.empty()&&str[0]=='$') {
		uint32_t flags = DEFAULT_ATTRIBUTE;
		for(size_t i=1;i<str.length();++i){
			switch(str[i]){
			case 'c':
				flags &= ~COPY_TO_CLONES;
				break;
			case 'C':
				flags |= COPY_TO_CLONES;
				break;
			case 'i':
				flags &= ~COPY_TO_INSTANCES;
				break;
			case 'I':
				flags |= COPY_TO_INSTANCES;
				break;
			case 's':
				flags &= ~SAVE_TO_FILE;
				break;
			case 'S':
				flags |= SAVE_TO_FILE;
				break;
			case '$':
				return flags;
			default:
				std::cerr << "Unknown NodeAttributeModifier: '"<<str[i]<<"'";
			}
		}
	}
	return DEFAULT_ATTRIBUTE;
}


}

}
