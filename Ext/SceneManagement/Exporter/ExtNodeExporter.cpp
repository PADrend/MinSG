/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExtNodeExporter.h"
#include "../ExtConsts.h"

#include "../../../SceneManagement/SceneDescription.h"
#include "../../../SceneManagement/SceneManager.h"
#include "../../../SceneManagement/Exporter/ExporterTools.h"

#include "../../../Helper/StdNodeVisitors.h"

#include "../../Nodes/BillboardNode.h"
#include "../../Nodes/GenericMetaNode.h"

#include "../../ValuatedRegion/ValuatedRegionNode.h"
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
#include "../../VisibilitySubdivision/VisibilityVector.h"
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
#include "../../SkeletalAnimation/SkeletalNode.h"
#include "../../SkeletalAnimation/Joints/ArmatureNode.h"
#include "../../SkeletalAnimation/Joints/JointNode.h"
#include "../../SkeletalAnimation/Joints/RigidJoint.h"
#endif

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "../../MultiAlgoRendering/MultiAlgoGroupNode.h"
#endif

#ifdef MINSG_EXT_PARTICLE
#include "../../ParticleSystem/ParticleSystemNode.h"
#endif

#ifdef MINSG_EXT_WAYPOINTS
#include "../../Waypoints/PathNode.h"
#include "../../Waypoints/Waypoint.h"
#endif

#include <Util/GenericAttribute.h>
#include <Util/Graphics/Color.h>

#include <Geometry/Rect.h>

#include <string>
#include <map>
#include <cassert>

namespace MinSG {
namespace SceneManagement{


static void describeBillboardNode(ExporterContext &,NodeDescription & desc,Node * node) {
	BillboardNode * bb=dynamic_cast<BillboardNode *>(node);
	assert(bb);
	desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_BILLBOARD);
	Geometry::Rect r=bb->getRect();
	desc.setString(Consts::ATTR_BILLBOARD_RECT,
				 Util::StringUtils::toString(r.getMinX())+' '+
				 Util::StringUtils::toString(r.getMinY())+' '+
				 Util::StringUtils::toString(r.getWidth())+' '+
				 Util::StringUtils::toString(r.getHeight()));
	desc.setString(Consts::ATTR_BILLBOARD_ROTATE_UP,	 bb->getRotateUpAxis() ? "true" : "false");
	desc.setString(Consts::ATTR_BILLBOARD_ROTATE_RIGHT,	 bb->getRotateRightAxis() ? "true" : "false");
}
static void describeGenericMetaNode(ExporterContext &,NodeDescription & desc,Node * _node) {
	GenericMetaNode * node = dynamic_cast<GenericMetaNode *>(_node);
	assert(node);
	desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_GENERIC_META_NODE);
	desc.setString(Consts::ATTR_GENERIC_META_NODE_BB, Util::StringUtils::toString(node->getBB()));
}


#ifdef MINSG_EXT_MULTIALGORENDERING
static void describeMultiAlgoGroupNode(ExporterContext & ctxt,NodeDescription & desc,Node * node) {
	MAR::MultiAlgoGroupNode * magn = dynamic_cast<MAR::MultiAlgoGroupNode *>(node);
	assert(magn);

	desc.setValue(Consts::TYPE, Util::GenericAttribute::createString(Consts::TYPE_NODE));
	desc.setValue(Consts::ATTR_NODE_TYPE,Util::GenericAttribute::createString(Consts::NODE_TYPE_MULTIALGOGROUPNODE));
	desc.setValue(Consts::ATTR_MAGN_NODEID, Util::GenericAttribute::createNumber<uint32_t>(magn->getNodeId()));

	ExporterTools::addChildNodesToDescription(ctxt,desc, magn->getNodeForExport());

//	auto children = new Util::GenericAttributeList();
//	desc.setValue(Consts::CHILDREN,children);
//	const auto childrenNodes = getChildNodes( magn->getNodeForExport() );
//	for(const auto & child : childrenNodes) 
//		children->push_back(ctxt.sceneManager.createDescriptionForNode(ctxt, child));
}
#endif

#ifdef MINSG_EXT_PARTICLE
static void describeParticleSystemNode(ExporterContext &,NodeDescription & desc,Node * node) {
	ParticleSystemNode * psn=dynamic_cast<ParticleSystemNode *>(node);
    desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_PARTICLESYSTEM);
	desc.setValue(Consts::ATTR_PARTICLE_RENDERER, Util::GenericAttribute::createNumber<unsigned int>(psn->getRendererType()));
	desc.setValue(Consts::ATTR_PARTICLE_MAX_PARTICLE_COUNT, Util::GenericAttribute::createNumber(psn->getMaxParticleCount()));
}
#endif

