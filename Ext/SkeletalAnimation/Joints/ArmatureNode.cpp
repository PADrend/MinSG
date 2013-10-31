/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include <Geometry/Matrix4x4.h>

#include "ArmatureNode.h"
#include "JointNode.h"

namespace MinSG {

ArmatureNode::ArmatureNode() : AbstractJoint()
{
	identityMatrix = Geometry::Matrix4x4();
	setName("Armature");
}

//ArmatureNode * ArmatureNode::clone()const
//{
//    auto myclone = new ArmatureNode();
//    for(uint32_t i=0; i<countChildren(); ++i)
//        myclone->addChild(getChild(i)->clone());
//    
//	return myclone;
//}

void ArmatureNode::catchJointNodes(std::deque<JointNode *> *nodes)
{
    for(uint32_t i=0; i<countChildren(); ++i)
    {
        JointNode *node = dynamic_cast<JointNode *>(getChild(i));
        if(node != nullptr)
            node->catchJointNodes(nodes);
    }
}

//ArmatureNode *ArmatureNode::clone(std::unordered_map<std::string, AbstractJoint *> &jMap)
//{
//	auto myclone = new ArmatureNode();
//    
//    for(uint32_t i=0; i<countChildren(); ++i)
//    {
//        AbstractJoint *child = dynamic_cast<AbstractJoint *>(getChild(i));
//        if(child != nullptr)
//            myclone->addChild(child->clone(jMap));
//        else
//            myclone->addChild(getChild(i)->clone());
//    }
//    
//    return myclone;
//}

void ArmatureNode::generateJointNodeMap(std::unordered_map<std::string, AbstractJoint *> &jMap)
{
	for(uint32_t i=0; i<countChildren(); ++i)
	{
        JointNode *joint = dynamic_cast<JointNode *>(getChild(i));
		if(joint != nullptr)
		   joint->generateJointNodeMap(jMap);
	}
}

}

#endif
