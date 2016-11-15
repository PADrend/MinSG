/*
	This file is part of the MinSG library.
	Copyright (C) 2016 Stanislaw Eppinger
	Copyright (C) 2016 Sascha Brandt

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_PHOTONRENDERER_H
#define MINSG_EXT_THESISSTANISLAW_PHOTONRENDERER_H

#include <Util/ReferenceCounter.h>

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/CameraNode.h"

#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/FBO.h>

namespace MinSG{
namespace ThesisStanislaw{

class PhotonSampler;
class LightPatchRenderer;
  
class PhotonRenderer {
private:  
  Util::Reference<Rendering::FBO>      _fbo;
  Util::Reference<Rendering::Texture>  _indirectLightTexture, _normalTex, _depthTexture;
  bool                                 _fboChanged;
  uint32_t                             _samplingWidth, _samplingHeight;
  
  Util::Reference<Rendering::Shader>   _indirectLightShader, _accumulationShader;
  
  Util::Reference<Node>                _approxScene;
  
  PhotonSampler*                       _photonSampler;
  LightPatchRenderer*                  _lightPatchRenderer;
  
  Util::Reference<CameraNode>          _photonCamera;

  bool initializeFBO(Rendering::RenderingContext& rc);
  Util::Reference<CameraNode> computePhotonCamera();
  
public:

  PhotonRenderer();
  ~PhotonRenderer() = default;  

  bool gatherLight(FrameContext & context, const RenderParam & rp);
  
  void setShader(const std::string& gatheringShaderFile, const std::string& accumShaderFile);  
  void setApproximatedScene(Node* root);
  void setLightPatchRenderer(LightPatchRenderer* renderer);
  void setPhotonSampler(PhotonSampler* sampler);
  void setSamplingResolution(uint32_t width, uint32_t height);
  
  void bindPhotonBuffer(unsigned int location);
  void unbindPhotonBuffer(unsigned int location);
  Util::Reference<Rendering::Texture> getLightTexture() const { return _indirectLightTexture; }
  Util::Reference<Rendering::Texture> getNormalTexture() const { return _normalTex; }
};

}
}



#endif // MINSG_EXT_THESISSTANISLAW_PHOTONRENDERER_H
#endif // MINSG_EXT_THESISSTANISLAW
