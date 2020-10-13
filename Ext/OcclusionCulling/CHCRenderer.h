/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012,2015 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef CHCRENDERER_H
#define CHCRENDERER_H

#include "../../Core/States/State.h"

namespace MinSG {

/*! CHC Occlusion Culling Algorithm.
	Based on:
		Bittner, J.; Wimmer, M.; Piringer, H. & Purgathofer, W.; 
		"Coherent Hierarchical Culling: Hardware Occlusion Queries Made Useful"; 
		Computer Graphics Forum, Blackwell Publishing, Inc, 2004, 23, 615-624
	
	Minor adaptations to original algorithms and bug fix (don't render nodes twice)
	@ingroup states
*/
class CHCRenderer : public State {
		PROVIDES_TYPE_NAME(CHCRenderer)
	public:

		CHCRenderer():State(),debugShowVisible(false),frameNr(1){}
		virtual ~CHCRenderer(){}

		bool getDebugShowVisible()const				{	return debugShowVisible;	}
		void setDebugShowVisible(bool b)			{	debugShowVisible = b;	}
		CHCRenderer * clone() const override	{	return new CHCRenderer;	}

	private:
		bool debugShowVisible;
		int frameNr;

		MINSGAPI State::stateResult_t performCulling(FrameContext & context,Node * rootNode, const RenderParam & rp);
		MINSGAPI State::stateResult_t showVisible(FrameContext & context,Node * rootNode, const RenderParam & rp);

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;

};
}

#endif // CHCRENDERER_H