#ifdef MINSG_EXT_WAYPOINTS
static void describePathNode(ExporterContext & ctxt,NodeDescription & desc,Node * node) {
	PathNode * pathNode = dynamic_cast<PathNode *>(node);

	desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_PATH);
	if(pathNode->isLooping()) {
		desc.setString(Consts::ATTR_PATHNODE_LOOPING, "true");
	}

	for(auto & elem : *pathNode) {
		NodeDescription waypointDesc;
		waypointDesc.setString(Consts::ATTR_DATA_TYPE, Consts::NODE_TYPE_WAYPOINT);

		const Waypoint * wp = elem.second.get();
		ExporterTools::addAttributesToDescription(ctxt, waypointDesc, wp->getAttributes());
		waypointDesc.setString(Consts::ATTR_WAYPOINT_TIME, Util::StringUtils::toString(wp->getTime()));
		ExporterTools::addSRTToDescription(waypointDesc, wp->getSRT());
		ExporterTools::addDataEntry(desc,std::move(waypointDesc));
	}
	
}
#endif
#ifdef MINSG_EXT_SKELETAL_ANIMATION
static void describeSkeletalAnimationNode(ExporterContext & ctxt,NodeDescription & desc,Node * node) {
    desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_SKEL_SKELETALOBJECT);
	
	ExporterTools::addChildNodesToDescription(ctxt,desc,node);
}
    
static void describeSkeletalAnimationArmatureNode(ExporterContext & ctxt,NodeDescription & desc,Node * node) {
    desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_SKEL_ARMATURE);
    ExporterTools::addChildNodesToDescription(ctxt,desc,node);
}
    
static void describeSkeletalAnimationRigidJoint(ExporterContext & ctxt,NodeDescription & desc,Node * node) {
    RigidJoint *rNode = dynamic_cast<RigidJoint *>(node);
    desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_SKEL_RIGIDJOINT);
    desc.setValue(Consts::ATTR_SKEL_JOINTID, Util::GenericAttribute::createNumber(rNode->getId()));
    desc.setString(Consts::ATTR_NODE_ID, rNode->getName());
    std::stringstream ss;
    ss << rNode->getInverseBindMatrix();
    desc.setString(Consts::ATTR_SKEL_INVERSEBINDMATRIX, ss.str());
    
    ss.str("");
    ss << (*rNode->getBindMatrix());
    desc.setString(Consts::ATTR_MATRIX, ss.str());
    
	ExporterTools::addChildNodesToDescription(ctxt,desc,node);
    
    if(rNode->isStacking())
        desc.setString(Consts::ATTR_SKEL_RIGIDSTACKING, "true");
    
    ss.str("");
    ss << rNode->getOffsetMatrix();
    desc.setString(Consts::ATTR_SKEL_RIGIDOFFSETMATRIX, ss.str());
}
    
static void describeSkeletalAnimationJointNode(ExporterContext & ctxt,NodeDescription & desc,Node * node) {
    JointNode *jNode = dynamic_cast<JointNode *>(node);
    
    desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_SKEL_JOINT);
    desc.setValue(Consts::ATTR_SKEL_JOINTID, Util::GenericAttribute::createNumber(jNode->getId()));
    desc.setString(Consts::ATTR_NODE_ID, jNode->getName());
    std::stringstream ss;
    ss << jNode->getInverseBindMatrix();
    desc.setString(Consts::ATTR_SKEL_INVERSEBINDMATRIX, ss.str());

    ss.str("");
    ss << (*jNode->getBindMatrix());
    desc.setString(Consts::ATTR_MATRIX, ss.str());
    
    ExporterTools::addChildNodesToDescription(ctxt,desc,node);
}
#endif

