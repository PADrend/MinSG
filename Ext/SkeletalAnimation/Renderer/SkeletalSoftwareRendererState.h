/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef __PADrendComplete__SkeletalSoftwareRendererState__
#define __PADrendComplete__SkeletalSoftwareRendererState__

#include "SkeletalAbstractRendererState.h"
#include "../Util/SkeletalAnimationUtils.h"

#include <vector>

namespace Geometry {
    template<typename _T> class _Vec4;
    typedef _Vec4<float> Vec4;
}

namespace MinSG {
    class GeometryNode;
    
    //! @ingroup states
    class SkeletalSoftwareRendererState : public SkeletalAbstractRendererState {
        PROVIDES_TYPE_NAME(SkeletalSoftwareRendererState)
    private:
        struct VertexPair {
            uint32_t vId;
            std::vector<float> weights;
            std::vector<uint32_t> jointIds;
            Geometry::Vec4 bindPosition;
        };
        
        typedef std::pair<GeometryNode*, std::vector<VertexPair> > MeshSkin_t;
        std::vector<MeshSkin_t> meshSkins;
        
    public:
        SkeletalSoftwareRendererState();
        SkeletalSoftwareRendererState(const SkeletalSoftwareRendererState &source);
        virtual ~SkeletalSoftwareRendererState() {}
        
        /// ---|> [SkeletalAbstractRendererState]
        virtual void validateMatriceOrder(Node *node) override;
        
        /// ---|> [State]
        stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		SkeletalSoftwareRendererState *clone()const override;
    };
}

#endif /* defined(__PADrendComplete__SkeletalSoftwareRendererState__) */
#endif
