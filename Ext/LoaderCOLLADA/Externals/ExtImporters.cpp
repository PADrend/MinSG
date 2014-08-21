/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "ExtImporters.h"
#include "ExtConsts.h"

#include "../Utils/LoaderCOLLADAConsts.h"
#include "../Writer.h"
#include "../Utils/DescriptionUtils.h"

#include "../../../SceneManagement/SceneDescription.h"
#include <Util/GenericAttribute.h>

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include <Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wignored-qualifiers)
#include <COLLADAFWController.h>
#include <COLLADAFWSkinController.h>
#include <COLLADAFWSkinControllerData.h>
#include <COLLADAFWTypes.h>
#include <COLLADAFWVisualScene.h>
#include <COLLADAFWAnimation.h>
#include <COLLADAFWAnimationCurve.h>
#include <COLLADAFWAnimationList.h>
COMPILER_WARN_POP

#include "../../SkeletalAnimation/Util/SkeletalAnimationUtils.h"
#include "../Core/VisualSceneImporter.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Serialization/Serialization.h>

#include <Util/JSON_Parser.h>

#include <sstream>
#include <vector>

#endif /* MINSG_EXT_SKELETAL_ANIMATION */

namespace MinSG {
namespace LoaderCOLLADA {
namespace ExternalImporter {

#ifdef MINSG_EXT_SKELETAL_ANIMATION
static void appendSkeletalAttributes(Rendering::MeshVertexData & vertexData, const SceneManagement::DescriptionMap * vertexDesc,
									 const std::vector<uint32_t> triArray) {

	Rendering::VertexDescription vd = vertexData.getVertexDescription();

	const Rendering::VertexAttribute & weightAttr1 = vd.getAttribute("sg_Weights1");
	const Rendering::VertexAttribute & weightAttr2 = vd.getAttribute("sg_Weights2");
	const Rendering::VertexAttribute & weightAttr3 = vd.getAttribute("sg_Weights3");
	const Rendering::VertexAttribute & weightAttr4 = vd.getAttribute("sg_Weights4");

	const Rendering::VertexAttribute & weightAttrIndex1 = vd.getAttribute("sg_WeightsIndex1");
	const Rendering::VertexAttribute & weightAttrIndex2 = vd.getAttribute("sg_WeightsIndex2");
	const Rendering::VertexAttribute & weightAttrIndex3 = vd.getAttribute("sg_WeightsIndex3");
	const Rendering::VertexAttribute & weightAttrIndex4 = vd.getAttribute("sg_WeightsIndex4");

	const Rendering::VertexAttribute & weightAttrCount = vd.getAttribute("sg_WeightsCount");

	SceneManagement::floatVecWrapper_t * weights = dynamic_cast<SceneManagement::floatVecWrapper_t *>(vertexDesc->getValue(LoaderCOLLADA::Consts::DAE_WEIGHT_ARRAY));
	SceneManagement::uint32VecWrapper_t * weightIndices = dynamic_cast<SceneManagement::uint32VecWrapper_t *>(vertexDesc->getValue(LoaderCOLLADA::Consts::DAE_WEIGHT_INDICES));
	SceneManagement::uint32VecWrapper_t * jointIndices = dynamic_cast<SceneManagement::uint32VecWrapper_t *>(vertexDesc->getValue(LoaderCOLLADA::Consts::DAE_WEIGHT_JOINTINDICES));
	SceneManagement::uint32VecWrapper_t * jointsPerVertex = dynamic_cast<SceneManagement::uint32VecWrapper_t *>(vertexDesc->getValue(LoaderCOLLADA::Consts::DAE_WEIGHT_JOINTSPERVERTEX));

	std::vector<uint32_t> jointPositionIndices(jointsPerVertex->ref().size(), 0);
	jointPositionIndices[0] = 0;
	for(uint32_t i = 1; i < jointsPerVertex->ref().size(); ++i) {
		jointPositionIndices[i] = jointPositionIndices[i - 1] + jointsPerVertex->ref()[i - 1];
	}

	for(uint32_t i = 0; i < vertexData.getVertexCount(); ++i) {
		uint32_t daeJointId = triArray[i];

		uint32_t indexPosition = jointPositionIndices[daeJointId];

		float * value = reinterpret_cast<float *>(vertexData[i] + weightAttrCount.getOffset());
		value[0] = jointsPerVertex->ref()[daeJointId];

		for(uint32_t j = 0; j < jointsPerVertex->ref()[daeJointId]; ++j) {
			if(j < 4) {
				value = reinterpret_cast<float *>(vertexData[i] + weightAttr1.getOffset());
				value[j] = weights->ref()[weightIndices->ref()[indexPosition]];

				value = reinterpret_cast<float *>(vertexData[i] + weightAttrIndex1.getOffset());
				value[j] = jointIndices->ref()[indexPosition];
			} else if(j < 8) {
				value = reinterpret_cast<float *>(vertexData[i] + weightAttr2.getOffset());
				value[j - 4] = weights->ref()[weightIndices->ref()[indexPosition]];

				value = reinterpret_cast<float *>(vertexData[i] + weightAttrIndex2.getOffset());
				value[j - 4] = jointIndices->ref()[indexPosition];
			} else if(j < 12) {
				value = reinterpret_cast<float *>(vertexData[i] + weightAttr3.getOffset());
				value[j - 8] = weights->ref()[weightIndices->ref()[indexPosition]];

				value = reinterpret_cast<float *>(vertexData[i] + weightAttrIndex3.getOffset());
				value[j - 8] = jointIndices->ref()[indexPosition];
			} else if(j < 16) {
				value = reinterpret_cast<float *>(vertexData[i] + weightAttr4.getOffset());
				value[j - 12] = weights->ref()[weightIndices->ref()[indexPosition]];

				value = reinterpret_cast<float *>(vertexData[i] + weightAttrIndex4.getOffset());
				value[j - 12] = jointIndices->ref()[indexPosition];
			}

			indexPosition++;
		}
	}

	vertexData.markAsChanged();
}


static bool importSkeletonController(const COLLADAFW::Controller * controller, referenceRegistry_t & referenceReg) {
	if(controller->getControllerType() != COLLADAFW::Controller::CONTROLLER_TYPE_SKIN) {
		return false;
	}

	const COLLADAFW::SkinController * skinController = dynamic_cast<const COLLADAFW::SkinController *>(controller);

	SceneManagement::DescriptionMap * skinControllerDesc = new SceneManagement::DescriptionMap();
	SceneManagement::DescriptionMap * skinDescription = referenceReg[skinController->getSkinControllerData()];
	LoaderCOLLADA::copyAttributesToNodeDescription(skinControllerDesc, skinDescription);

	referenceReg.erase(skinController->getSkinControllerData());

	SceneManagement::floatVecWrapper_t * invBindMatrices = dynamic_cast<SceneManagement::floatVecWrapper_t *>(skinDescription->getValue(LoaderCOLLADA::Consts::DAE_JOINT_INVERSEBINDMATRIX));
	if(invBindMatrices == nullptr) {
		return false;
	}

	LoaderCOLLADA::daeReferenceList * jointIds = new LoaderCOLLADA::daeReferenceList();
	uint32_t idCounter = 0;
	for(uint32_t i = 0; i < skinController->getJoints().getCount(); ++i) {
		std::stringstream ss;
		ss << idCounter;
		SceneManagement::DescriptionMap * jointDesc = new SceneManagement::DescriptionMap();
		jointDesc->setString(SceneManagement::Consts::ATTR_SKEL_JOINTID, ss.str());
		idCounter++;

		ss.str("");
		for(uint8_t j = 0; j < 16; ++j) {
			ss << invBindMatrices->ref().at(i * 16 + j) << " ";
		}
		jointDesc->setString(SceneManagement::Consts::ATTR_SKEL_INVERSEBINDMATRIX, ss.str());
		referenceReg[skinController->getJoints()[i]] = jointDesc;

		jointIds->ref().emplace_back(skinController->getJoints()[i]);
	}
	skinControllerDesc->setValue(LoaderCOLLADA::Consts::DAE_JOINT_IDS, jointIds);

	SceneManagement::DescriptionArray * geometries = dynamic_cast<SceneManagement::DescriptionArray *>(referenceReg[skinController->getSource()]->getValue(SceneManagement::Consts::CHILDREN));
	for(uint32_t i = 0; i < geometries->size(); ++i) {
		SceneManagement::DescriptionMap * geometryChild = dynamic_cast<SceneManagement::DescriptionMap *>(geometries->at(i));
		if(geometryChild == nullptr) {
			continue;
		}

		SceneManagement::DescriptionArray * meshList = dynamic_cast<SceneManagement::DescriptionArray *>(geometryChild->getValue(SceneManagement::Consts::CHILDREN));
		if(meshList == nullptr) {
			continue;
		}

		for(uint32_t j = 0; j < meshList->size(); ++j) {
			SceneManagement::DescriptionMap * meshDesc = dynamic_cast<SceneManagement::DescriptionMap *>(meshList->at(j));
			if(meshDesc == nullptr) {
				continue;
			}

			if(meshDesc->getString(SceneManagement::Consts::ATTR_DATA_TYPE) != "mesh") {
				continue;
			}

			SceneManagement::uint32VecWrapper_t * triArrayDesc = dynamic_cast<SceneManagement::uint32VecWrapper_t *>(meshDesc->getValue(LoaderCOLLADA::Consts::DAE_MESH_VERTEXORDER));

			if(triArrayDesc == nullptr) {
				continue;
			}

			Rendering::Serialization::MeshWrapper_t * meshWrapper = dynamic_cast<Rendering::Serialization::MeshWrapper_t *>(meshDesc->getValue(SceneManagement::Consts::ATTR_MESH_DATA));

			Rendering::Mesh * mesh = meshWrapper->get();
			SkeletalAnimationUtils::appendSkeletanDescriptionToMesh(&mesh->openVertexData());
			appendSkeletalAttributes(mesh->openVertexData(), skinDescription, triArrayDesc->ref());
		}
	}

	skinControllerDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
	skinControllerDesc->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_SKEL_SKELETALOBJECT);
	copyAttributesToNodeDescription(skinControllerDesc, referenceReg[skinController->getSource()]);

