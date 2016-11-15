/*
	This file is part of the MinSG library.
	Copyright (C) 2016 Stanislaw Eppinger
	Copyright (C) 2016 Sascha Brandt

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#include "PolygonIndexing.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GeometryNode.h"

#include <Geometry/Vec3.h>

#include <Util/Graphics/Color.h>
#include <Util/StringIdentifier.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshUtils.h>

#include <iostream>

namespace MinSG{
namespace ThesisStanislaw{
    
struct IndexingVisitor : public NodeVisitor {
private:
  uint32_t currentID;
  
public:
  bool outputDebug;  
  
  IndexingVisitor() : currentID(0), outputDebug(false){}
  virtual ~IndexingVisitor() {}

  // ---|> NodeVisitor
  NodeVisitor::status enter(Node * node) override {
    GeometryNode * geometry = dynamic_cast<GeometryNode*>(node);
    if (geometry != nullptr && geometry->hasMesh()) {
      if(outputDebug) std::cout << "Start indexing from " << currentID << " to ";
      
      auto mesh = geometry->getMesh();
      auto& vertexData = mesh->openVertexData();
      auto vertexCount = vertexData.getVertexCount();
      auto triangleCount = geometry->getTriangleCount();
      
      Rendering::VertexDescription vertexDesc;
      vertexDesc.appendPosition3D();
      vertexDesc.appendNormalFloat();
      vertexDesc.appendColorRGBFloat();
      vertexDesc.appendUnsignedIntAttribute(Util::StringIdentifier("sg_PolygonID"), static_cast<uint8_t>(1), false);
      
      if(mesh->isUsingIndexData()){
        auto& vertexIndex = mesh->openIndexData();
        auto indexCount = vertexIndex.getIndexCount();
        
        mesh->setUseIndexData(false);
        
        std::vector<Geometry::Vec3> oldPos, oldNormal;
        std::vector<Util::Color4f> oldColor;
        
        {
          auto posAcc = Rendering::PositionAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::POSITION);
          auto norAcc = Rendering::NormalAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::NORMAL);
          auto colAcc = Rendering::ColorAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::COLOR);
          for(uint32_t i = 0; i < vertexCount; i++){
            oldPos.push_back(posAcc->getPosition(i));
            oldNormal.push_back(norAcc->getNormal(i));
            oldColor.push_back(colAcc->getColor4f(i));
          }
        }
        
        vertexData.allocate(triangleCount * 3, vertexDesc);
        
        {
          auto posAcc = Rendering::PositionAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::POSITION);
          auto norAcc = Rendering::NormalAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::NORMAL);
          auto colAcc = Rendering::ColorAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::COLOR);
          auto polIDAcc = Rendering::UIntAttributeAccessor::create(vertexData, Util::StringIdentifier("sg_PolygonID"));
          
          for(uint32_t i = 0; i < indexCount; i++){
              posAcc->setPosition(i, oldPos[vertexIndex[i]]);
              norAcc->setNormal(i, oldNormal[vertexIndex[i]]);
              colAcc->setColor(i, oldColor[vertexIndex[i]]);
              polIDAcc->setValue(i, currentID);
              
              if(i%3 == 0 && i != 0) currentID++;
           }
        }
      } else {
        std::unique_ptr<Rendering::MeshVertexData> newData(Rendering::MeshUtils::convertVertices(vertexData, vertexDesc));
        vertexData.swap(*newData);
        
        auto polIDAcc = Rendering::UIntAttributeAccessor::create(vertexData, Util::StringIdentifier("sg_PolygonID"));
        for(uint32_t i = 0; i < vertexCount; i++){
          if(i%3 == 0 && i != 0) currentID++;
          polIDAcc->setValue(i, currentID);
        }
      }
      
      if(outputDebug){
        auto& indexData = mesh->openIndexData();
        std::cout << currentID << std::endl;
        std::cout
        << "Here is a mesh with triangle count: " << geometry->getTriangleCount() << std::endl
        << "Vertex Count: "<< geometry->getVertexCount() << std::endl
        << "Index Count: " << indexData.getIndexCount() << std::endl
        << "Is Using Index Data: " << mesh->isUsingIndexData() << std::endl 
        << std::endl;
      }
    }
    return CONTINUE_TRAVERSAL;
  }
  
  uint32_t getCurrentID() {return currentID;}
  void resetCurrentID() {currentID = 0;}
  void setDebug(bool v) {outputDebug = v;}
};

void indexPolygons(Node* node, bool debugOutput/*=false*/) {
  if(debugOutput){ std::cout << "Start Indexing Polygons" << std::endl; }
  IndexingVisitor visitor;
  if(debugOutput) visitor.setDebug(true);
  node->traverse(visitor);
  if(debugOutput){ std::cout << "End Indexing Polygons"  << std::endl; }
}

}}

#endif // MINSG_EXT_THESISSTANISLAW
