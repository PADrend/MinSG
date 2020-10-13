/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_BLENDINGSTATE_H
#define MINSG_BLENDINGSTATE_H

#include "RenderingParametersState.h"
#include <Rendering/RenderingContext/RenderingParameters.h>

namespace MinSG {

/**
 *  [BlendingState] ---|> [State]
 * @ingroup states
 */
class BlendingState : public RenderingParametersState<Rendering::BlendingParameters> {
		PROVIDES_TYPE_NAME(BlendingState)

		//!	@name Main
		//@{
	public:
		MINSGAPI BlendingState();
		explicit BlendingState(const Rendering::BlendingParameters & newParameter) :
			RenderingParametersState<Rendering::BlendingParameters>(newParameter), depthWritesEnabled(true) {}
		BlendingState(const BlendingState & other) :
			RenderingParametersState<Rendering::BlendingParameters>(other), depthWritesEnabled(other.depthWritesEnabled) {}
		virtual ~BlendingState() {}

		BlendingState * clone() const override{
			return new BlendingState(*this);
		}
		//@}

		// ------

		//!	@name Type:BLENDING
		//@{
	private:
		bool depthWritesEnabled;

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;

	public:
		bool getBlendDepthMask() const				{	return depthWritesEnabled;	}
		void setBlendDepthMask(bool b)				{	depthWritesEnabled = b;	}
		//@}

};
}
#endif // MINSG_BLENDINGSTATE_H