	SceneManagement::DescriptionMap * shaderState = new SceneManagement::DescriptionMap();
	shaderState->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_STATE);
	shaderState->setString(SceneManagement::Consts::ATTR_STATE_TYPE, SceneManagement::Consts::STATE_TYPE_SKEL_SKELETALHARDWARERENDERERSTATE);
	shaderState->setString(SceneManagement::Consts::ATTR_SKEL_BINDMATRIX, skinControllerDesc->getString(LoaderCOLLADA::Consts::DAE_ARMATURE_BINDMATRIX));
	addToMinSGChildren(skinControllerDesc, shaderState);

	SceneManagement::DescriptionMap * armature = new SceneManagement::DescriptionMap();
	armature->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
	armature->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_SKEL_ARMATURE);
	daeReferenceList * refList = new daeReferenceList();
	if(skinController->getJoints().getCount() > 0) {
		refList->ref().emplace_back(skinController->getJoints()[0]);
		armature->setValue(LoaderCOLLADA::Consts::DAE_REFERENCE_LIST, refList);
	}

	addToMinSGChildren(skinControllerDesc, armature);
	referenceReg[controller->getUniqueId()] = skinControllerDesc;

	return true;
}

static bool importSkinControllerData(const COLLADAFW::SkinControllerData * skinController, referenceRegistry_t & referenceReg) {

	SceneManagement::DescriptionMap * skinDescription = new SceneManagement::DescriptionMap();
	skinDescription->setValue(LoaderCOLLADA::Consts::DAE_JOINT_COUNT,
							  new Util::_NumberAttribute<uint32_t>(skinController->getJointsCount()));

	std::stringstream bindMatrix;
	for(uint8_t i = 0; i < 4; ++i) {
		for(uint8_t j = 0; j < 4; ++j) {
			bindMatrix << skinController->getBindShapeMatrix().getElement(i, j) << " ";
		}
	}
	skinDescription->setString(LoaderCOLLADA::Consts::DAE_ARMATURE_BINDMATRIX, bindMatrix.str());

	SceneManagement::floatVecWrapper_t * weights = new SceneManagement::floatVecWrapper_t();
	const COLLADAFW::FloatArray * weightArray = skinController->getWeights().getFloatValues();
	for(uint32_t i = 0; i < weightArray->getCount(); ++i) {
		weights->ref().emplace_back(weightArray->getData()[i]);
	}
	skinDescription->setValue(LoaderCOLLADA::Consts::DAE_WEIGHT_ARRAY, weights);

	SceneManagement::uint32VecWrapper_t * weightIndices = new SceneManagement::uint32VecWrapper_t();
	for(uint32_t i = 0; i < skinController->getWeightIndices().getCount(); ++i) {
		weightIndices->ref().emplace_back(skinController->getWeightIndices().getData()[i]);
	}
	skinDescription->setValue(LoaderCOLLADA::Consts::DAE_WEIGHT_INDICES, weightIndices);

	SceneManagement::uint32VecWrapper_t * jointIndices = new SceneManagement::uint32VecWrapper_t();
	for(uint32_t i = 0; i < skinController->getJointIndices().getCount(); ++i) {
		jointIndices->ref().emplace_back(skinController->getJointIndices().getData()[i]);
	}
	skinDescription->setValue(LoaderCOLLADA::Consts::DAE_WEIGHT_JOINTINDICES, jointIndices);

	SceneManagement::uint32VecWrapper_t * jointsPerVertex = new SceneManagement::uint32VecWrapper_t;
	for(uint32_t i = 0; i < skinController->getJointsPerVertex().getCount(); ++i) {
		jointsPerVertex->ref().emplace_back(skinController->getJointsPerVertex()[i]);
	}
	skinDescription->setValue(LoaderCOLLADA::Consts::DAE_WEIGHT_JOINTSPERVERTEX, jointsPerVertex);

	SceneManagement::floatVecWrapper_t * inverseBindMatrices = new SceneManagement::floatVecWrapper_t();
	for(uint32_t i = 0; i < skinController->getInverseBindMatrices().getCount(); ++i) {
		for(uint8_t j = 0; j < 4; ++j) {
			for(uint8_t k = 0; k < 4; ++k) {
				inverseBindMatrices->ref().emplace_back(skinController->getInverseBindMatrices().getData()[i].getElement(j, k));
			}
		}
	}
	skinDescription->setValue(LoaderCOLLADA::Consts::DAE_JOINT_INVERSEBINDMATRIX, inverseBindMatrices);

	referenceReg[skinController->getUniqueId()] = skinDescription;

	return true;
}

