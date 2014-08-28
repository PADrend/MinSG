/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "JointNode.h"

using namespace std;

namespace MinSG {

	JointNode::JointNode(uint32_t _id, std::string _name) : AbstractJoint(_id, _name) { }
    
    JointNode::JointNode(const JointNode &source) : AbstractJoint(source) {
        setRelTransformation(source.getRelTransformationMatrix());
        setBindMatrix(*source.getBindMatrix());
    }

//	JointNode * JointNode::clone()const
//	{
//        return new JointNode(*this);
//	}
//
//	JointNode *JointNode::clone(unordered_map<string, AbstractJoint *> &jMap)
//	{
//		JointNode *myclone = clone();
//
//		jMap[myclone->getName()] = myclone;
//
//		for (uint32_t i=0; i<countChildren(); ++i)
//		{
//			AbstractJoint *jointNode = (dynamic_cast<AbstractJoint *> (getChild(i)))->clone(jMap);
//			myclone->addChild(jointNode);
//		}
//         
//		return myclone;
//	}
    
    const Geometry::Box& JointNode::doGetBB() const {
        static const Geometry::Box box;
        return box;
    }
        
    void JointNode::catchJointNodes(std::deque<JointNode *> *nodes)
    {
        nodes->emplace_back(this);
        
        for(uint32_t i=0; i<countChildren(); ++i)
		{
			JointNode *jointNode = dynamic_cast<JointNode *>(getChild(i));
			if(jointNode != nullptr)
				jointNode->catchJointNodes(nodes);
		}
    }

	void JointNode::generateJointNodeMap(unordered_map<string, AbstractJoint *> &jMap)
	{
		jMap[getName()] = this;

		for(uint32_t i=0; i<countChildren(); ++i)
		{
			JointNode *jointNode = dynamic_cast<JointNode *>(getChild(i));
			if(jointNode != nullptr)
				jointNode->generateJointNodeMap(jMap);
		}
	}
}

#endif /* MINSG_EXT_SKELETAL_ANIMATION */
