#ifdef MINSG_EXT_THESISSTANISLAW
#include <iostream>
#include <iomanip>
#define LIB_GL
#define LIB_GLEW

#include "PhotonSampler.h"

#include "../../Core/FrameContext.h"
#include "../../Core/Transformations.h"

#include "../../../Rendering/Shader/Uniform.h"
#include "../../../Rendering/GLHeader.h"
#include "../../../Rendering/Texture/TextureUtils.h"
#include "../../../Rendering/RenderingContext/RenderingParameters.h"

#include "../../../Util/Graphics/PixelAccessor.h"
#include "../../../Util/Graphics/Bitmap.h"
#include "../../../Util/Graphics/PixelFormat.h"

#include "../../../Rendering/Mesh/MeshVertexData.h"
#include "../../../Rendering/Mesh/MeshDataStrategy.h"
#include "../../../Rendering/Mesh/VertexDescription.h"
#include "../../../Rendering/Mesh/VertexAttributeAccessors.h"
#include "../../../Rendering/Mesh/VertexAttributeIds.h"
#include "../../../Rendering/MeshUtils/MeshUtils.h"

#include "SamplingPatterns/PoissonGenerator.h"
#include "PhotonRenderer.h"

#include "../../../Geometry/SRT.h"

namespace MinSG{
namespace ThesisStanislaw{
  
const std::string PhotonSampler::_shaderPath = "ThesisStanislaw/ShaderScenes/shader/";
  
PhotonSampler::PhotonSampler() :
  State(),
  _fbo(nullptr), _photonMatrixFBO(nullptr), _depthTexture(nullptr), _posTexture(nullptr), _normalTexture(nullptr),
  _fboChanged(true), _camera(nullptr), _mrtShader(nullptr), _photonMatrixShader(nullptr), 
  _approxScene(nullptr), _photonNumber(50), _samplingMesh(nullptr), _photonBufferGLId(0)
{
  _mrtShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "MRT.vs"), Util::FileName(_shaderPath + "MRT.fs"), Rendering::Shader::USE_UNIFORMS);
  _photonMatrixShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "photonMatrices.vs"), Util::FileName(_shaderPath + "photonMatrices.fs"), Rendering::Shader::USE_UNIFORMS);
  resample();
  initializeSamplePointMesh();
}

void PhotonSampler::initializeSamplePointMesh(){
  // create mesh
  Rendering::VertexDescription vertexDesc;
  vertexDesc.appendPosition3D();
  vertexDesc.appendTexCoord(0);
  vertexDesc.appendUnsignedIntAttribute(Util::StringIdentifier("sg_PhotonID"), static_cast<uint8_t>(1));
  
  _samplingMesh = new Rendering::Mesh(vertexDesc, _photonNumber, 0);
  _samplingMesh->setDataStrategy(Rendering::SimpleMeshDataStrategy::getPureLocalStrategy());
  _samplingMesh->setUseIndexData(false);
  _samplingMesh->setDrawMode(Rendering::Mesh::draw_mode_t::DRAW_POINTS);
  
  // init vertex data
  auto& vertexData = _samplingMesh->openVertexData();

  vertexData.allocate(_samplePoints.size() + 1, vertexDesc);
  
  auto posAcc = Rendering::PositionAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::POSITION);
  auto texAcc = Rendering::TexCoordAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::TEXCOORD0);
  auto IDAcc = Rendering::UIntAttributeAccessor::create(vertexData, Util::StringIdentifier("sg_PhotonID"));
  auto mapPos = [](const Geometry::Vec2& p){ return Geometry::Vec3(p.x()* 2.f - 1.f, p.y()* 2.f - 1.f, 0.f);}; //map [0,1] to [-1,1]
  
  for(uint32_t i = 0; i < _samplePoints.size(); i++){
    posAcc->setPosition(i, mapPos(_samplePoints[i]));
    texAcc->setCoordinate(i, _samplePoints[i]);
    IDAcc->setValue(i, i);
  }
  
  
  // Initialize ssbo to store the photon matrices
  if(_photonBufferGLId != 0){
    glDeleteBuffers(1, &_photonBufferGLId);
  }

  glGenBuffers(1, &_photonBufferGLId);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, _photonBufferGLId);
  glBufferData(GL_SHADER_STORAGE_BUFFER, _photonNumber * sizeof(float) * 28 , NULL, GL_STATIC_READ);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

