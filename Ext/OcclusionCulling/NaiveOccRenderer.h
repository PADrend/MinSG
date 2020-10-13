/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012,2015 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef NAIVEOCCRENDERER_H
#define NAIVEOCCRENDERER_H

#include "../../Core/States/State.h"

namespace MinSG {

/*!
	This Occlusion Culling Algorithm implements the naive "Hierarchical stop-and-wait method" 
	as an illustration of the effect of active waiting for occlusion culling query results.
	This algorithm is also used by Bittner et al. in the Paper "Coherent Hierarchical Culling:
	Hardware Occlusion Queries Made Useful" (2004) as motivation for the necessity of more 
	sophisticated query handling in practical algorithms.
	@ingroup states
*/
class NaiveOccRenderer : public State {
		PROVIDES_TYPE_NAME(NaiveOccRenderer)
	public:

		NaiveOccRenderer():State(),debugShowVisible(false){}
		virtual ~NaiveOccRenderer(){}

		bool getDebugShowVisible()const				{	return debugShowVisible;	}
		void setDebugShowVisible(bool b)			{	debugShowVisible = b;	}
		NaiveOccRenderer * clone() const override	{	return new NaiveOccRenderer;	}

	private:
		bool debugShowVisible;

		MINSGAPI State::stateResult_t performCulling(FrameContext & context,Node * rootNode, const RenderParam & rp);
		MINSGAPI State::stateResult_t showVisible(FrameContext & context,Node * rootNode, const RenderParam & rp);

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;

};
}

#endif // NAIVEOCCRENDERER_H
