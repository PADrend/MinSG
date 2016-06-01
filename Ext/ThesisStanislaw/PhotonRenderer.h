#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_PHOTONRENDERER_H
#define MINSG_EXT_THESISSTANISLAW_PHOTONRENDERER_H

#include <Util/ReferenceCounter.h>

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
  Util::Reference<Rendering::Texture>  _indirectLightTexture, _depthTexture;
  bool                                 _fboChanged;
  uint32_t                             _samplingWidth, _samplingHeight;
  
  Util::Reference<Rendering::Shader>   _shader;
  
  Node*                                _approxScene;
  
  PhotonSampler*                       _photonSampler;
  LightPatchRenderer*                  _lightPatchRenderer;
  
  unsigned int                         _photonBufferGLId;
  
  bool initializeFBO(Rendering::RenderingContext& rc);
  void initializePhotonBuffer();
  Util::Reference<CameraNode> computePhotonCamera(Geometry::Vec3f pos, Geometry::Vec3f normal);
  
public:

  State::stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

  PhotonRenderer();

  ~PhotonRenderer();

  PhotonRenderer * clone() const override;
  
  void setApproximatedScene(Node* root);
  void setLightPatchRenderer(LightPatchRenderer* renderer);
  void setPhotonSampler(PhotonSampler* sampler);
  void setSamplingResolution(uint32_t width, uint32_t height);
  
};

}
}



#endif // MINSG_EXT_THESISSTANISLAW_PHOTONRENDERER_H
#endif // MINSG_EXT_THESISSTANISLAW
