#ifdef MINSG_EXT_THESISSTANISLAW

#include "PhongGI.h"

#include "../../Core/FrameContext.h"
#include "../../../Rendering/Shader/Uniform.h"


namespace MinSG{
namespace ThesisStanislaw{
  
const std::string PhongGI::_shaderPath = "ThesisStanislaw/ShaderScenes/shader/";
  
PhongGI::PhongGI() :
  State(),
  _shader(nullptr), _photonSampler(nullptr)
{
  _shader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "phongGI.vs"), Util::FileName(_shaderPath + "phongGI.fs"), Rendering::Shader::USE_UNIFORMS);
}

State::stateResult_t PhongGI::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  auto& rc = context.getRenderingContext();

  rc.pushAndSetShader(_shader.get());
  if(_photonSampler) {
    _shader->setUniform(rc, Rendering::Uniform("samplingTextureSize", static_cast<int32_t>(_photonSampler->getSamplingTextureSize())));
    _photonSampler->bindSamplingTexture(rc, 7);
    _photonSampler->bindPhotonBuffer(30);
  }
  return State::stateResult_t::STATE_OK;
}

void PhongGI::doDisableState(FrameContext & context, Node *, const RenderParam & /*rp*/) {
  auto& rc = context.getRenderingContext();
  
  if(_photonSampler){ 
    _photonSampler->unbindSamplingTexture(rc, 7);
    _photonSampler->unbindPhotonBuffer(30);
  }
  
  rc.popShader();
  
  //_photonSampler->outputPhotonBuffer("PhongGI");
}

void PhongGI::setPhotonSampler(PhotonSampler* sampler){
  _photonSampler = sampler;
}

PhongGI * PhongGI::clone() const {
  return new PhongGI(*this);
}

PhongGI::~PhongGI(){}

}
}


#endif // MINSG_EXT_THESISSTANISLAW
