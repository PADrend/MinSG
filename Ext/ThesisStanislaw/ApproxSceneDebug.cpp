/*
	This file is part of the MinSG library.
	Copyright (C) 20016 Stanislaw Eppinger

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#include "ApproxSceneDebug.h"

#include "../../Core/FrameContext.h"

namespace MinSG{
namespace ThesisStanislaw{
  
const std::string ApproxSceneDebug::_shaderPath = "ThesisStanislaw/ShaderScenes/shader/";
  
ApproxSceneDebug::ApproxSceneDebug() :
  State(),
  _shader(nullptr), _approxScene(nullptr), _renderer(nullptr)
{
  _shader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "debugApproxScene.vs"), Util::FileName(_shaderPath + "debugApproxScene.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
}

State::stateResult_t ApproxSceneDebug::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  auto& rc = context.getRenderingContext();
  
  rc.pushAndSetShader(_shader.get());
  _renderer->bindTBO(rc, true, false);
  return State::stateResult_t::STATE_OK;
}

void ApproxSceneDebug::doDisableState(FrameContext & context, Node *, const RenderParam & /*rp*/) {
  auto& rc = context.getRenderingContext();
  _renderer->unbindTBO(rc);
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
