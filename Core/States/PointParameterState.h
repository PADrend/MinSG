/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_POINTPARAMETERSTATE_H
#define MINSG_POINTPARAMETERSTATE_H

#include "RenderingParametersState.h"
#include <Rendering/RenderingContext/RenderingParameters.h>

namespace MinSG {
	
//! @ingroup states
class PointParameterState: public RenderingParametersState<Rendering::PointParameters> {
		PROVIDES_TYPE_NAME(PointParameterState)
	public:
		PointParameterState() :
			RenderingParametersState<Rendering::PointParameters>() {}
		explicit PointParameterState(const Rendering::PointParameters & newParameter) :
			RenderingParametersState<Rendering::PointParameters>(newParameter) {}
		PointParameterState(const PointParameterState & other) :
			RenderingParametersState<Rendering::PointParameters>(other) {}
		virtual ~PointParameterState() {}

		PointParameterState * clone() const override{
			return new PointParameterState(*this);
		}

	private:
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};

}

#endif /* MINSG_POINTPARAMETERSTATE_H */
