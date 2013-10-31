/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExporterTools.h"

#include "../MeshImportHandler.h"
#include "../SceneDescription.h"
#include "../SceneManager.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/States/State.h"
#include "../../Core/Behaviours/AbstractBehaviour.h"
#include "../../Core/Behaviours/BehaviourManager.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Matrix4x4.h>
#include <Geometry/SRT.h>
#include <Geometry/Vec3.h>
#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>
#include <Util/References.h>
#include <Util/StringIdentifier.h>
#include <list>
#include <ostream>
#include <utility>

using namespace Geometry;
using namespace Util;
namespace MinSG {
namespace SceneManagement {

//! (static)
void ExporterTools::finalizeBehaviourDescription(ExporterContext & /*ctxt*/,NodeDescription & description, AbstractBehaviour * behaviour){
	if(behaviour==nullptr)
		return;
	description.setString(Consts::TYPE,Consts::TYPE_BEHAVIOUR);
}

static void addMatrixToDescription(NodeDescription & description, const Geometry::Matrix4x4f & matrix) {
	if(!matrix.isIdentity()) {
		std::ostringstream matrixStream;
		matrixStream << matrix;
		description.setValue(Consts::ATTR_MATRIX, Util::GenericAttribute::createString(matrixStream.str()));
	}
}

//! (static)
void ExporterTools::addSRTToDescription(NodeDescription & description, const Geometry::SRT & srt) {
	Vec3 posV=srt.getTranslation();
	if (posV!=Vec3(0,0,0)) {
		std::stringstream pos;
		pos<<posV.getX()<<" "<<posV.getY()<<" "<<posV.getZ();
		description.setValue(Consts::ATTR_SRT_POS,GenericAttribute::createString(pos.str()));
	}
	Vec3 dirV=srt.getDirVector();
	Vec3 upV=srt.getUpVector();
	if (dirV!=Vec3(0,0,1) || upV!=Vec3(0,1,0)) {
		std::stringstream dir;
		dir<<dirV.getX()<<" "<<dirV.getY()<<" "<<dirV.getZ();
		description.setValue(Consts::ATTR_SRT_DIR,GenericAttribute::createString(dir.str()));

		std::stringstream up;
		up<<upV.getX()<<" "<<upV.getY()<<" "<<upV.getZ();
		description.setValue(Consts::ATTR_SRT_UP,GenericAttribute::createString(up.str()));
	}
	if (srt.getScale() != 1.0) {
		std::stringstream s;
		s<<srt.getScale();
		description.setValue(Consts::ATTR_SRT_SCALE,GenericAttribute::createString(s.str()));
	}
}

void ExporterTools::addTransformationToDescription(NodeDescription & description, Node * node) {
	if(node->hasSRT()) {
		addSRTToDescription(description, node->getSRT());
	} else if(node->hasMatrix()) {
		if(node->getMatrixPtr()->convertsSafelyToSRT()) {
			addSRTToDescription(description, node->getMatrixPtr()->_toSRT());
		} else {
			addMatrixToDescription(description, *node->getMatrixPtr());
		}
	}
}

//! (static)
void ExporterTools::addAttributesToDescription(ExporterContext & ctxt, NodeDescription & description, const Util::GenericAttribute::Map * attribs) {
	if(attribs == nullptr) {
		return;
	}
	for(auto & attrib : *attribs) {
		const std::string attributeName = attrib.first.toString();
		// Ignore attributes that should not be saved
		if(!NodeAttributeModifier::isSaved(attributeName))
			continue;

		const Util::GenericAttribute * attribute = attrib.second.get();

		NodeDescription attributeDescription;
		attributeDescription.setString(Consts::TYPE, Consts::TYPE_ATTRIBUTE);
		attributeDescription.setString(Consts::ATTR_ATTRIBUTE_NAME, attributeName);

		if(dynamic_cast<const Util::GenericNumberAttribute *>(attribute) != nullptr) {
			if(dynamic_cast<const Util::_NumberAttribute<float> *>(attribute) != nullptr) {
				attributeDescription.setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_FLOAT);
			} else if(dynamic_cast<const Util::_NumberAttribute<int> *>(attribute) != nullptr) {
				attributeDescription.setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_INT);
			} else { // unspecific number
				attributeDescription.setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_NUMBER);
			}
			attributeDescription.setString(Consts::ATTR_ATTRIBUTE_VALUE, attribute->toString());
		} else if(dynamic_cast<const Util::BoolAttribute *>(attribute) != nullptr) {
			attributeDescription.setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_BOOL);
			attributeDescription.setString(Consts::ATTR_ATTRIBUTE_VALUE, attribute->toString());
		} else if(dynamic_cast<const Util::StringAttribute *>(attribute) != nullptr) {
			attributeDescription.setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_STRING);
			attributeDescription.setString(Consts::ATTR_ATTRIBUTE_VALUE, attribute->toString());
		} else {
			attributeDescription.setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_GENERIC);
			static const Util::StringIdentifier CONTEXT_DATA_SCENEMANAGER("SceneManager");
			std::unique_ptr<Util::GenericAttributeMap> context(new Util::GenericAttributeMap);
			context->setValue(CONTEXT_DATA_SCENEMANAGER, new Util::WrapperAttribute<SceneManager &>(ctxt.sceneManager));
			attributeDescription.setString(Consts::ATTR_ATTRIBUTE_VALUE, 
											   Util::GenericAttributeSerialization::serialize(attribute, context.get()));
		}
		addChildEntry(description,std::move(attributeDescription));
	}
}

