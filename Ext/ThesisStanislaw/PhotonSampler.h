/*
	This file is part of the MinSG library.
	Copyright (C) 2016 Stanislaw Eppinger
	Copyright (C) 2016 Sascha Brandt

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H
#define MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H


#include <Util/ReferenceCounter.h>

#include <MinSG/Core/FrameContext.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/FBO.h>
#include <Rendering/BufferObject.h>

#include <Geometry/Vec2.h>

#include <tuple>

namespace MinSG{
class Node;
  
namespace ThesisStanislaw{
  
class PhotonSampler {
public:
  enum class Sampling : uint8_t {
    POISSON,
    UNIFORM
  };
  
private:  
  Util::Reference<Rendering::FBO>      _fbo, _photonMatrixFBO;
  Util::Reference<Rendering::Texture>  _depthTexturePhotonMatrix, _photonMatrixTexture;
  Util::Reference<Rendering::Texture>  _depthTexture, _posTexture, _normalTexture;
  bool                                 _fboChanged, _needResampling;
  
  Util::Reference<Rendering::Texture>  _samplingTexture;
  uint32_t                             _samplingTextureSize;
  
  Util::Reference<Rendering::Shader>   _mrtShader;
  Util::Reference<Rendering::Shader>   _photonMatrixShader;
  
  Util::Reference<Node>                _approxScene;
  
  std::vector<Geometry::Vec2f>         _samplePoints;
  uint32_t                             _photonNumber;
  Sampling                             _samplingStrategy;
  Util::Reference<Rendering::Mesh>     _samplingMesh;
  
  Rendering::BufferObject              _photonBuffer;

  bool initializeFBO(FrameContext & context);
  void initializeSamplePointMesh();
  void computePhotonMatrices(FrameContext & context);
  
  float distance(std::pair<float, float> p1, std::pair<float, float> p2);
  std::vector<size_t> getPointsInQuad(const std::vector<std::tuple<float, float, size_t>>& Points, float x1, float y1, float x2, float y2);
  std::pair<size_t, size_t> checkNeighbours(const std::vector<std::tuple<float, float, size_t>>& Points, const std::vector<std::vector<bool>>& occupiedCells, float x1, float y1, float x2, float y2, float offset, size_t x, size_t y, size_t& idxMovingPoint, size_t& idxStayingPoint);
  std::vector<int> computeSamplingImage(std::vector<std::tuple<float, float, size_t>> Points, size_t size);
  
  
public:

  PhotonSampler();
  ~PhotonSampler() = default;
  
  bool computePhotonSamples(FrameContext & context, const RenderParam & rp);
    
  void setShader(const std::string& mrtShaderFile, const std::string& photonShaderFile);    
  void setApproximatedScene(Node* root);
  void setPhotonNumber(uint32_t number);
  void setSamplingStrategy(uint8_t type);
  void resample(Rendering::RenderingContext& rc);
  
  uint32_t getPhotonNumber();
  
  Util::Reference<Rendering::Texture> getPosTexture();
  Util::Reference<Rendering::Texture> getNormalTexture();
  const std::vector<Geometry::Vec2f>& getSamplePoints();
  
  void bindSamplingTexture(Rendering::RenderingContext& rc, unsigned int location);
  void unbindSamplingTexture(Rendering::RenderingContext& rc, unsigned int location);
  int getSamplingTextureSize();
  
  void bindPhotonBuffer(unsigned int location);
  void unbindPhotonBuffer(unsigned int location);
  void clearPhotonBuffer();

  Util::Reference<Rendering::Texture> getSamplingTexture() const;
  Util::Reference<Rendering::Texture> getMatrixTexture() const;
  
  void outputPhotonBuffer();
};

}
}

#endif // MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H
#endif // MINSG_EXT_THESISSTANISLAW
