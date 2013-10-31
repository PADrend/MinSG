/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "VisualSceneImporter.h"
#include "../Utils/NodeImporterUtils.h"
#include "../Utils/DescriptionUtils.h"
#include "../Utils/LoaderCOLLADAConsts.h"

#include "../../../SceneManagement/SceneDescription.h"

#include <Util/Macros.h>

#include <Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wignored-qualifiers)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
#include <COLLADAFWVisualScene.h>
#include <COLLADAFWNode.h>
#include <COLLADAFWMaterialBinding.h>
#include <COLLADAFWInstanceBindingBase.h>
#include <COLLADABUMathMatrix4.h>
COMPILER_WARN_POP

#include <Geometry/Matrix4x4.h>

namespace MinSG {
namespace LoaderCOLLADA {

void resolveReference(SceneManagement::NodeDescription * node, const referenceRegistry_t & referenceRegistry) {
	if(node == nullptr) {
		return;
	}

	std::deque<SceneManagement::NodeDescription *> openList;
	openList.emplace_back(node);

	while(!openList.empty()) {
		SceneManagement::NodeDescription * curNode = openList.front();
		openList.pop_front();

		if(curNode->contains(SceneManagement::Consts::CHILDREN)) {
			auto children = dynamic_cast<const SceneManagement::NodeDescriptionList *>(curNode->getValue(SceneManagement::Consts::CHILDREN));
			if(children != nullptr) {
				for(const auto & childIt : *children) {
					SceneManagement::NodeDescription * child = dynamic_cast<SceneManagement::NodeDescription *>(childIt.get());
					if(child != nullptr) {
						openList.emplace_back(child);
					}
				}
			}
		}

		auto daeRefList = dynamic_cast<const LoaderCOLLADA::daeReferenceList *>(curNode->getValue(LoaderCOLLADA::Consts::DAE_REFERENCE_LIST));
		if(daeRefList == nullptr) {
			continue;
		}

		for(const auto & id : daeRefList->ref()) {
			const auto reference = referenceRegistry.find(id);
			if(reference == referenceRegistry.end()) {
				continue;
			}

			auto refDesc = dynamic_cast<SceneManagement::NodeDescription *>(reference->second);
			if(refDesc != nullptr) {
				addToMinSGChildren(curNode, refDesc);
			}
		}

		curNode->unsetValue(LoaderCOLLADA::Consts::DAE_REFERENCE_LIST);
	}
}

template<COLLADAFW::COLLADA_TYPE::ClassId classId>
void handleInstances(SceneManagement::NodeDescription * parent, const COLLADAFW::InstanceBindingBase<classId> * instance, const referenceRegistry_t & referenceRegistry) {
	SceneManagement::NodeDescription * geoDescription = referenceRegistry.at(instance->getInstanciatedObjectId());
	if(geoDescription == nullptr) {
		return;
	}

	SceneManagement::NodeDescriptionList * geometryList = dynamic_cast<SceneManagement::NodeDescriptionList *>(geoDescription->getValue(SceneManagement::Consts::CHILDREN));

	// Handle geometry without materials
	if(instance->getMaterialBindings().getCount() == 0) {
		for(SceneManagement::NodeDescriptionList::iterator it = geometryList->begin(); it != geometryList->end(); it++) {
			SceneManagement::NodeDescription * child = dynamic_cast<SceneManagement::NodeDescription *>(it->get());
			if(child != nullptr) {
				addToMinSGChildren(parent, child);
			}
		}
		return;
	}

	// Geometry with materials ...
	for(size_t k = 0; k < instance->getMaterialBindings().getCount(); ++k) {
		COLLADAFW::MaterialBinding materialBinding = instance->getMaterialBindings()[k];

		SceneManagement::NodeDescription * meshDescription = nullptr;
		for(SceneManagement::NodeDescriptionList::iterator it = geometryList->begin(); it != geometryList->end(); it++) {
			SceneManagement::NodeDescription * child = dynamic_cast<SceneManagement::NodeDescription *>(it->get());
			if(child->contains(LoaderCOLLADA::Consts::DAE_SUB_MATERIALID)) {
				COLLADAFW::MaterialId materialId = (dynamic_cast<LoaderCOLLADA::MaterialReference *>(child->getValue(LoaderCOLLADA::Consts::DAE_SUB_MATERIALID)))->get();
				if(materialId == materialBinding.getMaterialId()) {
					meshDescription = child;
					break;
				}
			}
		}
		if(meshDescription == nullptr) {
			continue;
		}

		SceneManagement::NodeDescription * materialReference = dynamic_cast<SceneManagement::NodeDescription *>(referenceRegistry.at(materialBinding.getReferencedMaterial()));
		if(materialReference == nullptr) {
			continue;
		}

		daeReferenceData * effectReference = dynamic_cast<daeReferenceData *>(materialReference->getValue(Consts::DAE_REFERENCE));
		if(effectReference == nullptr) {
			continue;
		}

		SceneManagement::NodeDescription * materialEffect = dynamic_cast<SceneManagement::NodeDescription *>(referenceRegistry.at(effectReference->get()));
		if(materialEffect != nullptr) {
			addToMinSGChildren(meshDescription, materialEffect);

			if(materialEffect->contains(Consts::EFFECT_LIST)) {
				SceneManagement::NodeDescriptionList * effectList = dynamic_cast<SceneManagement::NodeDescriptionList *>(materialEffect->getValue(Consts::EFFECT_LIST));
				if(effectList != nullptr) {
					for(size_t i = 0; i < effectList->size(); ++i) {
						SceneManagement::NodeDescription * effectDesc = dynamic_cast<SceneManagement::NodeDescription *>(effectList->at(i));
						if(effectDesc != nullptr) {
							daeReferenceData * effectInstance = dynamic_cast<daeReferenceData *>(effectDesc->getValue(Consts::DAE_REFERENCE));
							if(effectInstance != nullptr) {
								addToMinSGChildren(meshDescription, referenceRegistry.at(effectInstance->get()));
							} else {
								addToMinSGChildren(meshDescription, effectDesc);
							}
						}
					}
				}
			}
		}
		resolveReference(geoDescription, referenceRegistry);
	}
	addToMinSGChildren(parent, geoDescription);
}

void extractInstancesFromNodes(SceneManagement::NodeDescription * parent, 
							   referenceRegistry_t & referenceRegistry,
							   const COLLADAFW::Node * node, 
							   const sceneNodeFunc_t & sceneNodeFunc) {

	const auto & geometryInstances = node->getInstanceGeometries();
	for(size_t i = 0; i < geometryInstances.getCount(); ++i) {
		handleInstances(parent, geometryInstances[i], referenceRegistry);
	}

	const auto & controllerInstances = node->getInstanceControllers();
	for(size_t i = 0; i < controllerInstances.getCount(); ++i) {
		handleInstances(parent, controllerInstances[i], referenceRegistry);
	}

	const auto & lightInstances = node->getInstanceLights();
	for(size_t i = 0; i < lightInstances.getCount(); ++i) {
		addToMinSGChildren(parent, referenceRegistry.at(lightInstances[i]->getInstanciatedObjectId()));
	}

	finalizeNodeDescription(node, parent);

	const auto & childNodes = node->getChildNodes();
	for(size_t i = 0; i < childNodes.getCount(); ++i) {
		auto childDesc = sceneNodeFunc(childNodes[i], referenceRegistry);
		if(childDesc != nullptr) {
			addToMinSGChildren(parent, childDesc);
		}
	}
}

SceneManagement::NodeDescription * sceneNodeImporter(const COLLADAFW::Node * childNode, 
													 referenceRegistry_t & referenceRegistry) {
	if(childNode == nullptr) {
		return nullptr;
	}

	auto listDesc = new SceneManagement::NodeDescription;

	LoaderCOLLADA::addTransformationDataIntoNodeDescription(*listDesc, childNode->getTransformationMatrix());
	listDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
	listDesc->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_LIST);

