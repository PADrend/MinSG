/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_RENDERINGPARAMETERSSTATE_H
#define MINSG_RENDERINGPARAMETERSSTATE_H

#include "State.h"

namespace MinSG {
class FrameContext;
class Node;
class RenderParam;

/**
 * @brief Base class for states that change rendering parameters
 *
 * Abstract base class for states that hold rendering parameters.
 * These states change the rendering parameters when enabled, and revert the change when disabled.
 * @author Benjamin Eikel
 * @date 2012-02-21
 */
template<typename RenderingParameters>
class RenderingParametersState : public State {
	private:
		RenderingParameters parameters;

	public:
		RenderingParametersState() : State(), parameters() {
		}
		explicit RenderingParametersState(RenderingParameters  newParameters) : State(), parameters(std::move(newParameters)) {
		}
		RenderingParametersState(const RenderingParametersState & other) : State(other), parameters(other.parameters) {
		}
		virtual ~RenderingParametersState() {
		}

		RenderingParameters & changeParameters() {
			return parameters;
		}
		const RenderingParameters & getParameters() const {
			return parameters;
		}
		void setParameters(const RenderingParameters & newParameters) {
			parameters = newParameters;
		}
};

}

#endif /* MINSG_RENDERINGPARAMETERSSTATE_H */
