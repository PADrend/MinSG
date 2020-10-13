/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_SPHEREVISUALIZATIONRENDERER_H
#define MINSG_SVS_SPHEREVISUALIZATIONRENDERER_H

#include "../../Core/States/NodeRendererState.h"
#include <Util/TypeNameMacro.h>

namespace MinSG {
class FrameContext;
class Node;
class RenderParam;
namespace SVS {

/**
 * Renderer that uses preprocessed visibility information.
 * This information has to be attached to the nodes that are to be rendered.
 * By using this information, occlusion culling is performed.
 *
 * @author Benjamin Eikel
 * @date 2013-11-28
 */
class SphereVisualizationRenderer : public NodeRendererState {
		PROVIDES_TYPE_NAME(SVS::SphereVisualizationRenderer)
	private:
		MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

	public:
		MINSGAPI SphereVisualizationRenderer();

		MINSGAPI SphereVisualizationRenderer * clone() const override;
};

}
}

#endif /* MINSG_SVS_SPHEREVISUALIZATIONRENDERER_H */

#endif /* MINSG_EXT_SVS */