static SceneManagement::DescriptionMap * sceneNodeImporter(const COLLADAFW::Node * childNode, referenceRegistry_t & referenceRegistry) {
	if(childNode == nullptr) {
		return nullptr;
	}

	SceneManagement::DescriptionMap * listDesc;
	referenceRegistry_t::iterator childIt = referenceRegistry.find(childNode->getUniqueId());
	if(childIt == referenceRegistry.end()) {
		listDesc = new SceneManagement::DescriptionMap();
	} else {
		listDesc = childIt->second;
		if(listDesc == nullptr) {
			listDesc = new SceneManagement::DescriptionMap();
		}
	}

	LoaderCOLLADA::addTransformationDataIntoNodeDescription(*listDesc, childNode->getTransformationMatrix());

	listDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
	extractInstancesFromNodes(listDesc, referenceRegistry, childNode, sceneNodeImporter);

	if(childNode->getType() == COLLADAFW::Node::NODE) {
		listDesc->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_LIST);
	} else if(childNode->getType() == COLLADAFW::Node::JOINT) {
		listDesc->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_SKEL_JOINT);
	}
	listDesc->setString(SceneManagement::Consts::ATTR_NODE_ID, childNode->getOriginalId());

	SceneManagement::DescriptionArray * children = dynamic_cast<SceneManagement::DescriptionArray *>(listDesc->getValue(SceneManagement::Consts::CHILDREN));
	if(children != nullptr) {
		for(SceneManagement::DescriptionArray::iterator it = children->begin(); it < children->end(); ++it) {
			SceneManagement::DescriptionMap * child = dynamic_cast<SceneManagement::DescriptionMap *>(it->get());

			if(child != nullptr) {
				if(child->getString(SceneManagement::Consts::ATTR_NODE_TYPE) == SceneManagement::Consts::NODE_TYPE_SKEL_SKELETALOBJECT) {
					listDesc = child;
					break;
				}
			}
		}
	}

	referenceRegistry[childNode->getUniqueId()] = listDesc;

	return listDesc;
}