void PhotonSampler::computePhotonMatrices(Rendering::RenderingContext& rc, FrameContext & context){
  // Only for debug purposes. Create a cameranode with the same position and direction as created in shader "photonMatrices.fs"
  // This camera node will then provide the matrix "sg_matrix_worldToCamera" to the shader "photonMatrices.fs".
//  Util::Reference<CameraNode> camera = new CameraNode;
//  auto normal = getNormalAt(rc, Geometry::Vec2f(0.5, 0.5));
//  auto pos = getPosAt(rc,Geometry::Vec2f(0.5, 0.5));
//  auto srt = Geometry::_SRT<float>();
//  srt.translate(pos);
//  camera->setRelTransformation(srt);
//  MinSG::Transformations::rotateToWorldDir(*camera.get(), normal * -1.f);
//  camera->setViewport(Geometry::Rect_i(0, 0,512, 512));
//  camera->setNearFar(0.01, 500);
//  camera->setAngles(-70, 70, -50, 50);
  
//  std::cout << "CameraNode: " <<std::endl;
//  auto mat = camera->getRelTransformationMatrix();
//  for(int i = 0; i < 4; i++){
//    for(int j = 0; j < 4; j++){
//      std::cout << mat.at(i * 4 + j) << " ";  
//    }
//    std::cout << std::endl;
//  }
//  std::cout << std::endl;
  
  
  rc.pushAndSetFBO(_photonMatrixFBO.get());
  _photonMatrixFBO->setDrawBuffers(1);
  rc.pushAndSetShader(_photonMatrixShader.get());
  rc.clearDepth(1.f);
  rc.clearColor(Util::Color4f(0.f, 0.f, 0.f));
  rc.pushAndSetTexture(0, _posTexture.get());
  rc.pushAndSetTexture(1, _normalTexture.get());
  bindPhotonBuffer(2);
  
  rc.pushAndSetLighting(Rendering::LightingParameters(false));
  //context.pushAndSetCamera(camera.get());
  rc.displayMesh(_samplingMesh.get());
  //context.popCamera();
  rc.popLighting();
  unbindPhotonBuffer(2);
  rc.popTexture(1);
  rc.popTexture(0);
  rc.popShader();
  rc.popFBO();
}

void PhotonSampler::allocateSamplingTexture(std::vector<int>& samplingImage){
  using namespace Rendering;
  auto size = static_cast<uint32_t>(std::sqrt(samplingImage.size()));
  _samplingTexture = TextureUtils::createDataTexture(TextureType::TEXTURE_2D, size, size, 1, Util::TypeConstant::INT32, 1);
}

bool PhotonSampler::initializeFBO(Rendering::RenderingContext& rc){
  if(!_camera) return false;
  
  _fbo = new Rendering::FBO;
  _photonMatrixFBO = new Rendering::FBO;
  
  auto width = _camera->getWidth();
  auto height = _camera->getHeight();
  
  _depthTexture = Rendering::TextureUtils::createDepthTexture(width, height);
  _posTexture = Rendering::TextureUtils::createHDRTexture(width, height, false);
  _normalTexture = Rendering::TextureUtils::createHDRTexture(width, height, false);
  
  _fbo->attachDepthTexture(rc, _depthTexture.get());
  _fbo->attachColorTexture(rc, _posTexture.get(), 0);
  _fbo->attachColorTexture(rc, _normalTexture.get(), 1);
  
  _depthTexturePhotonMatrix = Rendering::TextureUtils::createDepthTexture(width, height);
  _photonMatrixTexture = Rendering::TextureUtils::createHDRTexture(width, height, false);
  
  _photonMatrixFBO->attachDepthTexture(rc, _depthTexturePhotonMatrix.get());
  _photonMatrixFBO->attachColorTexture(rc, _photonMatrixTexture.get(), 0);
  
  if(!_fbo->isComplete(rc)){
    WARN( _fbo->getStatusMessage(rc) );
    return false;
  }
  
  return true;
}

State::stateResult_t PhotonSampler::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  auto& rc = context.getRenderingContext();
  rc.setImmediateMode(true);
  rc.applyChanges();
  
  if(_fboChanged){
    if(!initializeFBO(rc)){
      WARN("Could not initialize FBO for PhotonSampler!");
      return State::stateResult_t::STATE_SKIPPED;
    }
    _fboChanged = false;
  }
  
  clearPhotonBuffer();
  
  rc.pushAndSetFBO(_fbo.get());
  _fbo->setDrawBuffers(2);
  rc.pushAndSetShader(_mrtShader.get());
  
  if(_camera != nullptr){
    context.pushAndSetCamera(_camera);
    _mrtShader->setUniform(rc, Rendering::Uniform("sg_useMaterials", false));
    
    rc.clearDepth(1.0f);
    rc.clearColor(Util::Color4f(0.f, 0.f, 0.f));
    _approxScene->display(context, rp);

    context.popCamera();
  }
  
  rc.popShader();
  rc.popFBO();
  
