/*
	This file is part of the MinSG library.
	Copyright (C) 20016 Stanislaw Eppinger

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_PHOTONRENDERER_H
#define MINSG_EXT_THESISSTANISLAW_PHOTONRENDERER_H

#include <Util/ReferenceCounter.h>
#include <Util/Timer.h>

#include "Statistics.h"

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/States/State.h"

#include "../../../Rendering/RenderingContext/RenderingContext.h"
#include "../../../Rendering/Shader/Shader.h"
#include "../../../Rendering/Texture/Texture.h"
#include "../../../Rendering/Texture/TextureUtils.h"
#include "../../../Rendering/Texture/TextureType.h"
#include "../../../Rendering/FBO.h"

namespace MinSG{
namespace ThesisStanislaw{

class PhotonSampler;
class LightPatchRenderer;
  
class PhotonRenderer : public State {
  PROVIDES_TYPE_NAME(PhotonRenderer)
private:
  static const std::string             _shaderPath;
  
  Util::Reference<Rendering::FBO>      _fbo;
  Util::Reference<Rendering::Texture>  _indirectLightTexture, _normalTex, _depthTexture;
  bool                                 _fboChanged;
  uint32_t                             _samplingWidth, _samplingHeight;
  
  Util::Reference<Rendering::Shader>   _indirectLightShader, _accumulationShader;
  
  Node*                                _approxScene;
  
  PhotonSampler*                       _photonSampler;
  LightPatchRenderer*                  _lightPatchRenderer;
  
  std::vector<LightNode*>              _spotLights;
  
  Util::Reference<CameraNode>          _photonCamera;

#ifdef MINSG_THESISSTANISLAW_GATHER_STATISTICS
  //Framestatistics
  Util::Timer _timer;
#endif // MINSG_THESISSTANISLAW_GATHER_STATISTICS

  bool initializeFBO(Rendering::RenderingContext& rc);
  Util::Reference<CameraNode> computePhotonCamera();
  
public:

  State::stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

  PhotonRenderer();

  ~PhotonRenderer();

  PhotonRenderer * clone() const override;
  
  void setApproximatedScene(Node* root);
  void setLightPatchRenderer(LightPatchRenderer* renderer);
  void setPhotonSampler(PhotonSampler* sampler);
  void setSpotLights(std::vector<LightNode*> lights);
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
