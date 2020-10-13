/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef LIGHTNODE_H
#define LIGHTNODE_H

#include "Node.h"
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <Rendering/RenderingContext/RenderingParameters.h>

// Forward declaration
namespace Rendering {
class Mesh;
}

namespace MinSG {

/**
 *    [LightNode] ---|> [Node]
 * @ingroup nodes
 */
class LightNode : public Node {

	PROVIDES_TYPE_NAME(LightNode)
		static const uint8_t INVALID_LIGHT_NUMBER = 255;
	public:


		//!	factories
		MINSGAPI static LightNode * createPointLight();
		MINSGAPI static LightNode * createDirectionalLight();
		MINSGAPI static LightNode * createSpotLight();

		// ----

		MINSGAPI LightNode(Rendering::LightParameters::lightType_t type = Rendering::LightParameters::POINT);
		MINSGAPI virtual ~LightNode();

		Rendering::LightParameters::lightType_t getType() const {
			return parameters.type;
		}
		void setLightType(Rendering::LightParameters::lightType_t type) {
			parameters.type = type;
			removeMetaMesh();
		}


		/// Color parameters
		void setAmbientLightColor(const Util::Color4f & color)	{	parameters.ambient = color;	}
		void setDiffuseLightColor(const Util::Color4f & color)	{	parameters.diffuse = color;	}
		void setSpecularLightColor(const Util::Color4f & color)	{	parameters.specular = color;	}
		const Util::Color4f & getAmbientLightColor() const		{	return parameters.ambient;	}
		const Util::Color4f & getDiffuseLightColor() const		{	return parameters.diffuse;	}
		const Util::Color4f & getSpecularLightColor() const		{	return parameters.specular;	}


		/// Attenuations
		void setConstantAttenuation(float attenuation)			{	parameters.constant = attenuation;	}
		float getConstantAttenuation() const					{	return parameters.constant;	}
		void setLinearAttenuation(float attenuation)			{	parameters.linear = attenuation;	}
		float getLinearAttenuation() const						{	return parameters.linear;	}
		void setQuadraticAttenuation(float attenuation)			{	parameters.quadratic = attenuation;	}
		float getQuadraticAttenuation() const 					{	return parameters.quadratic;	}

		/// SpoLight parameters
		float getCutoff() const									{	return parameters.cutoff;	}
		/* cutoff gets clamped to [0.0f , 90.0f] */
		MINSGAPI void setCutoff(float cutoff);
		void setExponent(float exponent)						{	parameters.exponent = exponent;	}
		float getExponent() const								{	return parameters.exponent;	}

		const Rendering::LightParameters & getParameters() const {
			validateParameters();
			return parameters;
		}

		MINSGAPI void switchOn(FrameContext & context);
		MINSGAPI void switchOff(FrameContext & context);
		bool isSwitchedOn()const								{	return lightNumber != INVALID_LIGHT_NUMBER; }

		/// ---|> [Node]
		MINSGAPI void doDisplay(FrameContext & context, const RenderParam & rp) override;

	private:
		/// ---|> [Node]
		MINSGAPI const Geometry::Box& doGetBB() const override;
		
		MINSGAPI explicit LightNode(const LightNode &);

		/// ---|> [Node]
		LightNode * doClone() const override	{	return new LightNode(*this);	}


		//! Invalidate the meta mesh. This should be called when parameters have changed and the mesh should be recreated.
		MINSGAPI void removeMetaMesh();

		mutable Rendering::LightParameters parameters;

		//! Temporary storage for the light number, when the light is enabled between @a activate and @a deactivate.
		uint8_t lightNumber;

		MINSGAPI void validateParameters()const;

		//! Reference for a Mesh that is used for displaying meta data.
		Util::Reference<Rendering::Mesh> metaMesh;

		//! Function that is called by the display function for creating a new mesh when the reference is invalid.
		MINSGAPI Rendering::Mesh * createMetaMesh();
};
}

#endif // LIGHTNODE_H