//! (static)
void ExporterTools::addChildEntry(	NodeDescription & description, NodeDescription && childDescription) {
	NodeDescriptionList * children = dynamic_cast<NodeDescriptionList *> (description.getValue(Consts::CHILDREN));
	if (children == nullptr) {
		children = new NodeDescriptionList;
		description.setValue(Consts::CHILDREN, children);
	}
	children->push_back(new NodeDescription(std::move(childDescription)));
}

//! (static)
void ExporterTools::addDataEntry(	NodeDescription & description,NodeDescription && dataDescription) {
	dataDescription.setString(Consts::TYPE, Consts::TYPE_DATA);
	addChildEntry(description, std::move(dataDescription));
}

//! (static)
void ExporterTools::addChildNodesToDescription(ExporterContext & ctxt,NodeDescription & description, Node * node){
	for(const auto & child : getChildNodes(node)){
		std::unique_ptr<NodeDescription> childDescription(ctxt.sceneManager.createDescriptionForNode(ctxt, child));
		if(childDescription)
			addChildEntry(description,std::move(*childDescription));
	}
}

//! (static)
void ExporterTools::addStatesToDescription(ExporterContext & ctxt,NodeDescription & description,Node * node){
	if( node->hasStates() ){
		for(const auto & state : node->getStates()){
			std::unique_ptr<NodeDescription> stateDescription(ctxt.sceneManager.createDescriptionForState(ctxt, state));
			if(stateDescription)
				ExporterTools::addChildEntry(description,std::move(*stateDescription));
		}
	}
}

//! (static)
void ExporterTools::addBehavioursToDescription(ExporterContext & ctxt,NodeDescription & description, Node * node){
	BehaviourManager::nodeBehaviourList_t behaviours=ctxt.sceneManager.getBehaviourManager()->getBehavioursByNode(node);
	auto * children = dynamic_cast<NodeDescriptionList *>(description.getValue(Consts::CHILDREN));
	if (!children) {
		children = new NodeDescriptionList;
		description.setValue(Consts::CHILDREN,children);
	}
	for(BehaviourManager::nodeBehaviourList_t::const_iterator it=behaviours.begin();it!=behaviours.end();++it){
		NodeDescription * behaviourDescription=ctxt.sceneManager.createDescriptionForBehaviour(ctxt,it->get());
		if (!behaviourDescription) continue;
		behaviourDescription->setString(Consts::TYPE,Consts::TYPE_BEHAVIOUR);
		children->push_back(behaviourDescription);
	}
}

}
}
