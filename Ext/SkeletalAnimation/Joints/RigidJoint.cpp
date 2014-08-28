/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION


#include <iostream>

#include "RigidJoint.h"

using namespace Geometry;

namespace MinSG {

RigidJoint::RigidJoint(JointNode *source) : JointNode(source->getId(), source->getName())
{
    init(Matrix4x4(), false);
    setRelTransformation(source->getRelTransformationMatrix());
    setBindMatrix(*source->getBindMatrix());
}

RigidJoint::RigidJoint(uint32_t _id, std::string _name) : JointNode(_id, _name)
{
    init(Matrix4x4(), false);
}

RigidJoint::RigidJoint(uint32_t _id, std::string _name, Geometry::Matrix4x4 _offsetMatrix, bool _stacking) : JointNode(_id, _name)
{
    init(_offsetMatrix, _stacking);
}

void RigidJoint::init(Geometry::Matrix4x4 _offsetMatrix, bool _stacking)
{
    offsetMatrix = _offsetMatrix;
    stacking = _stacking;
}

//RigidJoint * RigidJoint::clone()const
//{
//    RigidJoint *myclone = new RigidJoint(getId(), getName(), offsetMatrix, stacking);
//    myclone->setMatrix(getRelTransformationMatrix());
//    myclone->setBindMatrix(*getBindMatrix());
//    
//    return myclone;
//}
//
//RigidJoint * RigidJoint::clone(std::unordered_map<std::string, AbstractJoint *> &jMap)
//{
//    RigidJoint *myclone = new RigidJoint(getId(), getName(), offsetMatrix, stacking);
//    
//    jMap[myclone->getName()] = myclone;
//    
//    for (uint32_t i=0; i<countChildren(); ++i)
//    {
//        AbstractJoint *jointNode = (dynamic_cast<AbstractJoint *> (getChild(i)))->clone(jMap);
//        myclone->addChild(jointNode);
//    }
//    
//    return myclone;
//}

void RigidJoint::doAddChild(Util::Reference<Node> _child)
{
    Matrix4x4 childPositionOffset = offsetMatrix;
    if(stacking)
        for(uint32_t i=0; i<countChildren(); ++i)
            childPositionOffset.translate(0.0, 0.0, getChild(i)->getBB().getMaxZ());
    
    _child.get()->setRelTransformation(_child.get()->getRelTransformationMatrix() * childPositionOffset);
    inverseChildMatrices[_child] = childPositionOffset.inverse();
    
    ListNode::doAddChild(_child);
}

bool RigidJoint::doRemoveChild(Util::Reference<Node> _childToRemove)
{      
    for(auto child : inverseChildMatrices)
        if(child.first.get() == _childToRemove.get())
            _childToRemove.get()->setRelTransformation(_childToRemove.get()->getRelTransformationMatrix() * child.second);
        
    return ListNode::doRemoveChild(_childToRemove);
}

}

#endif