//  _posTexture->downloadGLTexture(rc);
//  _normalTexture->downloadGLTexture(rc);
  
  computePhotonMatrices(rc, context);
  
  
  // Check if the PhotonBuffer has changed somehow
//  glBindBuffer(GL_SHADER_STORAGE_BUFFER, _photonBufferGLId);
//  GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//  float* ptr = reinterpret_cast<float*>(p);
//  std::cout << "Photon Buffer (Sampler): " << std::endl;
//  std::cout << *(ptr) <<" "<< *(ptr+4) <<" "<< *((ptr)+8) <<" "<< *((ptr)+12) << std::endl;
//  std::cout << *(ptr+1) <<" "<< *((ptr)+5) <<" "<< *((ptr)+9) <<" "<< *((ptr)+13) << std::endl;
//  std::cout << *(ptr+2) <<" "<< *((ptr)+6) <<" "<< *((ptr)+10) <<" "<< *((ptr)+14) << std::endl;
//  std::cout << *(ptr+3) <<" "<< *((ptr)+7) <<" "<< *((ptr)+11) <<" "<< *((ptr)+15) << std::endl << std::endl;
////  std::cout << "Diffuse: " << *(ptr+16) <<" "<< *((ptr)+17) <<" "<< *((ptr)+18) <<" "<< *((ptr)+19) << std::endl;
////  std::cout << "Pos: " << *(ptr+20) <<" "<< *((ptr)+21) <<" "<< *((ptr)+22) <<" "<< *((ptr)+23) << std::endl;
////  std::cout << "Nor: " << *(ptr+24) <<" "<< *((ptr)+25) <<" "<< *((ptr)+26) <<" "<< *((ptr)+27) << std::endl << std::endl;
//  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  
  rc.setImmediateMode(false);
  
//  rc.pushAndSetShader(nullptr);
//  auto width = _camera->getWidth();
//  auto height = _camera->getHeight();
////   Displays the normal texture correctly if and only if the mentioned line in the LightPatchRenderer is commented out.
//  Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, width, height), *(_photonMatrixTexture.get()), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
//  rc.popShader();
//  return State::stateResult_t::STATE_SKIP_RENDERING;
  
  return State::stateResult_t::STATE_OK;
}

Geometry::Vec3f PhotonSampler::getNormalAt(Rendering::RenderingContext& rc, const Geometry::Vec2f& texCoord){
  auto acc = Rendering::TextureUtils::createColorPixelAccessor(rc, *(_normalTexture.get()));
  auto width = _camera->getWidth();
  auto height = _camera->getHeight();
  auto x = static_cast<uint32_t>(texCoord.x() * width);
  auto y = static_cast<uint32_t>(texCoord.y() * height);
  
  if(x == width) x--;
  if(y == height) y--;
  
  auto color = acc->readColor4f(x, y);
  return Geometry::Vec3f(color.r(), color.g(), color.b());
}

Geometry::Vec3f PhotonSampler::getPosAt(Rendering::RenderingContext& rc, const Geometry::Vec2f& texCoord){
  auto acc = Rendering::TextureUtils::createColorPixelAccessor(rc, *(_posTexture.get()));
  auto width = _camera->getWidth();
  auto height = _camera->getHeight();
  auto x = static_cast<uint32_t>(texCoord.x() * width);
  auto y = static_cast<uint32_t>(texCoord.y() * height);
  
  if(x == width) x--;
  if(y == height) y--;
  
  auto color = acc->readColor4f(x, y);
  return Geometry::Vec3f(color.r(), color.g(), color.b());
}

void PhotonSampler::setApproximatedScene(Node* root){
  _approxScene = root;
}

void PhotonSampler::setPhotonNumber(uint32_t number){
  _photonNumber = number;
  resample();
  initializeSamplePointMesh();
}

uint32_t PhotonSampler::getTextureWidth(){
  if(!_camera) return 0;
  return _camera->getWidth();
}