static bool animationImporter(const COLLADAFW::Animation * animation, referenceRegistry_t & referenceRegistry) {
	if(animation->getAnimationType() != COLLADAFW::Animation::AnimationType::ANIMATION_CURVE) {
		return false;
	}

	const COLLADAFW::AnimationCurve * animationCurve = dynamic_cast<const COLLADAFW::AnimationCurve *>(animation);
	if(animationCurve == nullptr) {
		return false;
	}

	SceneManagement::DescriptionMap * animationDesc = new SceneManagement::DescriptionMap();

	if(animationCurve->getInPhysicalDimension() != COLLADAFW::PHYSICAL_DIMENSION_TIME) {
		return false;
	}

	std::stringstream ss;
	{
		COLLADAFW::FloatOrDoubleArray input = animationCurve->getInputValues();
		for(uint32_t i = 0; i < input.getFloatValues()->getCount(); ++i) {
			ss << input.getFloatValues()->getData()[i];
			if(i != input.getFloatValues()->getCount() - 1) {
				ss << " ";
			}
		}
		animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_TIMELINE, ss.str());
	}
	ss.str("");

	{
		COLLADAFW::FloatOrDoubleArray output = animationCurve->getOutputValues();
		for(uint32_t i = 0; i < output.getFloatValues()->getCount(); ++i) {
			ss << output.getFloatValues()->getData()[i];
			if(i != output.getFloatValues()->getCount() - 1) {
				ss << " ";
			}
		}
		animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALSAMPLERDATA, ss.str());
	}
	ss.str("");

	{
		if(animationCurve->getInterpolationType() == COLLADAFW::AnimationCurve::INTERPOLATION_LINEAR) {
			uint32_t typeSize = animationCurve->getKeyCount() - 1;
			for(uint32_t i = 0; i < typeSize; ++i) {
				ss << "LINEAR ";
			}
		}
		animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALINTERPOLATIONTYPE, ss.str());
	}
	ss.str("");

	{
		std::string animationName = animation->getOriginalId();
		uint32_t fIndex = 0;
		for(auto item : animationName) {
			fIndex++;
			if(item == '_') {
				break;
			}
		}

		uint32_t delimeterCounter = 0;
		uint32_t i;
		for(i = animationName.size() - 1; i > fIndex; i--) {
			if(animationName[i] == '_') {
				delimeterCounter++;
			}

			if(delimeterCounter == 2) {
				break;
			}
		}

		std::string targetName = animationName.substr(fIndex, i - fIndex);
		animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALANIMATIONTARGET, targetName);
	}

	animationDesc->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_SKEL_ANIMATIONSAMPLE);
	animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALANIMATIONSTARTTIME, "0");

	if(!animation->getName().empty()) {
		animationDesc->setString(LoaderCOLLADA::Consts::DAE_ANIMATION_NAME, animation->getName());
	}

	referenceRegistry[animationCurve->getUniqueId()] = animationDesc;

	return true;
}

