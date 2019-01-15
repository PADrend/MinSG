/*
	This file is part of the MinSG library.
	Copyright (C) 2016 Stanislaw Eppinger
	Copyright (C) 2016 Sascha Brandt

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H
#define MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H

#include <Util/ReferenceCounter.h>

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/CameraNode.h"

#include <Rendering/Shader/Shader.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/FBO.h>
#include <Rendering/RenderingContext/RenderingContext.h>


namespace MinSG{
//! @ingroup ext
namespace ThesisStanislaw{

class LightPatchRenderer {
private:  
  Util::Reference<Rendering::FBO>      _lightPatchFBO;
  Util::Reference<Rendering::Texture>  _depthTextureFBO;
  
  uint32_t                             _samplingWidth, _samplingHeight;
  bool                                 _fboChanged;
  
  Rendering::Texture::Format           _tboFormat;
  Rendering::ImageBindParameters       _tboBindParameters;
  Util::Reference<Rendering::Texture>  _lightPatchTBO;
  Util::Reference<Rendering::Texture>  _polygonIDTexture;
  
  Util::Reference<Rendering::Shader>   _polygonIDWriterShader;
  Util::Reference<Rendering::Shader>   _lightPatchShader;
  
  std::vector<Util::Reference<LightNode>> _spotLights;
  
  Util::Reference<Node>                _approxScene;
  Util::Reference<CameraNode>          _lightCamera;
  
  Util::Reference<Rendering::Texture> _normalTexture;

  void allocateLightPatchTBO();
  void initializeFBO(Rendering::RenderingContext& rc);
  void computeLightMatrix(const LightNode* light);
  
public:
  LightPatchRenderer();
  ~LightPatchRenderer() = default;

  bool computeLightPatches(FrameContext & context, const RenderParam & rp);
  
  void setShader(const std::string& lightPatchShaderFile, const std::string& polygonIdShaderFile);
  void setApproximatedScene(Node* root);
  void setLightSources();
  void setSamplingResolution(uint32_t width, uint32_t height);
  void setSpotLights(std::vector<LightNode*> lights);
  
  void bindTBO(Rendering::RenderingContext& rc, bool read, bool write);
  void unbindTBO(Rendering::RenderingContext& rc);
  
  Util::Reference<Rendering::Texture> getPolygonIDTexture() const { return _polygonIDTexture; }
  Util::Reference<Rendering::Texture> getLightPatchTBO() const { return _lightPatchTBO; }
  Util::Reference<Rendering::Texture> getNormalTexture() const { return _normalTexture; }
};

}
}


#endif // MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H
#endif // MINSG_EXT_THESISSTANISLAW
