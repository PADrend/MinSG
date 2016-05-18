#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_APPROXSCENEDEBUG_H
#define MINSG_EXT_THESISSTANISLAW_APPROXSCENEDEBUG_H

#include "LightPatchRenderer.h"

#include <Util/ReferenceCounter.h>

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/States/NodeRendererState.h"


#include "../../../Rendering/RenderingContext/RenderingContext.h"
#include "../../../Rendering/Shader/Shader.h"
#include "../../../Rendering/Texture/Texture.h"
#include "../../../Rendering/Texture/TextureUtils.h"
#include "../../../Rendering/Texture/TextureType.h"
#include "../../../Rendering/FBO.h"

namespace MinSG{
namespace ThesisStanislaw{
  
class ApproxSceneDebug : public NodeRendererState {
  PROVIDES_TYPE_NAME(ApproxSceneDebug)
private:
  static const std::string             _shaderPath;
  
  Util::Reference<Rendering::Shader>   _shader;
  
  Node*                                _approxScene;
  
  LightPatchRenderer*                  _renderer;
public:
  /**
   * Node renderer function.
   * This function is registered at the configured channel when the state is activated.
   * This function has to be implemented by subclasses.
   */
  NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;
  State::stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

  /**
   * Create a new node renderer that treats the given channel.
   * 
   * @param newChannel Rendering channel identifier
   */
  ApproxSceneDebug();

  ~ApproxSceneDebug();

  ApproxSceneDebug * clone() const override;
  
  void setApproximatedScene(Node* root);
  void setLightPatchRenderer(LightPatchRenderer* renderer);
  
};

}
}



#endif // MINSG_EXT_THESISSTANISLAW_ApproxSceneDebug_H
#endif // MINSG_EXT_THESISSTANISLAW
