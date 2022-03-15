/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef IBLEnvironmentState_H
#define IBLEnvironmentState_H

#include "../../Core/States/State.h"
#include <Util/IO/FileName.h>
#include <Geometry/Convert.h>

namespace Rendering {
class Texture;
class Shader;
} // Rendering

namespace MinSG {

/**
 *  A State for displaying an environment at great distance around the observer's
 *  position (like a skydome).
 *  When activated, the environment is displayed at the current position with
 *  a deactivated depth mask and deactivated depth testing.
 *  [IBLEnvironmentState] ---|> [State]
 * @ingroup states
 */
class IBLEnvironmentState : public State {
		PROVIDES_TYPE_NAME(IBLEnvironmentState)
	public:

		MINSGAPI IBLEnvironmentState();
		MINSGAPI virtual ~IBLEnvironmentState();

		// ---|> [State]
		MINSGAPI IBLEnvironmentState * clone() const override;

		MINSGAPI void loadEnvironmentMapFromHDR(const Util::FileName& filename);
		MINSGAPI void setEnvironmentMap(const Util::Reference<Rendering::Texture>& texture);

		Rendering::Texture* getEnvironmentMap() const { return environmentMap.get(); }
		Rendering::Texture* getIrradianceMap() const { return irradianceMap.get(); }
		Rendering::Texture* getPrefilteredEnvMap() const { return prefilteredEnvMap.get(); }
		Rendering::Texture* getBrdfLUT() const { return brdfLUT.get(); }
		const Util::FileName& getHdrFile() const { return hdrFile; }

		float getLOD() const { return lod; }
		void setLOD(float v) { lod = v; }
		bool isDrawEnvironmentEnabled() const { return drawEnvironmentMap; }
		void setDrawEnvironment(bool b) { drawEnvironmentMap = b; }
		float getRotation() const { return rotation; }
		void setRotation(float rad) { rotation = rad; }
		float getRotationDeg() const { return Geometry::Convert::radToDeg(rotation); }
		void setRotationDeg(float deg) { rotation = Geometry::Convert::degToRad(deg); }

		MINSGAPI void generateFromScene(FrameContext& context, Node* node, const RenderParam& rp);
	private:
		void buildCubeMapFromEquirectangularMap(FrameContext& context);
		void buildIrradianceMap(FrameContext& context);
		void buildPrefilteredEnvMap(FrameContext& context);
		void buildBrdfLUT(FrameContext& context);
		MINSGAPI stateResult_t doEnableState(FrameContext& context, Node* node, const RenderParam& rp) override;
		MINSGAPI void doDisableState(FrameContext& context, Node* node, const RenderParam& rp) override;

		uint32_t resolution = 512;
		uint32_t irrResolution = 32;
		uint32_t prefilterResolution = 128;
		uint32_t brdfLutResolution = 512;
		bool drawEnvironmentMap = true;
		uint8_t baseTextureUnit = 7;
		float lod=0.0;
		float rotation=0.0;
		Util::FileName hdrFile;
		Util::Reference<Rendering::Texture> hdrEquirectangularMap; // will be converted to cube map
		Util::Reference<Rendering::Texture> environmentMap;
		Util::Reference<Rendering::Texture> irradianceMap;
		Util::Reference<Rendering::Texture> prefilteredEnvMap;
		Util::Reference<Rendering::Texture> brdfLUT;
		Util::Reference<Rendering::Shader> environmentShader;
};

}

#endif // IBLEnvironmentState_H
