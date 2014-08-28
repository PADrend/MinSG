/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "AbstractJoint.h"
#include "ArmatureNode.h"

#include "../Util/SkeletalAnimationUtils.h"

#include "../../../Core/Nodes/GeometryNode.h"

#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/Mesh/Mesh.h>
#include <Geometry/Matrix4x4.h>

#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include "../../../Core/FrameContext.h"

using namespace Geometry;

namespace MinSG {

    AbstractJoint::AbstractJoint() : ListNode(), bindMatrix(Geometry::Matrix4x4()), id(0), name(""),
                                    invBindMatrix(Geometry::Matrix4x4()) { }
    
    AbstractJoint::AbstractJoint(uint32_t _id, std::string _name) : ListNode(), bindMatrix(Geometry::Matrix4x4()), id(_id), name(std::move(_name)),
                                    invBindMatrix(Geometry::Matrix4x4()) { }
    
    AbstractJoint::AbstractJoint(const AbstractJoint &source) : ListNode(source), bindMatrix(*source.getBindMatrix()), id(source.getId()),
                                name(source.getName()), invBindMatrix(source.getInverseBindMatrix()) { }

    void AbstractJoint::doAddChild(Util::Reference<Node> child) {
        if(dynamic_cast<AbstractJoint *>(child.get()) == nullptr)
            return;
    
        if(dynamic_cast<ArmatureNode *> (child.get()) != nullptr)
            return;
        
        ListNode::doAddChild(child);
    }
    
//    NodeVisitor::status AbstractJoint::traverse(NodeVisitor & visitor) {
//        NodeVisitor::status status = visitor.enter(this);
//        return status == NodeVisitor::EXIT_TRAVERSAL ? status : visitor.leave(this);
//    }
    
    void AbstractJoint::setBindMatrix(Geometry::Matrix4x4 _bindMatrix) {
        bindMatrix = Geometry::Matrix4x4(_bindMatrix);
        setRelTransformation(_bindMatrix);
    }

}

#endif
