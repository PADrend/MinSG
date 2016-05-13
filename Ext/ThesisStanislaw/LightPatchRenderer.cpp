#ifdef MINSG_EXT_THESISSTANISLAW

#include "LightPatchRenderer.h"

#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"

#include "../../../Rendering/Texture/TextureType.h"
#include "../../../Rendering/Texture/TextureUtils.h"
#include "../../../Rendering/Shader/Uniform.h"
#include "../../../Util/Graphics/PixelFormat.h"
#include "../../../Util/TypeConstant.h"


namespace MinSG{
namespace ThesisStanislaw{
  
const std::string LightPatchRenderer::_shaderPath = "ThesisStanislaw/shader/";
  
LightPatchRenderer::LightPatchRenderer() : 
  NodeRendererState(FrameContext::DEFAULT_CHANNEL),
  _lightPatchFBO(nullptr), _samplingWidth(256), _samplingHeight(256), _fboChanged(true),
  _lightPatchTBO(nullptr), _lightPatchShader(nullptr), _approxScene(nullptr)
{
  _lightPatchShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "lightPatchEstimation.vs"), Util::FileName(_shaderPath + "lightPatchEstimation.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
  std::vector<Rendering::Uniform> uniforms;
  _lightPatchShader->getActiveUniforms(uniforms);
  
  for(auto& uniform : uniforms){
    std::cout << uniform.toString() << std::endl << std::endl;
  }
}

void LightPatchRenderer::initializeFBO(Rendering::RenderingContext& rc){
  _lightPatchFBO = new Rendering::FBO;
  _depthTextureFBO = Rendering::TextureUtils::createDepthTexture(_samplingWidth, _samplingHeight);
  
  rc.pushAndSetFBO(_lightPatchFBO.get());
  
  _lightPatchFBO->attachDepthTexture(rc, _depthTextureFBO.get());
  
  if(!_lightPatchFBO->isComplete(rc)){
    WARN( _lightPatchFBO->getStatusMessage(rc) );
    rc.popFBO();
    return;
  }
  
  rc.popFBO();
}

void LightPatchRenderer::allocateLightPatchTBO(){
  if(!_approxScene) return;
  
  auto NONE = Util::PixelFormat::NONE;
  
  auto pf = Util::PixelFormat(Util::TypeConstant::UINT32, 0, NONE, NONE, NONE);
  auto glpf = Rendering::TextureUtils::pixelFormatToGLPixelFormat(pf);
  
  _tboFormat.sizeY = _tboFormat.numLayers = 1;
  _tboFormat.sizeX =  countTriangles(_approxScene);
  _tboFormat.linearMinFilter = _tboFormat.linearMagFilter = false;
  _tboFormat.pixelFormat = glpf;
  
  _lightPatchTBO = new Rendering::Texture(_tboFormat);
}

NodeRendererResult LightPatchRenderer::displayNode(FrameContext & context, Node * node, const RenderParam & rp){
  auto& rc = context.getRenderingContext();
  
  if(_fboChanged) initializeFBO(rc);
  
  rc.pushAndSetFBO(_lightPatchFBO.get());
  rc.pushAndSetShader(_lightPatchShader.get());
  auto numLights = uint32_t(0); // TODO: Get number of participating light sources
  //TODO: Bind _lightPatchTBO  for image load/store 
  for(uint32_t i = 0; i < numLights; i++){
   rc.clearDepth(0);
   _lightPatchShader->setUniform(rc, Rendering::Uniform("lightID", static_cast<int32_t>(1<<i))); //TODO: set correct lightID
   
   _approxScene->display(context, rp);
  }
  
  rc.popFBO();
  
  return NodeRendererResult::PASS_ON;
}

LightPatchRenderer * LightPatchRenderer::clone() const {
  return new LightPatchRenderer(*this);
}

LightPatchRenderer::~LightPatchRenderer(){}

void LightPatchRenderer::setApproximatedScene(Node* root){
  _approxScene = root;
  allocateLightPatchTBO();
}

void LightPatchRenderer::setSamplingResolution(uint32_t width, uint32_t height){
  _samplingWidth  = width;
  _samplingHeight = height;
  _fboChanged = true;
}

}
}

#endif // MINSG_EXT_THESISSTANISLAW
