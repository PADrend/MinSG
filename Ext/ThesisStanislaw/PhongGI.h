#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_PHONGGI_H
#define MINSG_EXT_THESISSTANISLAW_PHONGGI_H

#include "PhotonSampler.h"
#include "PhotonRenderer.h"

#include <Util/ReferenceCounter.h>

#include "../../Core/States/State.h"

#include "../../../Rendering/RenderingContext/RenderingContext.h"
#include "../../../Rendering/Shader/Shader.h"

namespace MinSG{
namespace ThesisStanislaw{
  
class PhongGI : public State {
  PROVIDES_TYPE_NAME(PhongGI)
private:
  static const std::string             _shaderPath;
  
  Util::Reference<Rendering::Shader>   _shader;
  
  PhotonSampler*                       _photonSampler;
  
public:
  State::stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
  void doDisableState(FrameContext & context,Node *, const RenderParam & rp) override;

  PhongGI();

  ~PhongGI();

  PhongGI * clone() const override;
  
  void setPhotonSampler(PhotonSampler* sampler);
  
};

}
}



#endif // MINSG_EXT_THESISSTANISLAW_PHONGGI_H
#endif // MINSG_EXT_THESISSTANISLAW
