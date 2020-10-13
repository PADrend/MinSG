/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SkyboxState_H
#define SkyboxState_H

#include "../../Core/States/State.h"
#include <Geometry/Definitions.h>
#include <Rendering/Texture/Texture.h>

namespace MinSG {

/**
 *  [SkyboxState] ---|> [State]
 * @ingroup states
 */
class SkyboxState : public State {
	PROVIDES_TYPE_NAME(SkyboxState)
	public:
		MINSGAPI static SkyboxState * createSkybox(const std::string & filename);

		MINSGAPI SkyboxState();
		MINSGAPI SkyboxState(const SkyboxState & source);
		virtual ~SkyboxState() {}

		MINSGAPI void setTexture(Geometry::side_t side, Rendering::Texture * texture);

		/**
		 * Specify the side length of the cube.
		 *
		 * @param size New side length.
		 */
		void setSize(float size) {
			sideLength = size;
		}

		// ---|> [State]
		MINSGAPI SkyboxState * clone() const override;

	private:
		Util::Reference<Rendering::Texture> textures[6];
		float sideLength;

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;
};

}

#endif // SkyboxState_H
