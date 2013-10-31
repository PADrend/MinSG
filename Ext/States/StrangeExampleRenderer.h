/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef STRANGEEXAMPLERENDERER_H
#define STRANGEEXAMPLERENDERER_H

#include "../../Core/States/State.h"

namespace MinSG {

/**
 *  [StrangeExampleRenderer] ---|> [State]
 */
class StrangeExampleRenderer : public State {
		PROVIDES_TYPE_NAME(StrangeExampleRenderer)
	private:
		stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;
	public:
		StrangeExampleRenderer();
		virtual ~StrangeExampleRenderer();

		StrangeExampleRenderer * clone() const override;
};
}

#endif // STRANGEEXAMPLERENDERER_H
