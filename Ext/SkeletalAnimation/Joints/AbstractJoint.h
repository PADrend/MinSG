/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef PADrend_AbstractJoint_h
#define PADrend_AbstractJoint_h

#include <Geometry/Matrix4x4.h>

#include "../../../Core/Nodes/ListNode.h"

#include <map>

namespace MinSG {
    class GeometryNode;
}

namespace MinSG {
    /*
     *  @brief representation of an abstract joint within a skeleton.
     *
     *  Provides abstract organization of joints. Specially interconnection with
     *  animationdata. 
     *
     *  AbstractJoint ---|> ListNode ---|> GroupNode ---|> Node
     *
     */
	class AbstractJoint : public ListNode
	{
	private:        
        mutable Geometry::Matrix4x4 bindMatrix;

	protected:
		mutable uint32_t id;
        mutable std::string name;
		mutable Geometry::Matrix4x4 invBindMatrix;

		AbstractJoint();
        AbstractJoint(uint32_t _id, std::string _name);
        AbstractJoint(const AbstractJoint &source);

	public:
        /****************************************************************
         * id is using for identify joint inside skeleton, 
         * name for displaying only. Name is not unique unlike id. 
         ****************************************************************/
		uint32_t getId() const { return id; }
        
        void setName(std::string _name) { name = _name; }
		std::string getName() const { return name; }
        
        /****************************************************************
         * Bind matrix brings mesh from mesh space into skeleton space
         ****************************************************************/
        void setBindMatrix(Geometry::Matrix4x4 _bindMatrix);
        const Geometry::Matrix4x4 *getBindMatrix() const { return &bindMatrix; }
        
        const Geometry::Matrix4x4 & getInverseBindMatrix() const { return invBindMatrix; }
        void setInverseBindMatrix(Geometry::Matrix4x4 _invBind) { invBindMatrix = _invBind; }

        /****************************************************************
         * Generates map for connecting with animation data. 
         ****************************************************************/
		virtual void generateJointNodeMap(std::unordered_map<std::string, AbstractJoint *> &jMap) = 0;
        
        /****************************************************************
         *                  ---|> GroupNode
         ****************************************************************/
        virtual void doAddChild(Util::Reference<Node> child) override;

//        /****************************************************************
//         *                  ---|> Node
//         ****************************************************************/
//		virtual AbstractJoint * clone()const = 0;
//		virtual AbstractJoint * clone(std::unordered_map<std::string, AbstractJoint *> &jMap) = 0;
//        virtual NodeVisitor::status traverse(NodeVisitor & visitor);
        
        // do not visit children, in an armature structure there are not states and geometry!
        void doDisplay(FrameContext & /*context*/,const RenderParam & /*rp*/) override { }
	};
}


#endif
#endif
