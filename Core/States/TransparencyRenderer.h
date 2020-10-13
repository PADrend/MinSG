/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef __TransparencyRenderer_H
#define __TransparencyRenderer_H

#include "NodeRendererState.h"
#include "../../Helper/DistanceSorting.h"

#include <queue>

namespace MinSG {
class FrameContext;
enum class NodeRendererResult : bool;

/**
 *  [TransparencyRenderer] ---|> [NodeRendererState]
 * @ingroup states
 */
class TransparencyRenderer : public NodeRendererState {
	PROVIDES_TYPE_NAME(TransparencyRenderer)
	private:
		//! @name Main
		//@{
		std::unique_ptr<DistanceSetB2F<Node>> nodes;

		//! Flag to toggle usage of premultiplied alpha.
		bool usePremultipliedAlpha;

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node *, const RenderParam & rp) override;

		MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		MINSGAPI TransparencyRenderer(const TransparencyRenderer &);
	public:
		MINSGAPI TransparencyRenderer();
		MINSGAPI virtual ~TransparencyRenderer();

		MINSGAPI void addNode(Node * n);

		/// ---|> [State]
		MINSGAPI TransparencyRenderer * clone() const override;

		/**
		 * Specify the usage of premultiplied-alpha colors for blending.
		 *
		 * @param usePMA Specify the new status.
		 */
		void setUsePremultipliedAlpha(bool usePMA) {
			usePremultipliedAlpha = usePMA;
		}


		/**
		 * Check the usage of premultiplied-alpha colors for blending.
		 *
		 * @return Current usage status.
		 */
		bool getUsePremultipliedAlpha() const {
			return usePremultipliedAlpha;
		}
		//@}

};
}
#endif // TransparencyRenderer_H
