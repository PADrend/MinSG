/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the 
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef PADrend_RigidJoint_h
#define PADrend_RigidJoint_h

#include "JointNode.h"

#include <Geometry/Matrix4x4.h>

#include <map>



namespace MinSG {
    
    /*
     *  @brief physical represantation of an joint node.
     *
     *  This is an pre alpha early adopation sergeant version. 
     *  Mainly an entrie point for physical interaction within one
     *  skeleton. 
     *
     *  It has been introduced to allow a skeleton to interact with
     *  other objects within the scene. 
     *
     *  Currently objects can be stacked on a skeleton with a given offset matrix.
     *
     *  RigidJoint ---|> JointNode ---|> AbstractJoint ---|> ListNode ---|> GroupNode ---|> Node
     *
     */
    class RigidJoint : public JointNode 
    {
        PROVIDES_TYPE_NAME(RigidJoint)
    
    private:        
        // all children will be multiplied by this matrix when joining.
        mutable Geometry::Matrix4x4 offsetMatrix;
        
        // stores inverse matrix for removing child to his old position when removing
        std::map<Util::Reference<Node>, Geometry::Matrix4x4> inverseChildMatrices;
        
        // if object have to be stacked or not.
        bool stacking;
        
        void init(Geometry::Matrix4x4 _offsetMatrix, bool _stacking);
        
    public:
        RigidJoint(JointNode *source);
        
        RigidJoint(uint32_t _id, std::string _name);
        RigidJoint(uint32_t _id, std::string _name, Geometry::Matrix4x4 _offsetMatrix, bool _stacking=false);
        
        /****************************************************************
         *                  some getters and setters
         ****************************************************************/
        void setOffsetMatrix(Geometry::Matrix4x4 _m) { offsetMatrix = Geometry::Matrix4x4(_m); }
        Geometry::Matrix4x4 getOffsetMatrix() const { return offsetMatrix; }
        
        /****************************************************************
         * For stacking objects by their bounding box.
         ****************************************************************/
        void stackObject() { stacking = true; }
        void stopStackingObject() { stacking = false; }
        bool isStacking() { return stacking; }
        
        /****************************************************************
         *                  Object interactions
         ****************************************************************/
         
        // adds an child by first multiplying the child with the offset matrix and if given by all other children (stacking)
        virtual void doAddChild(Util::Reference<Node> _child);
        virtual bool doRemoveChild(Util::Reference<Node> _childToRemove);
        
        
	private:
		RigidJoint(const RigidJoint&) = default;
		RigidJoint* doClone()const override	{	return new RigidJoint(*this);	}
//        /****************************************************************
//         *                  ---|> Node
//         ****************************************************************/
//        
//        RigidJoint * clone()const;
//        RigidJoint * clone(std::unordered_map<std::string, AbstractJoint *> &jMap);
    };
    
}


#endif
#endif