uint32_t PhotonSampler::getTextureHeight(){
  if(!_camera) return 0;
  return _camera->getHeight();
}

void PhotonSampler::bindPhotonBuffer(unsigned int location){
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, _photonBufferGLId);
}

void PhotonSampler::unbindPhotonBuffer(unsigned int location){
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, _photonBufferGLId);
}

void PhotonSampler::clearPhotonBuffer(){
  float clearVal = 0.f;
  glClearNamedBufferData(_photonBufferGLId, GL_R32F, GL_RED, GL_FLOAT, &clearVal);
}

const std::vector<Geometry::Vec2f>& PhotonSampler::getSamplePoints(){
  return _samplePoints;
}

uint32_t PhotonSampler::getPhotonNumber(){
  return _photonNumber;
}

void PhotonSampler::setSamplingStrategy(uint8_t type){
  _samplingStrategy = static_cast<Sampling>(type);
  resample();
}

void PhotonSampler::resample(){
  std::vector<int> samplingImage;

  uint32_t additionalPhotons = 0;
  size_t imageSize = 0;
  
  while(samplingImage.size() == 0 || _samplePoints.size() < _photonNumber){
      
    switch(_samplingStrategy){
      case Sampling::POISSON:
      default:{
        Sampler::PoissonGenerator::DefaultPRNG PRNG;
        auto points = Sampler::PoissonGenerator::GeneratePoissonPoints( _photonNumber + additionalPhotons, PRNG , 50, false);
        _samplePoints.clear();
        for(auto& point : points){
          _samplePoints.push_back(Geometry::Vec2f(point.x, point.y));
        }
      }
    }
    
    imageSize = static_cast<size_t>(std::ceil(std::sqrt(_photonNumber + additionalPhotons)));
    std::vector<std::tuple<float, float, size_t>> fPoints;
    for (size_t idx = 0; idx < _samplePoints.size(); idx++) {
      fPoints.push_back(std::make_tuple(_samplePoints[idx].x(), _samplePoints[idx].y(), idx));
    }
    
    samplingImage = computeSamplingImage(fPoints, imageSize);
    additionalPhotons++;
  }
  
  std::vector<uint8_t> byteData;
  
  uint8_t* ptr = reinterpret_cast<uint8_t*>(&samplingImage[0]);
  for(size_t i = 0; i < samplingImage.size() * sizeof(int); i++){
    byteData.push_back(ptr[i]);
  }
  
  auto NONE = Util::PixelFormat::NONE;
  Util::PixelFormat pf = Util::PixelFormat(Util::TypeConstant::INT32, 0, NONE, NONE, NONE);
  
  Util::Bitmap bitmap(static_cast<const uint32_t>(imageSize), static_cast<const uint32_t>(imageSize), pf);
  bitmap.setData(byteData);
  bitmap.flipVertically();
  
  _samplingTexture = Rendering::TextureUtils::createTextureFromBitmap(bitmap);
  std::cout << "Num Sample Points: " << _samplePoints.size() << " Image Size: " << imageSize << std::endl;
//  std::cout << samplingImage.size() << std::endl;
//  std::cout << "------------------------------------------------------" << std::endl;
//  size_t countt = 0;
//  for(auto point : _samplePoints){
//    std::cout << countt <<": ";
//    std::cout << std::fixed << std::setprecision(10) << std::setfill('0') << point.x() << " " << point.y() << std::endl;
//    countt++;
//  }
//  std::cout << "--------static_cast<uint32_t>(----------------------------------------------" << std::endl;
//  auto width = 1280;
//  auto height = 720;
//  countt = 0;
//  for(auto point : _samplePoints){
//    std::cout << countt <<": ";
//    auto x = static_cast<uint32_t>(point.x() * width);
//    auto y = static_cast<uint32_t>(point.y() * height);
//    std::cout << x << " " << y << std::endl;
//    countt++;
//  }
  
  allocateSamplingTexture(samplingImage);
}

void PhotonSampler::setCamera(CameraNode* camera){
  _camera = camera;
  _fboChanged = true;
}

Util::Reference<Rendering::Texture> PhotonSampler::getPosTexture(){
  return _posTexture;
}

Util::Reference<Rendering::Texture> PhotonSampler::getNormalTexture(){
  return _normalTexture;
}

void PhotonSampler::bindSamplingTexture(Rendering::RenderingContext& rc){
  rc.pushAndSetTexture(0, _samplingTexture.get());
}

