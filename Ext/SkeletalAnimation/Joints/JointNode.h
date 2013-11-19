/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef JointNode_H_
#define JointNode_H_

#include <map>
#include "AbstractJoint.h"

namespace MinSG {

    /*
     *  @brief representation of an abstract joint within a skeleton.
     *
     *  Provides abstract organization of joints. Specially interconnection with
     *  animationdata.
     *
     *  JointNode ---|> AbstractJoint ---|> ListNode ---|> GroupNode ---|> Node
     *
     */
	class JointNode : public AbstractJoint
	{
		PROVIDES_TYPE_NAME(JointNode)

	private:
		std::map<int, float> weights;

	public:
		JointNode() : AbstractJoint() { }
		JointNode(uint32_t _id, std::string _name);
        

		void generateJointNodeMap(std::unordered_map<std::string, AbstractJoint *> &jMap) override;
        
        void catchJointNodes(std::deque<JointNode *> *nodes);
        
//        /****************************************************************
//         *                  ---|> Node
//         ****************************************************************/
//		JointNode * clone()const;
//		JointNode * clone(std::unordered_map<std::string, AbstractJoint *> &jMap);
        
        // bounding box
	protected:
		JointNode(const JointNode &source);
	private:
		JointNode * doClone()const override		{	return new JointNode(*this);	}
		const Geometry::Box& doGetBB() const override;
		
	};

}

#endif /* JointNode_H_ */
#endif /* MINSG_EXT_SKELETAL_ANIMATION */
