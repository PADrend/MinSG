/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef PADrend_ArmatureNode_h
#define PADrend_ArmatureNode_h

namespace Geometry {
    template<typename _T> class _Matrix4x4;
    typedef _Matrix4x4<float> Matrix4x4;
}

namespace MinSG {
    class JointNode;
}

#include "AbstractJoint.h"

/*
 *  @brief this class representes the root of the joint structure.
 *
 *  All joints root world matrix starting with the identity matrix.
 *  The container is so called armature. All movement is done within the geometry
 *  and all deforming or manipulation can be done by the weight and the transformation
 *  of the joints.
 *  An transformation of the armature cause a transformation of the vertices twice !!!!
 *  First by the transformation of the geometry and second by the transformation of the
 *  root joint.
 *
 *  ArmatureNode ---|> AbstractJoint ---|> ListNode ---|> GroupNode ---|> Node
 * @ingroup nodes
 *
 */
namespace MinSG {
	class ArmatureNode : public AbstractJoint
	{
		 PROVIDES_TYPE_NAME(ArmatureNode)

	private:
		Geometry::Matrix4x4 identityMatrix;

	public:
		ArmatureNode();
		virtual ~ArmatureNode() {}

		void generateJointNodeMap(std::unordered_map<std::string, AbstractJoint *> &jMap) override;
        void catchJointNodes(std::deque<JointNode *> *nodes);

	private:
		ArmatureNode(const ArmatureNode&) = default;
		ArmatureNode * doClone()const override { return new ArmatureNode(*this);	}
//		ArmatureNode *clone(std::unordered_map<std::string, AbstractJoint *> &jMap);
	};
}


#endif
#endif
