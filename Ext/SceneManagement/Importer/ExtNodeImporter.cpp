/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExtNodeImporter.h"

#include "../ExtConsts.h"
#include "../../../SceneManagement/SceneDescription.h"
#include "../../../SceneManagement/SceneManager.h"
#include "../../../SceneManagement/Importer/ImporterTools.h"

#include "../../../Core/Nodes/Node.h"
#include "../../../Core/Nodes/GroupNode.h"

#include "../../ValuatedRegion/ValuatedRegionNode.h"
#include "../../Nodes/BillboardNode.h"
#include "../../Nodes/GenericMetaNode.h"

#include <functional>
#include <cassert>

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "../../MultiAlgoRendering/MultiAlgoGroupNode.h"
#endif

#ifdef MINSG_EXT_PARTICLE
#include "../../ParticleSystem/ParticleSystemNode.h"
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
#include "../../SkeletalAnimation/SkeletalNode.h"
#include "../../SkeletalAnimation/Joints/ArmatureNode.h"
#include "../../SkeletalAnimation/Renderer/SkeletalHardwareRendererState.h"
#include "../../SkeletalAnimation/JointPose/MatrixPose.h"
#include "../../SkeletalAnimation/Joints/JointNode.h"
#include "../../SkeletalAnimation/Joints/RigidJoint.h"

#include <iterator>
#include <string>
using namespace Geometry;
using namespace std;
#endif /* MINSG_EXT_SKELETAL_ANIMATION */

#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
#include "../../VisibilitySubdivision/VisibilityVector.h"
#endif

#ifdef MINSG_EXT_WAYPOINTS
#include "../../Waypoints/PathNode.h"
#include "../../Waypoints/Waypoint.h"
#endif

#include <Util/Macros.h>
#include <Geometry/Rect.h>

