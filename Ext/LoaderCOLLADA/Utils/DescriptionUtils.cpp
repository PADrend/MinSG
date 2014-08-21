/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "DescriptionUtils.h"
#include "../../../SceneManagement/SceneDescription.h"

#include <Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wignored-qualifiers)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
#include <COLLADAFWNode.h>
#include <COLLADABUMathMatrix4.h>
COMPILER_WARN_POP 

#include <map>

namespace MinSG {
namespace LoaderCOLLADA {

std::vector<SceneManagement::DescriptionMap *> findDescriptions(const referenceRegistry_t & referenceRegistry, const Util::StringIdentifier & key, const std::string & name) {
	std::vector<SceneManagement::DescriptionMap *> result;
	for(const auto & item : referenceRegistry) {
		if(item.second->contains(key)) {
			if(item.second->getString(key) == name) {
				result.emplace_back(item.second);
			}
		}
	}
	return result;
}

void addToParentAsChildren(SceneManagement::DescriptionMap * parent, SceneManagement::DescriptionMap * child, const Util::StringIdentifier & childrenName) {
	Util::GenericAttribute::List * children = dynamic_cast<Util::GenericAttribute::List *>(parent->getValue(childrenName));
	if(children == nullptr) {
		children = new Util::GenericAttribute::List;
		parent->setValue(childrenName, children);
	}

	children->push_back(child);
}

void addToMinSGChildren(SceneManagement::DescriptionMap * parent, SceneManagement::DescriptionMap * child) {
	addToParentAsChildren(parent, child, SceneManagement::Consts::CHILDREN);
}

void copyAttributesToNodeDescription(SceneManagement::DescriptionMap * parent, SceneManagement::DescriptionMap * description, bool force) {
	if(parent == nullptr) {
		return;
	}

	for(auto & elem : * description) {
		if(!parent->contains(elem.first) || force) {
			parent->setValue(elem.first, elem.second.get()->clone());
		}
	}
}

void finalizeNodeDescription(const COLLADAFW::Node * object, SceneManagement::DescriptionMap * description) {
	if(!object->getName().empty()) {
		description->setString(SceneManagement::Consts::ATTR_NODE_ID, object->getName());
	} else if(!object->getOriginalId().empty()) {
		description->setString(SceneManagement::Consts::ATTR_NODE_ID, object->getOriginalId());
	}
}



void addTransformationDataIntoNodeDescription(SceneManagement::DescriptionMap & sourceDescription, const COLLADABU::Math::Matrix4 & colladaMatrix) {
	std::ostringstream matrixStream;
	for(uint32_t j = 0; j < 16; ++j) {
		matrixStream << colladaMatrix.getElement(j) << " ";
	}

	sourceDescription.setString(SceneManagement::Consts::ATTR_MATRIX, matrixStream.str());
}


void addDAEFlag(referenceRegistry_t & referenceRegistry, const Util::StringIdentifier & identifier, Util::GenericAttribute * attr) {
	COLLADAFW::UniqueId uId(Consts::DAE_FLAGS);
	SceneManagement::DescriptionMap * flagDesc;
	referenceRegistry_t::const_iterator descIt = referenceRegistry.find(uId);
	if(descIt == referenceRegistry.end()) {
		flagDesc = new SceneManagement::DescriptionMap();
		flagDesc->setString(SceneManagement::Consts::TYPE, Consts::DAE_FLAGS);
		referenceRegistry[uId] = flagDesc;
	} else {
		flagDesc = dynamic_cast<SceneManagement::DescriptionMap *>(descIt->second);
	}

	flagDesc->setValue(identifier, attr);
}
}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
