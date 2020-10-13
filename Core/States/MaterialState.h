/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_MATERIALSTATESTATE_H
#define MINSG_MATERIALSTATESTATE_H

#include "RenderingParametersState.h"
#include <Rendering/RenderingContext/RenderingParameters.h>

namespace MinSG {
	
//! @ingroup states
class MaterialState: public RenderingParametersState<Rendering::MaterialParameters> {
		PROVIDES_TYPE_NAME(MaterialState)
	public:
		MaterialState() :
			RenderingParametersState<Rendering::MaterialParameters>() {}
		explicit MaterialState(const Rendering::MaterialParameters & newParameter) :
			RenderingParametersState<Rendering::MaterialParameters>(newParameter) {}
		MaterialState(const MaterialState & other) :
			RenderingParametersState<Rendering::MaterialParameters>(other) {}
		virtual ~MaterialState() {}

		MaterialState * clone() const override{
			return new MaterialState(*this);
		}

		/**
		 * Multiply the color values of the material with the alpha value.
		 * This is needed for alpha blending when using premultiplied-alpha colors.
		 */
		MINSGAPI void preMultiplyAlpha();

	private:
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};

}

#endif /* MINSG_MATERIALSTATESTATE_H */
