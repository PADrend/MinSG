#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_PHONGGI_H
#define MINSG_EXT_THESISSTANISLAW_PHONGGI_H

#include <Util/ReferenceCounter.h>
#include <Util/Timer.h>

#include "PhotonSampler.h"
#include "PhotonRenderer.h"
#include "Statistics.h"

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

#ifdef MINSG_THESISSTANISLAW_GATHER_STATISTICS
  //Framestatistics
  Util::Timer _timer;
#endif // MINSG_THESISSTANISLAW_GATHER_STATISTICS

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
