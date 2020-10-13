/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_ALPHATESTSTATE_H
#define MINSG_ALPHATESTSTATE_H

#include "RenderingParametersState.h"
#include <Rendering/RenderingContext/RenderingParameters.h>

namespace MinSG {
	
//! @ingroup states
class AlphaTestState: public RenderingParametersState<Rendering::AlphaTestParameters> {
		PROVIDES_TYPE_NAME(AlphaTestState)
	public:
		AlphaTestState() :
			RenderingParametersState<Rendering::AlphaTestParameters>() {}
		explicit AlphaTestState(const Rendering::AlphaTestParameters & newParameter) :
			RenderingParametersState<Rendering::AlphaTestParameters>(newParameter) {}
		AlphaTestState(const AlphaTestState & other) :
			RenderingParametersState<Rendering::AlphaTestParameters>(other) {}
		virtual ~AlphaTestState() {}

		AlphaTestState * clone() const override{
			return new AlphaTestState(*this);
		}

	private:
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};

}

#endif /* MINSG_ALPHATESTSTATE_H */