void PhotonSampler::unbindSamplingTexture(Rendering::RenderingContext& rc){
  rc.popTexture(0);
}

PhotonSampler * PhotonSampler::clone() const {
  return new PhotonSampler(*this);
}

PhotonSampler::~PhotonSampler(){}

float PhotonSampler::distance(std::pair<float, float> p1, std::pair<float, float> p2) {
  return static_cast<float>(std::sqrt((p1.first - p2.first) * (p1.first - p2.first) + (p1.second - p2.second) * (p1.second - p2.second)));
}

std::vector<size_t> PhotonSampler::getPointsInQuad(const std::vector<std::tuple<float, float, size_t>>& Points, float x1, float y1, float x2, float y2) {
  using std::get;
  std::vector<size_t> ret;

  for (size_t i = 0; i < Points.size(); i++) {
    auto point = Points[i];
    if (get<0>(point) >= x1  && get<0>(point) <= x2 && get<1>(point) >= y1 && get<1>(point) <= y2) {
      ret.push_back(i);
    }
  }

  return ret;
}

std::pair<size_t, size_t> PhotonSampler::checkNeighbours(const std::vector<std::tuple<float, float, size_t>>& Points, const std::vector<std::vector<bool>>& occupiedCells, float x1, float y1, float x2, float y2, float offset, size_t x, size_t y, size_t& idxMovingPoint, size_t& idxStayingPoint) {
  using std::get;
  using std::make_pair;

  float dist = 2.f;
  auto points = getPointsInQuad(Points, x1, y1, x2, y2);
  auto retX = x;
  auto retY = y;

  auto dp1 = distance(make_pair(x1 + (x2-x1)/2.f, y1 + (y2-y1)/2.f), make_pair(get<0>(Points[points[0]]), get<1>(Points[points[0]])) );
  auto dp2 = distance(make_pair(x1 + (x2 - x1) / 2.f, y1 + (y2 - y1) / 2.f), make_pair(get<0>(Points[points[1]]), get<1>(Points[points[1]])));

  auto movingPoint = make_pair(get<0>(Points[points[0]]), get<1>(Points[points[0]]));
  idxMovingPoint = points[0];
  idxStayingPoint = points[1];
  if (dp1 > dp2) {
    movingPoint = make_pair(get<0>(Points[points[1]]), get<1>(Points[points[1]]));
    idxMovingPoint = points[1];
    idxStayingPoint = points[0];
  }

  /*
  x o o
  o o o
  o o o
  */
  if (x1 - offset >= 0.f && y2 + offset <= 1.f) 
  {
    auto pINq = getPointsInQuad(Points, x1 - offset, y1 + offset, x2 - offset, y2 + offset);
    if (pINq.size() == 0 && occupiedCells[x-1][y+1] == false) {
      auto thisDist = distance(make_pair( x1 - offset + (x2 - x1) / 2.f, y1 + offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x - 1;
        retY = y + 1;
      }
    }
  }

  /*
  o x o
  o o o
  o o o
  */
  if (y2 + offset <= 1.f)
  {
    auto pINq = getPointsInQuad(Points, x1, y1 + offset, x2, y2 + offset);
    if (pINq.size() == 0 && occupiedCells[x][y + 1] == false) {
      auto thisDist = distance(make_pair(x1 + (x2 - x1) / 2.f, y1 + offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x;
        retY = y + 1;
      }
    }
  }

  /*
  o o x
  o o o
  o o o
  */
  if (x2 + offset <= 1.f && y2 + offset <= 1.f)
  {
    auto pINq = getPointsInQuad(Points, x1 + offset, y1 + offset, x2 + offset, y2 + offset);
    if (pINq.size() == 0 && occupiedCells[x + 1][y + 1] == false) {
      auto thisDist = distance(make_pair(x1 + offset + (x2 - x1) / 2.f, y1 + offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x + 1;
        retY = y + 1;
      }
    }
  }

  /*
  o o o
  x o o
  o o o
  */
  if (x1 - offset >= 0.f)
  {
    auto pINq = getPointsInQuad(Points, x1 - offset, y1, x2 - offset, y2);
    if (pINq.size() == 0 && occupiedCells[x - 1][y] == false) {
      auto thisDist = distance(make_pair(x1 - offset + (x2 - x1) / 2.f, y1 + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x - 1;
        retY = y;
      }
    }
  }

  /*
  o o o
  o o x
  o o o
  */
  if (x2 + offset <= 1.f)
  {
    auto pINq = getPointsInQuad(Points, x1 + offset, y1, x2 + offset, y2);
    if (pINq.size() == 0 && occupiedCells[x + 1][y] == false) {
      auto thisDist = distance(make_pair(x1 + offset + (x2 - x1) / 2.f, y1 + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x + 1;
        retY = y;
      }
    }
  }

  /*
  o o o
  o o o
  x o o
  */
  if (x1 - offset >= 0.f && y1 - offset >= 0.f)
  {
    auto pINq = getPointsInQuad(Points, x1 - offset, y1 - offset, x2 - offset, y2 - offset);
    if (pINq.size() == 0 && occupiedCells[x - 1][y - 1] == false) {
      auto thisDist = distance(make_pair(x1 - offset + (x2 - x1) / 2.f, y1 - offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x - 1;
        retY = y - 1;
      }
    }
  }

  /*
  o o o
  o o o
  o x o
  */
  if (y1 - offset >= 0.f)
  {
    auto pINq = getPointsInQuad(Points, x1, y1 - offset, x2, y2 - offset);
    if (pINq.size() == 0 && occupiedCells[x][y - 1] == false) {
      auto thisDist = distance(make_pair(x1 + (x2 - x1) / 2.f, y1 - offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x;
        retY = y - 1;
      }
    }
  }

  /*
  o o o
  o o o
  o o x
  */
  if (x2 + offset <= 1.f && y1 - offset >= 0.f)
  {
    auto pINq = getPointsInQuad(Points, x1 + offset, y1 - offset, x2 + offset, y2 - offset);
    if (pINq.size() == 0 && occupiedCells[x + 1][y - 1] == false) {
      auto thisDist = distance(make_pair(x1 + offset + (x2 - x1) / 2.f, y1 - offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x + 1;
        retY = y - 1;
      }
    }
  }

  return make_pair(retX, retY);

}

std::vector<int> PhotonSampler::computeSamplingImage(std::vector<std::tuple<float, float, size_t>> Points, size_t size) {
  using std::get;

  std::vector<int> image; 
  image.resize(size * size);
  std::fill(image.begin(), image.end(), -1);

  float offset = 1.0f / static_cast<float>(size);
  float x1, y1, x2, y2;

  x1 = y1 = 0.f;
  x2 = y2 = offset;

  std::vector<std::vector<bool>> occupiedCells;
  occupiedCells.resize(size);
  for (size_t i = 0; i < size; i++) {
    occupiedCells[i].resize(size);
    std::fill(occupiedCells[i].begin(), occupiedCells[i].end(), false);
  }

  for (size_t y = 0; y < size; y++){
    for (size_t x = 0; x < size; x++) {
      auto pINq = getPointsInQuad(Points, x1, y1, x2, y2);
      
      if (pINq.size() == 2) {
        auto idxMovingPoint = pINq[0];
        auto idxStayingPoint = pINq[1];
        auto newPos = checkNeighbours(Points, occupiedCells, x1, y1, x2, y2, offset, x, y, idxMovingPoint, idxStayingPoint);

        if (newPos.first == x && newPos.second == y) {
          image.clear();
          return image;
        }
        else {
          if (image[x + y * size] != -1 || image[newPos.first + newPos.second * size] != -1) {
            image.clear();
            return image;
          }
          
          image[x + y * size] = get<2>(Points[idxStayingPoint]);
          image[newPos.first + newPos.second * size] = get<2>(Points[idxMovingPoint]);
          occupiedCells[x][y] = true;
          
          if (idxMovingPoint > idxStayingPoint) {
            Points.erase(Points.begin() + idxMovingPoint);
            Points.erase(Points.begin() + idxStayingPoint);
          }
          else {
            Points.erase(Points.begin() + idxStayingPoint);
            Points.erase(Points.begin() + idxMovingPoint);
          }
        }
      } 
      else if(pINq.size() == 1) {
        if (image[x + y * size] != -1){
          image.clear();
          return image;
        }
        
        image[x + y * size] = get<2>(Points[pINq[0]]); 
        occupiedCells[x][y] = true;
        Points.erase(Points.begin() + pINq[0]);
      }
      else if (pINq.size() > 2) {
        image.clear();
        return image;
      }

      x1 += offset;
      x2 += offset;
    }

    y1 += offset;
    y2 += offset;
    x1 = 0;
    x2 = offset;
  }

  return image;
}

}
}


#endif // MINSG_EXT_THESISSTANISLAW