	extractInstancesFromNodes(listDesc, referenceRegistry, childNode, sceneNodeImporter);

	referenceRegistry[childNode->getUniqueId()] = listDesc;

	return listDesc;
}

bool visualSceneCoreImporter(const COLLADAFW::VisualScene * visualScene, referenceRegistry_t & referenceRegistry, SceneManagement::NodeDescription * sceneDesc, const sceneNodeFunc_t & sceneNodeFunc) {
	sceneDesc->setString(SceneManagement::Consts::ATTR_NODE_ID, visualScene->getName());
	sceneDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
	sceneDesc->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_LIST);

	const COLLADAFW::NodePointerArray & nodes = visualScene->getRootNodes();
	for(size_t i = 0; i < nodes.getCount(); ++i) {

		SceneManagement::NodeDescription * childDesc = sceneNodeFunc(nodes[i], referenceRegistry);
		if(childDesc != nullptr) {
			addToMinSGChildren(sceneDesc, childDesc);
		}
	}

	referenceRegistry_t::const_iterator flagsIt = referenceRegistry.find(COLLADAFW::UniqueId(Consts::DAE_FLAGS));
	if(flagsIt != referenceRegistry.end()) {
		if(flagsIt->second->contains(Consts::DAE_FLAG_USE_TRANSPARENCY_RENDERER)) {
			SceneManagement::NodeDescription * transDesc = new SceneManagement::NodeDescription;
			transDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_STATE);
			transDesc->setString(SceneManagement::Consts::ATTR_STATE_TYPE, SceneManagement::Consts::STATE_TYPE_TRANSPARENCY_RENDERER);
			transDesc->setString(SceneManagement::Consts::ATTR_TRANSPARENY_USE_PREMULTIPLIED_ALPHA, "true");

			addToMinSGChildren(sceneDesc, transDesc);
		}
	}

	return true;
}
}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
