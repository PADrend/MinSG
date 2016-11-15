/*
	This file is part of the MinSG library.
	Copyright (C) 2016 Stanislaw Eppinger
	Copyright (C) 2016 Sascha Brandt

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#include "PhotonSampler.h"
#include "SamplingPatterns/PoissonGenerator.h"
#include "SamplingPatterns/UniformSampler.h"

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Transformations.h"

#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Texture/TextureType.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Texture/TextureType.h>
#include <Rendering/Texture/PixelFormatGL.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshUtils.h>

#include <Util/Graphics/PixelAccessor.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelFormat.h>

#include <Geometry/SRT.h>

#include <iostream>
#include <iomanip>

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

#ifndef GL_STATIC_READ
#define GL_STATIC_READ 0x88E5
#endif

namespace MinSG{
namespace ThesisStanislaw{
    
PhotonSampler::PhotonSampler() :
  _fbo(nullptr), _photonMatrixFBO(nullptr), _depthTexturePhotonMatrix(nullptr), _photonMatrixTexture(nullptr),
  _depthTexture(nullptr), _posTexture(nullptr), _normalTexture(nullptr),
  _fboChanged(true), _needResampling(true), _samplingTexture(nullptr), _samplingTextureSize(0), _mrtShader(nullptr), _photonMatrixShader(nullptr), 
  _approxScene(nullptr), _photonNumber(50), _samplingMesh(nullptr), _photonBuffer() { }

void PhotonSampler::setShader(const std::string& mrtShaderFile, const std::string& photonShaderFile) {
  _mrtShader = Rendering::Shader::loadShader(Util::FileName(mrtShaderFile), Util::FileName(mrtShaderFile), Rendering::Shader::USE_UNIFORMS);
  _photonMatrixShader = Rendering::Shader::loadShader(Util::FileName(photonShaderFile), Util::FileName(photonShaderFile), Rendering::Shader::USE_UNIFORMS);
}

void PhotonSampler::initializeSamplePointMesh() {
  // create mesh
  Rendering::VertexDescription vertexDesc;
  vertexDesc.appendPosition3D();
  vertexDesc.appendTexCoord(0);
  vertexDesc.appendUnsignedIntAttribute(Util::StringIdentifier("sg_PhotonID"), static_cast<uint8_t>(1), false);
  
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
  _photonBuffer.destroy();
  _photonBuffer.allocateData<uint32_t>(GL_SHADER_STORAGE_BUFFER, _photonNumber * 32, GL_STATIC_READ);
}

void PhotonSampler::computePhotonMatrices(FrameContext & context){
  auto& rc = context.getRenderingContext();
  rc.pushAndSetFBO(_photonMatrixFBO.get());
  _photonMatrixFBO->setDrawBuffers(1);
  rc.pushAndSetShader(_photonMatrixShader.get());
  rc.clearDepth(1.f);
  rc.clearColor(Util::Color4f(0.f, 0.f, 0.f));
  rc.pushAndSetTexture(0, _posTexture.get());
  rc.pushAndSetTexture(1, _normalTexture.get());
  bindPhotonBuffer(2);
  
  rc.pushAndSetLighting(Rendering::LightingParameters(false));

  rc.displayMesh(_samplingMesh.get());

  rc.popLighting();
  unbindPhotonBuffer(2);
  rc.popTexture(1);
  rc.popTexture(0);
  rc.popShader();
  rc.popFBO();
}

bool PhotonSampler::initializeFBO(FrameContext & context){
  auto camera = context.getCamera();
  
  auto& rc = context.getRenderingContext();
  _fbo = new Rendering::FBO;
  _photonMatrixFBO = new Rendering::FBO;
  
  auto width = camera->getWidth();
  auto height = camera->getHeight();
  
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

bool PhotonSampler::computePhotonSamples(FrameContext & context, const RenderParam & rp){
  
  if(!_approxScene){
    WARN("No approximated Scene present in PhotonSampler!");
    return false;
  }
  
  auto& rc = context.getRenderingContext();
  rc.setImmediateMode(true);
  rc.applyChanges();
  
  if(_fboChanged){
    if(!initializeFBO(context)){
      WARN("Could not initialize FBO for PhotonSampler!");
      return false;
    }
    _fboChanged = false;
  }
  
  if(_needResampling){
    resample(rc);
    initializeSamplePointMesh();
    _needResampling = false;  
  }
  
  clearPhotonBuffer();
  
  rc.pushAndSetFBO(_fbo.get());
  _fbo->setDrawBuffers(2);
  rc.pushAndSetShader(_mrtShader.get());

  //context.pushAndSetCamera(_camera.get());
  _mrtShader->setUniform(rc, Rendering::Uniform("sg_useMaterials", false));
  
  rc.clearDepth(1.0f);
  rc.clearColor(Util::Color4f(0.f, 0.f, 0.f));
  _approxScene->display(context, rp);
  //context.popCamera();
  
  rc.popShader();
  rc.popFBO();
  
  computePhotonMatrices(context);
  
  rc.setImmediateMode(false);
  
  return true;
}

void PhotonSampler::outputPhotonBuffer(){
  // Check if the PhotonBuffer has changed somehow
  std::vector<float> photonData = _photonBuffer.downloadData<float>(GL_SHADER_STORAGE_BUFFER, _photonNumber*32);
  for(uint32_t i = 0; i < _photonNumber; ++i) {
//  std::cout << *(ptr) <<" "<< *(ptr+4) <<" "<< *((ptr)+8) <<" "<< *((ptr)+12) << std::endl;
//  std::cout << *(ptr+1) <<" "<< *((ptr)+5) <<" "<< *((ptr)+9) <<" "<< *((ptr)+13) << std::endl;
//  std::cout << *(ptr+2) <<" "<< *((ptr)+6) <<" "<< *((ptr)+10) <<" "<< *((ptr)+14) << std::endl;
//  std::cout << *(ptr+3) <<" "<< *((ptr)+7) <<" "<< *((ptr)+11) <<" "<< *((ptr)+15) << std::endl;// << std::endl;
  std::cout << "Diffuse: " << photonData[i * 32 +16] <<" "<< photonData[i * 32 +17] <<" "<< photonData[i * 32 +18] <<" "<< photonData[i * 32 +19] << std::endl;
//  std::cout << "Pos: " << *(ptr+20) <<" "<< *((ptr)+21) <<" "<< *((ptr)+22) <<" "<< *((ptr)+23) << std::endl;
//  std::cout << "PosSS: " << *(ptr+ i * 32 +24) <<" "<< *((ptr)+ i * 32 +25) <<" "<< *((ptr)+ i * 32 +26) <<" "<< *((ptr)+ i * 32 +27) << std::endl;
  }
  std::cout << std::endl;
}

void PhotonSampler::setApproximatedScene(Node* root){
  _approxScene = root;
}

void PhotonSampler::setPhotonNumber(uint32_t number){
  _photonNumber = number;
  _needResampling = true;
}

int PhotonSampler::getSamplingTextureSize(){
  return _samplingTextureSize;
}

void PhotonSampler::bindPhotonBuffer(unsigned int location){
  _photonBuffer.bind(GL_SHADER_STORAGE_BUFFER, location);
}

void PhotonSampler::unbindPhotonBuffer(unsigned int location){
  _photonBuffer.unbind(GL_SHADER_STORAGE_BUFFER, location);
}

void PhotonSampler::bindSamplingTexture(Rendering::RenderingContext& rc, unsigned int location){
  rc.pushAndSetTexture(location, _samplingTexture.get());
}

void PhotonSampler::unbindSamplingTexture(Rendering::RenderingContext& rc, unsigned int location){
  rc.popTexture(location);
}

void PhotonSampler::clearPhotonBuffer(){
  static auto format = Rendering::TextureUtils::pixelFormatToGLPixelFormat(Util::PixelFormat::MONO_FLOAT);
  _photonBuffer.clear(GL_SHADER_STORAGE_BUFFER, format.glInternalFormat, format.glLocalDataFormat, format.glLocalDataType);
}

const std::vector<Geometry::Vec2f>& PhotonSampler::getSamplePoints(){
  return _samplePoints;
}

uint32_t PhotonSampler::getPhotonNumber(){
  return _photonNumber;
}

void PhotonSampler::setSamplingStrategy(uint8_t type){
  _samplingStrategy = static_cast<Sampling>(type);
  _needResampling = true;
}

void PhotonSampler::resample(Rendering::RenderingContext& rc){
  uint32_t additionalPhotons = 0;
  _samplePoints.clear();
  
  while(_samplePoints.size() < _photonNumber){
    switch(_samplingStrategy){
      case Sampling::UNIFORM:{
        auto generatedPoints = Sampler::UniformSampler::generateUniformSamples( _photonNumber + additionalPhotons);
        _samplePoints.clear();
        for(auto& point : generatedPoints){
          _samplePoints.push_back(Geometry::Vec2f(point.x, point.y));
        }
      } break;
      case Sampling::POISSON:
      default:{
        Sampler::PoissonGenerator::DefaultPRNG PRNG;
        auto generatedPoints = Sampler::PoissonGenerator::GeneratePoissonPoints( _photonNumber + additionalPhotons, PRNG , 50, false);
        _samplePoints.clear();
        for(auto& point : generatedPoints){
          _samplePoints.push_back(Geometry::Vec2f(point.x, point.y));
        }
      }
    }
    additionalPhotons++;
  }
  
  
  
  std::vector<std::tuple<float, float, size_t>> fPoints;
  for (size_t idx = 0; idx < _samplePoints.size(); idx++) {
    fPoints.push_back(std::make_tuple(_samplePoints[idx].x(), _samplePoints[idx].y(), idx));
  }
  
  std::vector<int> samplingImage;
  size_t additionalSize = 0; 
  while(samplingImage.size() == 0){
    _samplingTextureSize = static_cast<int>(std::ceil(std::sqrt(_samplePoints.size() + additionalSize)));
    samplingImage = computeSamplingImage(fPoints, _samplingTextureSize);
    additionalSize++;
  }
    
  /////////////////// Upload Sample Image containing the photon ID's /////////////////

  std::vector<uint8_t> byteData;

  uint8_t* ptr = reinterpret_cast<uint8_t*>(&(samplingImage[0]));
  for(size_t i = 0; i < samplingImage.size() * sizeof(int); i++){
    byteData.push_back(ptr[i]);
  }
  
  auto NONE = Util::PixelFormat::NONE;
  Util::PixelFormat pf = Util::PixelFormat(Util::TypeConstant::INT32, 0, NONE, NONE, NONE);
  
  Util::Bitmap bitmap(static_cast<const uint32_t>(_samplingTextureSize), static_cast<const uint32_t>(_samplingTextureSize), pf);
  bitmap.setData(byteData);
  bitmap.flipVertically();
  
  _samplingTexture = Rendering::TextureUtils::createTextureFromBitmap(bitmap);
  _samplingTexture->_createGLID(rc);
  _samplingTexture->_uploadGLTexture(rc);
  
  
  std::cout << "Num Sample Points: " << _samplePoints.size() << " Image Size: " << _samplingTextureSize << std::endl;
//  std::cout << "First entry ID: " << samplingImage[0] << " with " << _samplePoints[samplingImage[0]] << std::endl;
  
  
//  std::cout << samplingImage.size() << std::endl;
//  std::cout << "------------------------------------------------------" << std::endl;
//  for(auto point : fPoints){
//    std::cout << std::get<2>(point) <<": ";
//    std::cout << std::fixed << std::setprecision(10) << std::setfill('0') << std::get<0>(point) << " " << std::get<1>(point) << std::endl;
//  }
//  std::cout << "--------static_cast<uint32_t>(----------------------------------------------" << std::endl;
//  auto width = 1280;
//  auto height = 740;
//  for(auto point : fPoints){
//    std::cout << std::get<2>(point) <<": ";
//    auto x = static_cast<uint32_t>(std::get<0>(point) * width);
//    auto y = static_cast<uint32_t>(std::get<1>(point) * height);
//    std::cout << x << " " << y << std::endl;
//  }
  
 // allocateSamplingTexture(samplingImage);
}

Util::Reference<Rendering::Texture> PhotonSampler::getPosTexture(){
  return _posTexture;
}

Util::Reference<Rendering::Texture> PhotonSampler::getNormalTexture(){
  return _normalTexture;
}

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

Util::Reference<Rendering::Texture> PhotonSampler::getSamplingTexture() const { return _samplingTexture; }
Util::Reference<Rendering::Texture> PhotonSampler::getMatrixTexture() const { return _photonMatrixTexture; }

}
}


#endif // MINSG_EXT_THESISSTANISLAW