static void describeValuatedRegionNode(ExporterContext & ctxt,NodeDescription & desc,Node * node) {
	ValuatedRegionNode * cn = dynamic_cast<ValuatedRegionNode *>(node);
	assert(cn);
	desc.setString(Consts::ATTR_NODE_TYPE,Consts::NODE_TYPE_VALUATED_REGION);

	{
		std::stringstream s;
		Geometry::Vec3 center = cn->getBB().getCenter();
		s << center.getX() << " " << center.getY() << " " << center.getZ() << " "
		  << cn->getBB().getExtentX() << " " << cn->getBB().getExtentY() << " " << cn->getBB().getExtentZ();
		desc.setString(Consts::ATTR_VALREGION_BOX, s.str());
	}

	{
		std::stringstream s;
		s << cn->getXResolution() << " " << cn->getYResolution() << " " << cn->getZResolution();
		desc.setString(Consts::ATTR_VALREGION_RESOLUTION, s.str());
	}

	if(cn->isLeaf() && cn->getValue()) {
		Util::GenericAttribute * v = cn->getValue();

		std::ostringstream valueString;
		Util::GenericAttribute::List * valueList = dynamic_cast<Util::GenericAttribute::List *>(v);
		if(valueList != nullptr) {
			for(Util::GenericAttribute::List::const_iterator it = valueList->begin();
					it != valueList->end();
					++it) {
				// Insert separator character.
				if(it != valueList->begin()) {
					valueString << ' ';
				}
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
				// Special case to store a VisibilityVector.
				const auto * vva = dynamic_cast<const VisibilitySubdivision::VisibilityVectorAttribute *>(it->get());
				if(vva != nullptr) {
					std::ostringstream id;
					const auto & vv = vva->ref();
					id << "VV" << reinterpret_cast<uintptr_t>(&vv);

					// VV not already inserted?
					if(!ctxt.isPrototypeUsed(id.str())) {
						NodeDescription nd;
						nd.setString(Consts::TYPE, Consts::TYPE_ADDITIONAL_DATA);

						bool firstElement = true;
						std::ostringstream vvValue;
						vvValue << '(';
						const uint32_t maxIndex = vv.getIndexCount();
						for(uint_fast32_t index = 0; index < maxIndex; ++index) {
							const auto benefits = vv.getBenefits(index);
							// Only save visible nodes.
							if(benefits > 0) {
								if(firstElement) {
									firstElement = false;
								} else {
									vvValue << ", ";
								}
								vvValue << '('
										<< ctxt.sceneManager.getNameOfRegisteredNode(vv.getNode(index)) << ", "
										<< vv.getCosts(index) << ", "
										<< benefits << ')';
							}
						}
						vvValue << ')';
						nd.setString(Consts::ATTR_NODE_TYPE, "visibility_vector");
						nd.setString(Consts::ATTR_ATTRIBUTE_VALUE, vvValue.str());
						nd.setString(Consts::ATTR_NODE_ID, id.str());

						ctxt.addUsedPrototype(id.str(), std::move(nd));
					}

					valueString << id.str();
					desc.setString(Consts::ATTR_VALREGION_VALUE_TYPE, "VisibilityVector");
				} else {
#endif
					desc.setString(Consts::ATTR_VALREGION_VALUE_TYPE, "Default");
					valueString << (*it)->toString();
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
				}
#endif
			}
		} else
			if(v != nullptr) {
				desc.setString(Consts::ATTR_VALREGION_VALUE_TYPE, "Default");
				valueString << v->toString();
			}
		if(!valueString.str().empty()) {
			desc.setString(Consts::ATTR_VALREGION_VALUE, valueString.str());
		}

		if(cn->getAdditionalData() != nullptr) {
			std::ostringstream s;
			for(auto it =
						cn->getAdditionalData()->colors.begin(); it
					!= cn->getAdditionalData()->colors.end(); ++it) {
				if(it != cn->getAdditionalData()->colors.begin()) {
					s << " ";
				}
				s << (*it).getR() << " " << (*it).getG() << " " << (*it).getB() << " "
				  << (*it).getA();
			}
			desc.setString(Consts::ATTR_VALREGION_COLOR, s.str());
			if(cn->getHeightScale()!=1) {
				desc.setString(Consts::ATTR_VALREGION_HEIGHT,Util::StringUtils::toString(cn->getHeightScale()));

			}
		}
	}

	if(!cn->isLeaf()) {
		ExporterTools::addChildNodesToDescription(ctxt,desc,cn);
	}
}


void initExtNodeExporter(SceneManager & sm) {
	sm.addNodeExporter(BillboardNode::getClassId(),&describeBillboardNode);
	sm.addNodeExporter(GenericMetaNode::getClassId(),&describeGenericMetaNode);
	sm.addNodeExporter(ValuatedRegionNode::getClassId(),&describeValuatedRegionNode);

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	sm.addNodeExporter(SkeletalNode::getClassId(),&describeSkeletalAnimationNode);
   	sm.addNodeExporter(JointNode::getClassId(),&describeSkeletalAnimationJointNode);
  	sm.addNodeExporter(ArmatureNode::getClassId(),&describeSkeletalAnimationArmatureNode);
    sm.addNodeExporter(RigidJoint::getClassId(),&describeSkeletalAnimationRigidJoint);
#endif

#ifdef MINSG_EXT_MULTIALGORENDERING
	sm.addNodeExporter(MAR::MultiAlgoGroupNode::getClassId(),&describeMultiAlgoGroupNode);
#endif

#ifdef MINSG_EXT_PARTICLE
	sm.addNodeExporter(ParticleSystemNode::getClassId(),&describeParticleSystemNode);
#endif

#ifdef MINSG_EXT_WAYPOINTS
	sm.addNodeExporter(PathNode::getClassId(),&describePathNode);
#endif
}

}
}