namespace MinSG {
namespace SceneManagement{

template < typename T >
static T * convertToTNode(Node * node) {
	T * t = dynamic_cast<T *>(node);
	if(t == nullptr)
		WARN(std::string(node != nullptr ? node->getTypeName() : "nullptr") + " can not be casted to " + T::getClassName());
	return t;
}

static bool importBillboardNode(ImportContext & ctxt, const std::string & type, const NodeDescription & d, GroupNode * parent) {
	if(type != "billboard")
		return false;
	
	std::vector<float> r=Util::StringUtils::toFloats(d.getString(Consts::ATTR_BILLBOARD_RECT,"-0.5 -0.5 1.0 1.0"));
	if(r.size() != 4) {
		WARN(" ");
		return false;
	}
	bool rotateUpAxis=d.getString(Consts::ATTR_BILLBOARD_ROTATE_UP,"false") == "true";
	bool rotateRightAxis=d.getString(Consts::ATTR_BILLBOARD_ROTATE_RIGHT,"false") == "true";
	auto node=new BillboardNode(Geometry::Rect(r[0],r[1],r[2],r[3]),rotateUpAxis,rotateRightAxis);
	ImporterTools::finalizeNode(ctxt,node,d);
	parent->addChild(node);
	return true;
}


static bool importGenericMetaNode(ImportContext & ctxt, const std::string & nodeType, const NodeDescription & d, GroupNode * parent) {
	if(nodeType != Consts::NODE_TYPE_GENERIC_META_NODE)
		return false;

	auto node = new GenericMetaNode;
	std::istringstream os( d.getString(Consts::ATTR_GENERIC_META_NODE_BB,"0 0 0 1 1 1") );
	Geometry::Box bb;
	os >> bb;
	node->setBB(bb);

	ImporterTools::finalizeNode(ctxt, node, d);
	parent->addChild(node);
	return true;
}

#ifdef MINSG_EXT_MULTIALGORENDERING
static bool importMultiAlgoGroupNode(ImportContext & ctxt, const std::string & type, const NodeDescription & d, GroupNode * parent) {
	if(type != Consts::NODE_TYPE_MULTIALGOGROUPNODE)
		return false;

	Util::Reference<MAR::MultiAlgoGroupNode> node = new MAR::MultiAlgoGroupNode();
	node->setAlgorithm(MAR::MultiAlgoGroupNode::BruteForce);
	node->setNodeId(d.getUInt(Consts::ATTR_MAGN_NODEID));
	parent->addChild(node.get());
	ImporterTools::finalizeNode(ctxt, node.get(), d);
	return node.detachAndDecrease();
}
#endif


#ifdef MINSG_EXT_PARTICLE
static bool importParticleSystemNode(ImportContext & ctxt, const std::string & type, const NodeDescription & d, GroupNode * parent) {
	if(type != Consts::NODE_TYPE_PARTICLESYSTEM)
		return false;

	ParticleSystemNode::ref_t node = new ParticleSystemNode();

	
	if(d.contains(Consts::ATTR_PARTICLE_RENDERER)) {
		ParticleSystemNode::renderer_t rendererType;
		switch(d.getValue(Consts::ATTR_PARTICLE_RENDERER)->toUnsignedInt()) {
			case ParticleSystemNode::BILLBOARD_RENDERER:
				rendererType = ParticleSystemNode::BILLBOARD_RENDERER;
				break;
			case ParticleSystemNode::POINT_RENDERER:
			default:
				rendererType = ParticleSystemNode::POINT_RENDERER;
				break;
		}
		node.get()->setRenderer(rendererType);
	}

	if(d.contains(Consts::ATTR_PARTICLE_MAX_PARTICLE_COUNT)) {
		node.get()->setMaxParticleCount(d.getValue(Consts::ATTR_PARTICLE_MAX_PARTICLE_COUNT)->toUnsignedInt());
	}

	ImporterTools::finalizeNode(ctxt,node.get(),d);
	parent->addChild(node.get());
	return true;
}
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION

static bool importSkeletalAnimationSkeletalNode(ImportContext & ctxt, const std::string & type, const NodeDescription & d, GroupNode * parent) {

	Node * node = nullptr;

	if(type == Consts::NODE_TYPE_SKEL_SKELETALOBJECT)
		node = new SkeletalNode();
    else
        return false;

	if(node == nullptr)
		return false;

	ImporterTools::finalizeNode(ctxt,node,d);
	parent->addChild(node);
	return true;
}
    
static bool importSkeletalAnimationRigidJoint(ImportContext & ctxt, const std::string & type, const NodeDescription & d, GroupNode *parent) {
    Node *node = nullptr;
    
	if(type != Consts::NODE_TYPE_SKEL_RIGIDJOINT)
		return false;
    
	if(!d.contains(Consts::ATTR_SKEL_JOINTID) || !d.contains(Consts::ATTR_NODE_ID))
		return false;
    
    float offsetArray[16];
    if(d.contains(Consts::ATTR_SKEL_RIGIDOFFSETMATRIX))
    {
        vector<float> offset = Util::StringUtils::toFloats(d.getString(Consts::ATTR_SKEL_RIGIDOFFSETMATRIX));
        if(offset.size() != 16)
        {
            WARN("Offsetmatrix size != 16.");
            return false;
        }
       
        for(uint32_t i=0; i<offset.size(); ++i)
            offsetArray[i] = offset[i];
    }else
    {
        WARN("Rigid offsetmatrix missing!");
        return false;
    }
    
    bool stacking = false;
    if(d.contains(Consts::ATTR_SKEL_RIGIDSTACKING))
        stacking = true;
    
	node = new RigidJoint(Util::StringUtils::toInts(d.getString(Consts::ATTR_SKEL_JOINTID)).front(), d.getString(Consts::ATTR_NODE_ID), Geometry::Matrix4x4(offsetArray), stacking);
    
	if(d.contains(Consts::ATTR_MATRIX)) {
		vector<float> mat = Util::StringUtils::toFloats(d.getString(Consts::ATTR_MATRIX));
		convertToTNode<RigidJoint>(node)->setBindMatrix(Geometry::Matrix4x4(mat.data()));
	}
    
    if(d.contains(Consts::ATTR_SKEL_INVERSEBINDMATRIX)) {
		vector<float> val = Util::StringUtils::toFloats(d.getString(Consts::ATTR_SKEL_INVERSEBINDMATRIX));
		convertToTNode<JointNode>(node)->setInverseBindMatrix(Geometry::Matrix4x4(val.data()));
	}
    
	if(node == nullptr)
		return false;
    
	ImporterTools::finalizeNode(ctxt,node,d);
	parent->addChild(node);
	return true;
}

static bool importSkeletalAnimationJointNode(ImportContext & ctxt, const std::string & type, const NodeDescription & d, GroupNode * parent) {

	Node * node = nullptr;

	if(type != Consts::NODE_TYPE_SKEL_JOINT)
		return false;

	if(!d.contains(Consts::ATTR_SKEL_JOINTID) || !d.contains(Consts::ATTR_NODE_ID))
		return false;

	node = new JointNode(Util::StringUtils::toInts(d.getString(Consts::ATTR_SKEL_JOINTID)).front(), d.getString(Consts::ATTR_NODE_ID));

	if(d.contains(Consts::ATTR_MATRIX)) {
		vector<float> mat = Util::StringUtils::toFloats(d.getString(Consts::ATTR_MATRIX));
		convertToTNode<JointNode>(node)->setBindMatrix(Geometry::Matrix4x4(mat.data()));
	}
    
    if(d.contains(Consts::ATTR_SKEL_INVERSEBINDMATRIX)) {
		vector<float> val = Util::StringUtils::toFloats(d.getString(Consts::ATTR_SKEL_INVERSEBINDMATRIX));
		convertToTNode<JointNode>(node)->setInverseBindMatrix(Geometry::Matrix4x4(val.data()));
	}

	if(node == nullptr)
		return false;

	ImporterTools::finalizeNode(ctxt,node,d);
	parent->addChild(node);
	return true;
}

static bool importSkeletalAnimationArmatureNode(ImportContext & ctxt, const std::string & type, const NodeDescription & d, GroupNode * parent) {

	Node * node = nullptr;

	if(type != Consts::NODE_TYPE_SKEL_ARMATURE)
        return false;
    
	node = new ArmatureNode();

	if(node == nullptr)
		return false;

	ImporterTools::finalizeNode(ctxt,node,d);
	parent->addChild(node);
	return true;
}
#endif /* MINSG_EXT_SKELETAL_ANIMATION */

static bool importValuatedRegionNode(ImportContext & ctxt,const std::string & type,const NodeDescription & d, GroupNode * parent) {
	if((type != "region" && /*depreciated*/ type != "classification") || parent == nullptr)
		return false;

	std::vector<float> box = Util::StringUtils::toFloats(d.getString(Consts::ATTR_VALREGION_BOX, "0 0 0 1 1 1"));
	if(box.size() != 6) FAIL();
	std::vector<float> resolution = Util::StringUtils::toFloats(d.getString(Consts::ATTR_VALREGION_RESOLUTION, "1 1 1"));
	if(resolution.size() != 3) FAIL();
	std::vector<float> color = Util::StringUtils::toFloats(d.getString(Consts::ATTR_VALREGION_COLOR, ""));
	int numColors = color.size() / 4;

	auto cn = new ValuatedRegionNode(Geometry::Box(Geometry::Vec3(box[0], box[1], box[2]), box[3], box[4], box[5]),
			Geometry::Vec3i(static_cast<int>(resolution[0]), static_cast<int>(resolution[1]), static_cast<int>(resolution[2])));

	for(int i = 0; i < numColors; i++) {
		cn->addColor(color[i*4+0], color[i*4+1], color[i*4+2], color[i*4+3]);
	}

	cn->setHeightScale(Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_VALREGION_HEIGHT, "1")));

