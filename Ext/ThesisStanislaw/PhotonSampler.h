#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H
#define MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H

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
  
class PhotonSampler : public State {
  PROVIDES_TYPE_NAME(PhotonSampler)
public:
  enum class Sampling : uint8_t {
    POISSON
  };
  
private:
  static const std::string             _shaderPath;
  
  Util::Reference<Rendering::FBO>      _fbo;
  Util::Reference<Rendering::Texture>  _depthTexture, _posTexture, _normalTexture;
  bool                                 _fboChanged;
  
  CameraNode*  _camera;
  
  Util::Reference<Rendering::Shader>   _shader;
  
  Node*                                _approxScene;
  
  std::vector<Geometry::Vec2f>    _samplePoints;
  uint32_t                        _photonNumber;
  Sampling                        _samplingStrategy;
  
  
  bool initializeFBO(Rendering::RenderingContext& rc);
public:

  State::stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

  PhotonSampler();

  ~PhotonSampler();

  PhotonSampler * clone() const override;
  
  void setApproximatedScene(Node* root);
  void setPhotonNumber(uint32_t number);
  void setSamplingStrategy(uint8_t type);
  void setCamera(CameraNode* camera);
  void resample();
  
  Util::Reference<Rendering::Texture> getPosTexture();
  Util::Reference<Rendering::Texture> getNormalTexture();

};

}
}



#endif // MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H
#endif // MINSG_EXT_THESISSTANISLAW
