
#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_POLYGONINDEXING_H
#define MINSG_EXT_THESISSTANISLAW_POLYGONINDEXING_H

#include "../../Core/States/NodeRendererState.h"
#include "../../Helper/NodeRendererRegistrationHolder.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../../Rendering/Mesh/Mesh.h"
#include "../../../Rendering/Mesh/MeshVertexData.h"
#include "../../../Rendering/Mesh/VertexDescription.h"
#include "../../../Rendering/Mesh/VertexAttributeAccessors.h"
#include "../../../Rendering/Mesh/VertexAttributeIds.h"
#include "../../../Rendering/MeshUtils/MeshUtils.h"
#include <Util/Graphics/Color.h>
#include <Util/StringIdentifier.h>

namespace MinSG {
  
class FrameContext;
class Node;
enum class NodeRendererResult : bool;
class RenderParam;

namespace ThesisStanislaw {
  
class PolygonIndexingState : public NodeRendererState {
  PROVIDES_TYPE_NAME(PolygonIndexingState)
  
  struct IndexingVisitor : public NodeVisitor {
    uint32_t currentID;

    IndexingVisitor() : currentID(0){}
    virtual ~IndexingVisitor() {}

    // ---|> NodeVisitor
    NodeVisitor::status enter(Node * node) override {
      GeometryNode * geometry = dynamic_cast<GeometryNode*>(node);
      if (geometry != nullptr && geometry->hasMesh()) {
        auto mesh = geometry->getMesh();
        auto& vertexData = mesh->openVertexData();
        auto vertexCount = vertexData.getVertexCount();
        
        Rendering::VertexDescription vertexDesc;
        vertexDesc.appendPosition3D();
        vertexDesc.appendNormalFloat();
        vertexDesc.appendColorRGBFloat();
        vertexDesc.appendUnsignedIntAttribute(Util::StringIdentifier("sg_PolygonID"), static_cast<uint8_t>(1));
        
        Rendering::MeshVertexData & oldData = mesh->openVertexData();
        std::unique_ptr<Rendering::MeshVertexData> newData(Rendering::MeshUtils::convertVertices(vertexData, vertexDesc));
        vertexData.swap(*newData);
        
        vertexData.allocate(vertexCount, vertexDesc);
        
        auto polIDAcc = Rendering::UIntAttributeAccessor::create(vertexData, Util::StringIdentifier("sg_PolygonID"));
        for(uint32_t i = 0; i < vertexCount; i++){
          polIDAcc->setValue(i, currentID++);
        }
        
        std::cout << "Here is a mesh with triangle count: " << geometry->getTriangleCount() << std::endl;
      }
      return CONTINUE_TRAVERSAL;
    }
    
    uint32_t getCurrentID() {return currentID;}
    void resetCurrentID() {currentID = 0;}
  };
  
private:
  bool updatePolygonIDs;
  IndexingVisitor visitor;
  
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
  PolygonIndexingState();

  ~PolygonIndexingState();

  PolygonIndexingState * clone() const override;
};

}
}


#endif // MINSG_EXT_THESISSTANISLAW_POLYGONINDEXING_H
#endif // MINSG_EXT_THESISSTANISLAW
