#ifdef MINSG_EXT_THESISSTANISLAW

#include "ApproxSceneDebug.h"

#include "../../Core/FrameContext.h"

namespace MinSG{
namespace ThesisStanislaw{
  
const std::string ApproxSceneDebug::_shaderPath = "ThesisStanislaw/shader/";
  
ApproxSceneDebug::ApproxSceneDebug() :
  State(),
  _shader(nullptr), _approxScene(nullptr), _renderer(nullptr)
{
  _shader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "debugApproxScene.vs"), Util::FileName(_shaderPath + "debugApproxScene.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
}

State::stateResult_t ApproxSceneDebug::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  auto& rc = context.getRenderingContext();
  
  rc.pushAndSetShader(_shader.get());
  if(_renderer) rc.pushAndSetBoundImage(0, _renderer->getTBOBindParameters());
  return State::stateResult_t::STATE_OK;
}

void ApproxSceneDebug::doDisableState(FrameContext & context, Node *, const RenderParam & /*rp*/) {
  auto& rc = context.getRenderingContext();
  if(_renderer) rc.popBoundImage(0);
  rc.popShader();
}

void ApproxSceneDebug::setApproximatedScene(Node* root){
  _approxScene = root;
}

void ApproxSceneDebug::setLightPatchRenderer(LightPatchRenderer* renderer){
  _renderer = renderer;
}

ApproxSceneDebug * ApproxSceneDebug::clone() const {
  return new ApproxSceneDebug(*this);
}

ApproxSceneDebug::~ApproxSceneDebug(){}


}
}


#endif // MINSG_EXT_THESISSTANISLAW
