/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_CULLFACESTATE_H
#define MINSG_CULLFACESTATE_H

#include "RenderingParametersState.h"
#include <Rendering/RenderingContext/RenderingParameters.h>

namespace MinSG {
	
//! @ingroup states
class CullFaceState: public RenderingParametersState<Rendering::CullFaceParameters> {
		PROVIDES_TYPE_NAME(CullFaceState)
	public:
		CullFaceState() :
			RenderingParametersState<Rendering::CullFaceParameters>() {
			changeParameters().enable();
		}
		explicit CullFaceState(const Rendering::CullFaceParameters & newParameter) :
			RenderingParametersState<Rendering::CullFaceParameters>(newParameter) {}
		CullFaceState(const CullFaceState & other) :
			RenderingParametersState<Rendering::CullFaceParameters>(other) {}
		virtual ~CullFaceState() {}

		CullFaceState * clone() const override{
			return new CullFaceState(*this);
		}

	private:
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};

}

#endif /* MINSG_CULLFACESTATE_H */