	const std::string valueStr = d.getString(Consts::ATTR_VALREGION_VALUE);
	if(!valueStr.empty()) {
		auto valueList = new Util::GenericAttribute::List;
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
		using namespace VisibilitySubdivision;
		if(d.getString(Consts::ATTR_VALREGION_VALUE_TYPE) == "VisibilityVector") {
			if(valueStr.compare(0, 16, "VisibilityVector") == 0) {
				// Old format
				WARN("Old VisibilityVector format not supported anymore.");
			} else {
				// New format
				static const Util::StringIdentifier attrName("VisibilityVectors");
				Util::GenericAttributeMap * vvMap = dynamic_cast<Util::GenericAttributeMap *>(ctxt.getAttribute(attrName));
				if(vvMap == nullptr) {
					WARN("visibility vectors not available");
				} else {
					const auto * vva = dynamic_cast<const VisibilitySubdivision::VisibilityVectorAttribute *>(vvMap->getValue(valueStr));
					if(vva == nullptr) {
						WARN("VisibilityVectorAttribute not found");
					} else {
						// Create a new VisibilityVectorAttribute here, because the map owns the attribute vva.
						valueList->push_back(new VisibilitySubdivision::VisibilityVectorAttribute(vva->ref()));
					}
				}
			}
		} else {
#endif
			std::deque<float> values;
			Util::StringUtils::extractFloats(valueStr, values);
			for(auto & value : values) {
				valueList->push_back(Util::GenericAttribute::createNumber(value));
			}
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
		}
#endif
		cn->setValue(valueList);
	}

