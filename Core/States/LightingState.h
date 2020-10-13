/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef LIGHTINGSTATE_H_
#define LIGHTINGSTATE_H_

#include "State.h"
#include "../Nodes/LightNode.h"
#include <Util/References.h>

namespace MinSG {

/**
 * State, that illuminates the node it is attached to.
 * It serves as a wrapper for an associated light.
 * When this state is (de-)activated, it (de-)activates the light.
 *
 * @author Benjamin Eikel
 * @date 2010-05-25
 * @ingroup states
 */
class LightingState : public State {
	PROVIDES_TYPE_NAME(LightingState)
		bool originalState, enableLight;
	public:
		MINSGAPI LightingState();
		MINSGAPI LightingState(LightNode * newLight);
		MINSGAPI LightingState(const LightingState & source);

		LightNode * getLight() const 		{			return light.get();	}
		void setLight(LightNode * newLight)	{			light = newLight;	}
		MINSGAPI LightingState * clone() const override;
		
		bool getEnableLight() const			{			return enableLight;	}
		void setEnableLight(bool b)			{			enableLight = b;	}

	private:
		Util::Reference<LightNode> light;

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};

}

#endif /* LIGHTINGSTATE_H_ */
