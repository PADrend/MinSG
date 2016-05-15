#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H
#define MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H

#include <Util/ReferenceCounter.h>

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/States/NodeRendererState.h"

#include "../../../Rendering/Shader/Shader.h"
#include "../../../Rendering/Texture/Texture.h"
#include "../../../Rendering/Texture/TextureType.h"
#include "../../../Rendering/FBO.h"
#include "../../../Rendering/RenderingContext/RenderingContext.h"
#include "../../../Rendering/RenderingContext/RenderingParameters.h"


namespace MinSG{
namespace ThesisStanislaw{

class LightPatchRenderer : public NodeRendererState {
  PROVIDES_TYPE_NAME(LightPatchRenderer)
private:
  static const std::string _shaderPath;
  
  Util::Reference<Rendering::FBO>      _lightPatchFBO;
  Util::Reference<Rendering::Texture>  _depthTextureFBO;
  Rendering::ImageBindParameters       _tboBindParameters;
  uint32_t                             _samplingWidth, _samplingHeight;
  bool                                 _fboChanged;
  
  Rendering::Texture::Format           _tboFormat;
  Util::Reference<Rendering::Texture>  _lightPatchTBO;
  
  Util::Reference<Rendering::Shader>   _lightPatchShader;
  
  std::vector<LightNode*> _spotLights;
  
  Node*               _approxScene;
  
  void allocateLightPatchTBO();
  void initializeFBO(Rendering::RenderingContext& rc);
  Util::Reference<CameraNode> computeLightMatrix(const LightNode* light);
public:
  /**
   * Node renderer function.
   * This function is registered at the configured channel when the state is activated.
   * This function has to be implemented by subclasses.
   */
  NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

  /**
   * Create a new node renderer that treats the given channel.
   * 
   * @param newChannel Rendering channel identifier
   */
  LightPatchRenderer();

  ~LightPatchRenderer();

  LightPatchRenderer * clone() const override;
  
  void setApproximatedScene(Node* root);
  void setLightSources();
  void setSamplingResolution(uint32_t width, uint32_t height);
  void setSpotLights(std::vector<LightNode*> lights);
  
};

}
}


#endif // MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H
#endif // MINSG_EXT_THESISSTANISLAW
