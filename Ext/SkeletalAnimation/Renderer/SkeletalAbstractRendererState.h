/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef __PADrendComplete__SkeletalAbstractRendererState__
#define __PADrendComplete__SkeletalAbstractRendererState__

#include "../../../Core/States/ShaderState.h"

#include <Geometry/Matrix4x4.h>

#include <vector>

namespace Util {
    class FileName;
}

namespace MinSG {
    class Node;
    class AbstractJoint;
    class SkeletalNode;
}

namespace MinSG {
    class SkeletalAbstractRendererState : public ShaderState {
    protected:
        bool validatedMatrices;
        
        std::vector<AbstractJoint *> matriceOrder;
        
        mutable SkeletalNode *rootJoint;
        
        uint32_t jointSize;
        
		mutable Geometry::Matrix4x4 bindMatrix;
        
        mutable std::vector<Geometry::Matrix4x4> jointMats;
		mutable std::vector<Geometry::Matrix4x4> inverseMatContainer;
        
		int debugJointId;
        
        SkeletalAbstractRendererState();
        SkeletalAbstractRendererState(const SkeletalAbstractRendererState &source);
        
    public:
        virtual ~SkeletalAbstractRendererState() {}
        
        /****************************************************************
         * Bind matrix for transforming geometry from model space
         * into skeleton space.
         ****************************************************************/
		void setBindMatrix(std::vector<float> _matrix);
		void setBindMatrix(const float _matrix[]);
		void setBindMatrix(Geometry::Matrix4x4 _matrix);
		const Geometry::Matrix4x4 &getBindMatrix() const { return bindMatrix; }
        
        /****************************************************************
         * validates joint matrice order with vertex attribute jointId.
         ****************************************************************/
        virtual void validateMatriceOrder(Node *node);
        
        /****************************************************************
         * Renders vertices with a color describing the influence of joint
         * blue -> no influence -> 0
         * red -> full influence -> 1
         ****************************************************************/
		void setDebugJointId(int _id) { debugJointId = _id; setUniform(Rendering::Uniform("debugJointId", debugJointId)); }
		int getDebugJointId() const { return debugJointId; }

    };
}

#endif /* defined(__PADrendComplete__SkeletalAbstractRendererState__) */
#endif
