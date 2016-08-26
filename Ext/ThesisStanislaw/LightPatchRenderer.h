#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H
#define MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H

#include <Util/ReferenceCounter.h>
#include <Util/Timer.h>

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/States/State.h"
#include "../../Core/Statistics.h"

#include "../../../Rendering/Shader/Shader.h"
#include "../../../Rendering/Texture/Texture.h"
#include "../../../Rendering/Texture/TextureType.h"
#include "../../../Rendering/FBO.h"
#include "../../../Rendering/RenderingContext/RenderingContext.h"
#include "../../../Rendering/RenderingContext/RenderingParameters.h"


namespace MinSG{
namespace ThesisStanislaw{

class LightPatchRenderer : public State {
  PROVIDES_TYPE_NAME(LightPatchRenderer)
private:
  static const std::string _shaderPath;
  
  Util::Reference<Rendering::FBO>      _lightPatchFBO;
  //Util::Reference<Rendering::FBO>      _fbo2;
  Util::Reference<Rendering::Texture>  _depthTextureFBO;
  //Util::Reference<Rendering::Texture>  _depthTextureFBO2;
  uint32_t                             _samplingWidth, _samplingHeight;
  bool                                 _fboChanged;
  
  Rendering::Texture::Format           _tboFormat;
  Rendering::ImageBindParameters       _tboBindParameters;
  Util::Reference<Rendering::Texture>  _lightPatchTBO;
  Util::Reference<Rendering::Texture>  _polygonIDTexture;
  
  Util::Reference<Rendering::Shader>   _polygonIDWriterShader;
  Util::Reference<Rendering::Shader>   _lightPatchShader;
  
  std::vector<LightNode*> _spotLights;
  
  Node*                   _approxScene;
  
  CameraNode*  _camera;
  Util::Reference<Rendering::Texture> _normalTexture;
  
  //Framestatistics
  Util::Timer _timer;
  uint32_t _statKey;
  bool _statInitialized;
  
  void allocateLightPatchTBO();
  void initializeFBO(Rendering::RenderingContext& rc);
  Util::Reference<CameraNode> computeLightMatrix(const LightNode* light);
  
public:

  State::stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
  void doDisableState(FrameContext & context,Node *, const RenderParam & rp) override;

  LightPatchRenderer();

  ~LightPatchRenderer();

  LightPatchRenderer * clone() const override;
  
  void setApproximatedScene(Node* root);
  void setLightSources();
  void setSamplingResolution(uint32_t width, uint32_t height);
  void setSpotLights(std::vector<LightNode*> lights);
  
  void bindTBO(Rendering::RenderingContext& rc, bool read, bool write);
  void unbindTBO(Rendering::RenderingContext& rc);
  
  void setCamera(CameraNode* camera);
  Util::Reference<Rendering::Texture> getPolygonIDTexture() const { return _polygonIDTexture; }
  Util::Reference<Rendering::Texture> getLightPatchTBO() const { return _lightPatchTBO; }
  Util::Reference<Rendering::Texture> getNormalTexture() const { return _normalTexture; }
};

}
}


#endif // MINSG_EXT_THESISSTANISLAW_LIGHTPATCHRENDERER_H
#endif // MINSG_EXT_THESISSTANISLAW