static bool animationListImporter(const COLLADAFW::AnimationList * animationList, referenceRegistry_t & referenceRegistry) {

	const auto container = findDescriptions(referenceRegistry, SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_SKEL_SKELETALOBJECT);
	if(container.empty()) {
		return false;
	}
	SceneManagement::DescriptionMap * skeletDesc = container.front();

	SceneManagement::DescriptionMap * animationDesc = nullptr;
	SceneManagement::DescriptionArray * aniDesc = nullptr;
	{
		const auto aniContainer = findDescriptions(referenceRegistry, SceneManagement::Consts::ATTR_BEHAVIOUR_TYPE, SceneManagement::Consts::BEHAVIOUR_TYPE_SKEL_ANIMATIONDATA);

		if(!aniContainer.empty()) {
			animationDesc = aniContainer.front();

			aniDesc = dynamic_cast<SceneManagement::DescriptionArray *>(Util::JSON_Parser::parse(animationDesc->getString(SceneManagement::Consts::DATA_BLOCK)));
		} else {
			animationDesc = new SceneManagement::DescriptionMap();
			aniDesc = new SceneManagement::DescriptionArray();

			animationDesc->setString(SceneManagement::Consts::ATTR_BEHAVIOUR_TYPE, SceneManagement::Consts::BEHAVIOUR_TYPE_SKEL_ANIMATIONDATA);

			animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALANIMATIONNAME, "std");
			animationDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_BEHAVIOUR);

			referenceRegistry[animationList->getUniqueId()] = animationDesc;

			addToMinSGChildren(skeletDesc, animationDesc);
		}
	}

	LoaderCOLLADA::daeReferenceList * jointIds = dynamic_cast<LoaderCOLLADA::daeReferenceList *>(skeletDesc->getValue(LoaderCOLLADA::Consts::DAE_JOINT_IDS));
	std::vector<std::string> jointNames;
	for(auto jId : jointIds->ref()) {
		jointNames.emplace_back(referenceRegistry[jId]->getString(SceneManagement::Consts::ATTR_NODE_ID));
	}

	struct StringSorter {
		bool operator()(const std::string & a, const std::string & b) const {
			return a.size() > b.size();
		}
	} stringSorter;

	std::sort(jointNames.begin(), jointNames.end(), stringSorter);
	if(jointIds != nullptr) {
		for(uint32_t i = 0; i < animationList->getAnimationBindings().getCount(); ++i) {
			SceneManagement::DescriptionMap * ani = referenceRegistry[animationList->getAnimationBindings().getData()[i].animation];
			aniDesc->push_back(ani);
			if(ani->contains(LoaderCOLLADA::Consts::DAE_ANIMATION_NAME)) {
				std::string name = ani->getString(LoaderCOLLADA::Consts::DAE_ANIMATION_NAME);
				for(uint32_t j = 0; j < jointNames.size(); ++j) {
					if(name.find(jointNames[j]) != std::string::npos) {
						ani->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALANIMATIONTARGET, jointNames[j]);
						jointNames.erase(jointNames.begin() + j);
						break;
					}
				}
			}
		}
	}

	animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALSTARTANIMATION, "true");
	animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALTOANIMATIONS, "");
	animationDesc->setString(SceneManagement::Consts::ATTR_SKEL_SKELETALFROMANIMATIONS, "");
	animationDesc->setString(SceneManagement::Consts::DATA_BLOCK, aniDesc->toJSON());

	return true;
}
#endif /* MINSG_EXT_SKELETAL_ANIMATION */

void initExternalFunctions(Writer & writer) {

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	writer.setControllerFunction(&importSkeletonController);
	writer.setSkinControllerFunction(&importSkinControllerData);
	writer.setSceneNodeFunction(sceneNodeImporter);
	writer.setAnimationFunction(&animationImporter);
	writer.setAnimationListFunction(&animationListImporter);
#endif
}
}
}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