	ImporterTools::finalizeNode(ctxt,cn,d);
	parent->addChild(cn);
	return true;
}

#ifdef MINSG_EXT_WAYPOINTS
static bool importPathNode(ImportContext & ctxt, const std::string & type, const NodeDescription & d, GroupNode * parent) {
	if(type != Consts::NODE_TYPE_PATH || parent == nullptr) {
		return false;
	}

	
	auto path = new PathNode;
	path->setLooping(Util::StringUtils::toBool(d.getString(Consts::ATTR_PATHNODE_LOOPING, "false")));

	const NodeDescriptionList * subDescriptions = dynamic_cast<const NodeDescriptionList *>(d.getValue(Consts::CHILDREN));
	const auto data = ImporterTools::filterElements(Consts::TYPE_DATA, subDescriptions);

	for(const auto & waypointDesc : data) {
		if(waypointDesc->getString(Consts::ATTR_DATA_TYPE) != "waypoint") {
			WARN(std::string("Unknown data type \"") + waypointDesc->getString(Consts::ATTR_DATA_TYPE) + "\" for PathNode.");
			delete path;
			return false;
		}
		float time = Util::StringUtils::toNumber<float>(waypointDesc->getString(Consts::ATTR_WAYPOINT_TIME, "0"));
		Geometry::SRT srt = ImporterTools::getSRT(*waypointDesc);
		Waypoint * wp = path->createWaypoint(srt, time);

		const NodeDescriptionList * waypointSubDescriptions = dynamic_cast<const NodeDescriptionList *>(waypointDesc->getValue(Consts::CHILDREN));
		ImporterTools::addAttributes(ctxt, waypointSubDescriptions, wp);
	}

	ImporterTools::finalizeNode(ctxt, path, d);
	parent->addChild(path);
	return true;
}
#endif

//! template for new importers
// static bool importXY(ImportContext & ctxt, const std::string & nodeType, const NodeDescription & d, GroupNode * parent) {
//  if(nodeType != Consts::NODE_TYPE_XY) // check parent != nullptr is done by SceneManager
//      return false;
//
//  XY * node = new XY;
//
//  //TODO
//
//
//  ImporterTools::finalizeNode(ctxt, node, d);
//  parent->addChild(node);
//  return true;
// }

void initExtNodeImporter(SceneManager & sm) {

	sm.addNodeImporter(&importBillboardNode);
	sm.addNodeImporter(&importGenericMetaNode);
	sm.addNodeImporter(&importValuatedRegionNode);

#ifdef MINSG_EXT_MULTIALGORENDERING
	sm.addNodeImporter(&importMultiAlgoGroupNode);
#endif

#ifdef MINSG_EXT_PARTICLE
	sm.addNodeImporter(&importParticleSystemNode);
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	sm.addNodeImporter(&importSkeletalAnimationSkeletalNode);
	sm.addNodeImporter(&importSkeletalAnimationJointNode);
	sm.addNodeImporter(&importSkeletalAnimationArmatureNode);
    sm.addNodeImporter(&importSkeletalAnimationRigidJoint);
#endif

#ifdef MINSG_EXT_WAYPOINTS
	sm.addNodeImporter(&importPathNode);
#endif
}

}
}
