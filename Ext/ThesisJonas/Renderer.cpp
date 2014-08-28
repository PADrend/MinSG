/*
	This file is part of the MinSG library extension ThesisJonas.
	Copyright (C) 2013 Jonas Knoll

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_THESISJONAS

#include "Renderer.h"
#include "Preprocessor.h"

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/BoxHelper.h>
#include <Geometry/Frustum.h>
#include <Geometry/Sphere.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>
#include <Util/References.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <numeric>
#include <random>

namespace MinSG {
namespace ThesisJonas {

static const Util::StringIdentifier triangleBudgetId("ThesisJonas_TriangleBudget");
static const Util::StringIdentifier traversalProjectedSizeId("ThesisJonas_traversalProjectedSize");
static const Util::StringIdentifier updatePrimitiveCountId("ThesisJonas_updatePrimitiveCount");
static const Util::StringIdentifier primitiveCountId("PrimitiveCount");
static const Util::StringIdentifier renderResultId("ThesisJonas_RenderResult");

Renderer::Renderer(Node * n) : 	nodeAttatchedTo(n),
						renderTriangles(true), renderPoints(false),
						renderOriginal(false), traverseAlg(INSIDE_BB),
						traversal_fixedProjectedSize(256),
						dynamicPrimitiveCount(false),
						frustumCulling(false), useTotalBudget(false),
						renderSMwhenTBtooLow(true){
	setAnnotationAttribute(triangleBudgetId);
	setBudget(10000);
}

void Renderer::addRendererToNode(Node * n){
	n->addState(new Renderer(n));
}

static inline uint32_t getChildrenPrimitiveCount(Node * n){
	uint32_t primitiveCount = 0;
	const auto children = getChildNodes(n);
	for(const auto & child : children) {
		primitiveCount += child->getAttribute(primitiveCountId)->toUnsignedInt();
	}
	return primitiveCount;
}

static inline void updatePrimitiveCountAndInformParent(uint32_t count, Node * n){
	const auto cpAttr = dynamic_cast<Util::_NumberAttribute<uint32_t> *>(n->getAttribute(primitiveCountId));
	if(count != cpAttr->toUnsignedInt()) {
		n->setAttribute(primitiveCountId, Util::GenericAttribute::createNumber(count));
	//	cpAttr->set(count);
		dynamic_cast<Util::BoolAttribute *>(n->getParent()->getAttribute(updatePrimitiveCountId))->set(true);
	}
}

NodeRendererResult Renderer::displayNode(FrameContext & context, Node * node, const RenderParam & rp) {
	// TODO: remove following (only for "single triangle drawback" measurement)
	// first version (display as many triangles of original as SamplingMesh has)
//	if(hasSamplingMesh(node)){
//		Rendering::Mesh * m = retrieveSamplingMesh(node);
//		uint32_t indexCount = m->getIndexCount();
//		GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
//		Rendering::Mesh * origM = geoNode->getMesh();
//		if(renderOriginal){
//			m = origM;
//		}
//
//		// display mesh
//		context.getRenderingContext().pushMatrix_modelToCamera();
//		context.getRenderingContext().multMatrix_modelToCamera(node->getRelTransformationMatrix());
//
//		context.displayMesh(m, 0, indexCount);
//
//		context.getRenderingContext().popMatrix_modelToCamera();
//		return NodeRendererResult::NODE_HANDLED;
//	} else {
//		return NodeRendererResult::PASS_ON;
//	}
	// second version (display all triangles)
//	if(hasSamplingMesh(node)){
//		Rendering::Mesh * m = retrieveSamplingMesh(node);
//		GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
//		Rendering::Mesh * origM = geoNode->getMesh();
//		if(renderOriginal)
//			m = origM;
//
//		// display mesh
//		context.getRenderingContext().pushMatrix_modelToCamera();
//		context.getRenderingContext().multMatrix_modelToCamera(node->getRelTransformationMatrix());
//		context.displayMesh(m);
//		context.getRenderingContext().popMatrix_modelToCamera();
//
//		return NodeRendererResult::NODE_HANDLED;
//	} else {
//		return NodeRendererResult::PASS_ON;
//	}
	// remove till here!!!

	// check if node should be handled (active and not outside of frustum [if enabled])
	if(!node->isActive()) {
//		node->setAttribute(renderResultId, Util::GenericAttribute::createString("not handled: node not active"));
		if(dynamicPrimitiveCount)
			updatePrimitiveCountAndInformParent(0, node);
		return NodeRendererResult::NODE_HANDLED;
	}

	Geometry::Box nodeWorldBB(1, 0, 1, 0, 1, 0); // invalid box
	if(frustumCulling){
		nodeWorldBB = Geometry::Helper::getTransformedBox(node->getBB(), node->getWorldTransformationMatrix());
		if(context.getCamera()->testBoxFrustumIntersection(nodeWorldBB)==Geometry::Frustum::OUTSIDE){
//			node->setAttribute(renderResultId, Util::GenericAttribute::createString("not handled: node outside frustum"));
			if(dynamicPrimitiveCount)
				updatePrimitiveCountAndInformParent(0, node);
			return NodeRendererResult::NODE_HANDLED;
		}
	}

	// check if primitive count has to be updated
	if(dynamicPrimitiveCount){
		Util::BoolAttribute * update = dynamic_cast<Util::BoolAttribute *>(node->getAttribute(updatePrimitiveCountId));
		if(update->get()) {
			// update primitive count
			updatePrimitiveCountAndInformParent(getChildrenPrimitiveCount(node), node);
			update->set(false);
		}
	}

	try {
			// calculate if scene graph should be traversed
			bool traverse = false;
			switch(traverseAlg){
				case NO_TRAVERSAL:
					traverse = false;
					break;
				default:
				case INSIDE_BB:
					{
						if(nodeWorldBB.isInvalid()){
							nodeWorldBB = Geometry::Helper::getTransformedBox(node->getBB(), node->getWorldTransformationMatrix());
						}
						const Geometry::Vec3f cameraPos = context.getCamera()->getWorldOrigin();
						traverse = nodeWorldBB.contains(cameraPos);
					}
					break;
				case FIXED_PROJECTED_SIZE:
					{
						const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());
						Geometry::Rect r = context.getProjectedRect(node);
						r.clipBy(screenRect);
						if(r.getArea()>traversal_fixedProjectedSize)
							traverse = true;
					}
					break;
				case FIXED_PROJECTED_SIZE_PREPROCESSED_DPC:
					{
						// 0=traverse, 1=stop traverse (alg), 2=stop traverse (frustum culling)
						static const Util::StringIdentifier stopTraverseId("ThesisJonas_stopTraverse");

						struct DPCAnnotationVisitor : public NodeVisitor {
							const FrameContext & m_context;
							const Util::StringIdentifier & m_primitiveCountId;
							const Util::StringIdentifier & m_stopTraverseId;
							const float m_traversal_fixedProjectedSize;
							DPCAnnotationVisitor(	const FrameContext & p_context,
													const Util::StringIdentifier & p_primitiveCountId,
													const Util::StringIdentifier & p_stopTraverseId,
													const float p_traversal_fixedProjectedSize) :
															m_context(p_context),
															m_primitiveCountId(p_primitiveCountId),
															m_stopTraverseId(p_stopTraverseId),
															m_traversal_fixedProjectedSize(p_traversal_fixedProjectedSize){}

							NodeVisitor::status enter(Node * _node) override {
								// frustum test
								Geometry::Box nodeBB = Geometry::Helper::getTransformedBox(_node->getBB(), _node->getWorldTransformationMatrix());
								if(m_context.getCamera()->testBoxFrustumIntersection(nodeBB)==Geometry::Frustum::OUTSIDE){
									uint32_t tmp = 2;
									_node->setAttribute(m_stopTraverseId, Util::GenericAttribute::createNumber(tmp));
									return BREAK_TRAVERSAL;
								}

								// check and set if node is traversed
								const Geometry::Rect_f screenRect(m_context.getRenderingContext().getWindowClientArea());
								Geometry::Rect r = m_context.getProjectedRect(_node);
								r.clipBy(screenRect);

								if(r.getArea()>m_traversal_fixedProjectedSize){
									uint32_t tmp = 0;
									_node->setAttribute(m_stopTraverseId, Util::GenericAttribute::createNumber(tmp));
									return CONTINUE_TRAVERSAL;
								} else {
									uint32_t tmp = 1;
									_node->setAttribute(m_stopTraverseId, Util::GenericAttribute::createNumber(tmp));

									GeometryNode * geoNode = dynamic_cast<GeometryNode *>(_node);
									if(geoNode != nullptr && !hasSamplingMesh(_node))
										return CONTINUE_TRAVERSAL;
									else
										return BREAK_TRAVERSAL;
								}
							}
							NodeVisitor::status leave(Node * _node) override {
								uint32_t stopTraverse = dynamic_cast<Util::_NumberAttribute<uint32_t> *>(_node->getAttribute(m_stopTraverseId))->toUnsignedInt();
								if(stopTraverse == 2){ // outside frustum
									uint32_t tmp = 0;
									_node->setAttribute(m_primitiveCountId, Util::GenericAttribute::createNumber(tmp));
									return CONTINUE_TRAVERSAL;
								}

								auto geoNode = dynamic_cast<GeometryNode *>(_node);
								bool hasSM = hasSamplingMesh(_node);

								// following cases from below!!!
								uint32_t pc = 0;
								// ----------- frist case -----------
								if(stopTraverse && hasSM) {
									pc = retrieveSamplingMesh(_node)->getPrimitiveCount();

								// ----------- second case -----------
								} else if(stopTraverse && !hasSM){
									if(geoNode == nullptr){
										const auto children = getChildNodes(_node);
										for(const auto & child : children) {
											pc += child->getAttribute(m_primitiveCountId)->toUnsignedInt();
										}
									} else {
										pc = geoNode->getMesh()->getPrimitiveCount();
									}

								// ----------- third case -----------
								} else if(!stopTraverse && geoNode != nullptr){
									pc = geoNode->getMesh()->getPrimitiveCount();

								// ----------- fourth case -----------
								} else /*if(!stopTraverse && geoNode == nullptr)*/{
									const auto children = getChildNodes(_node);
									for(const auto & child : children) {
										pc += child->getAttribute(m_primitiveCountId)->toUnsignedInt();
									}
								}

								_node->setAttribute(m_primitiveCountId, Util::GenericAttribute::createNumber(pc));

								return CONTINUE_TRAVERSAL;
							}
						};

						// check if preprocessing has to be done (no stopTraverse attribute set)
						if(!node->isAttributeSet(stopTraverseId)){
								DPCAnnotationVisitor visitor(context, primitiveCountId, stopTraverseId, traversal_fixedProjectedSize);
								node->traverse(visitor);
						}

						traverse = !(dynamic_cast<Util::_NumberAttribute<uint32_t> *>(node->getAttribute(stopTraverseId))->toUnsignedInt());

						// unset stopTraverse attribute to indicate node was processed
						node->unsetAttribute(stopTraverseId);
					}
					break;
			};

			// retrieve triangle budget
			uint32_t triangleBudget = 0;
			const auto tbAttribute = dynamic_cast<Util::_NumberAttribute<double> *>(node->getAttribute(triangleBudgetId));
			if(tbAttribute != nullptr) {
				triangleBudget = static_cast<uint32_t>(tbAttribute->get());
			}

			// check what should be done (render sampling/original mesh, traverse)
			enum _renderResult{
				RENDER_SAMPLING_MESH,
				RENDER_ORIGINAL_MESH,
				PASS_ON
			} result;
			GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
			const bool hasSM = hasSamplingMesh(node);

			/* render node, 4 distinct cases: 	1. approximate has SamplingMesh
			 *									2. approximate has no SamplingMesh
			 *									3. traverse at geometry node
			 *									4. traverse at other node
			 */
			// ----------- fist case -----------
			if(!traverse && hasSM) {
				// render approximation using sampling mesh
				result = _renderResult::RENDER_SAMPLING_MESH;
				if(dynamicPrimitiveCount)
					updatePrimitiveCountAndInformParent(retrieveSamplingMesh(node)->getPrimitiveCount(), node);

			// ----------- second case -----------
			} else if(!traverse && !hasSM){
				// check if traversing is possible (no geometry node), else approximate from original mesh
				if(geoNode == nullptr){
					// can traverse
					result = _renderResult::PASS_ON;
					if(dynamicPrimitiveCount)
						updatePrimitiveCountAndInformParent(getChildrenPrimitiveCount(node), node);
				} else {
					// can not traverse: render/approximate original mesh
					result = _renderResult::RENDER_ORIGINAL_MESH;
					if(dynamicPrimitiveCount)
						updatePrimitiveCountAndInformParent(geoNode->getMesh()->getPrimitiveCount(), node);
				}

			// ----------- third case -----------
			} else if(traverse && geoNode != nullptr){
				// should traverse but traversing is not possible (geometry node), render node or approximate if necessary

				// check if approximation is necessary (original node has too many triangles)
				if(geoNode->getMesh()->getPrimitiveCount()>triangleBudget){
					// approximation is necessary (original mesh has too many triangles): check if SamplingMesh available
					if(hasSM){
						// TODO: render original mesh if triangleBudget is close to primitive count? Is this the main reason why whole budget is not used?
						// approximate using Sampling mesh
						if(renderSMwhenTBtooLow)
							result = _renderResult::RENDER_SAMPLING_MESH;
						else
							result = _renderResult::RENDER_ORIGINAL_MESH;
						if(dynamicPrimitiveCount)
							updatePrimitiveCountAndInformParent(geoNode->getMesh()->getPrimitiveCount(), node);
					} else {
						// approximate using original mesh
						result = _renderResult::RENDER_ORIGINAL_MESH;
						if(dynamicPrimitiveCount)
							updatePrimitiveCountAndInformParent(geoNode->getMesh()->getPrimitiveCount(), node);
					}
				} else {
					// approximation is not necessary -> render original
					result = _renderResult::RENDER_ORIGINAL_MESH;
					if(dynamicPrimitiveCount)
						updatePrimitiveCountAndInformParent(geoNode->getMesh()->getPrimitiveCount(), node);
				}

			// ----------- fourth case -----------
			} else /*if(traverse && geoNode == nullptr)*/{
				// should traverse and traversing is possible
				result = _renderResult::PASS_ON;
				if(dynamicPrimitiveCount)
					updatePrimitiveCountAndInformParent(getChildrenPrimitiveCount(node), node);
			}


			// handle result
			switch(result){
				case _renderResult::RENDER_SAMPLING_MESH:
					{
						Rendering::Mesh * m = retrieveSamplingMesh(node);
						uint32_t indexCount = std::min(3*triangleBudget, m->getIndexCount());

						if(triangleBudget>0){ // condition is necessary, otherwise VBOs might get drawn and triangle count increases
							context.getRenderingContext().pushMatrix_modelToCamera();
							context.getRenderingContext().multMatrix_modelToCamera(node->getRelTransformationMatrix());

							// draw triangles
							if(renderTriangles)
								context.displayMesh(m, 0, indexCount);

							// draw points of triangles to fill holes
							if(m->getDrawMode()==Rendering::Mesh::DRAW_TRIANGLES && renderPoints){
								m->setDrawMode(Rendering::Mesh::DRAW_POINTS);
								context.displayMesh(m, 0, indexCount);
								m->setDrawMode(Rendering::Mesh::DRAW_TRIANGLES);
							}

							context.getRenderingContext().popMatrix_modelToCamera();
						}

						// use total triangle budget?
						if(useTotalBudget && indexCount/3<triangleBudget){
							if(geoNode==nullptr){
								// distribute remaining triangle budget to children (based on projected size)
//								node->setAttribute(renderResultId, Util::GenericAttribute::createString("render sampling mesh (pass on for total budget use)"));

								// subtract displayed triangles for triangle count to correctly distribute remaining budget
								node->setAttribute(triangleBudgetId, Util::GenericAttribute::createNumber(triangleBudget - indexCount/3));

								// call super function to set annotations for children
								BudgetAnnotationState::displayNode(context, node, rp);
								return NodeRendererResult::PASS_ON;
							} else {
								// render original mesh with remaining budget
								Rendering::Mesh * m_orig = geoNode->getMesh();
								uint32_t indexCount_orig = std::min(3*triangleBudget-indexCount, m_orig->getIndexCount());

								context.getRenderingContext().pushMatrix_modelToCamera();
								context.getRenderingContext().multMatrix_modelToCamera(geoNode->getRelTransformationMatrix());

								// draw triangles
								if(renderTriangles)
									context.displayMesh(m_orig, 0, indexCount_orig);

								// draw points of triangles to fill holes
								if(m_orig->getDrawMode()==Rendering::Mesh::DRAW_TRIANGLES && renderPoints){
									m_orig->setDrawMode(Rendering::Mesh::DRAW_POINTS);
									context.displayMesh(m_orig, 0, indexCount_orig);
									m_orig->setDrawMode(Rendering::Mesh::DRAW_TRIANGLES);
								}

								context.getRenderingContext().popMatrix_modelToCamera();
								return NodeRendererResult::NODE_HANDLED;
							}

						// render original mesh in addition to approximation?
						} else if(renderOriginal) {
//							node->setAttribute(renderResultId, Util::GenericAttribute::createString("render sampling mesh"));
							// call super function to set annotations for children
							BudgetAnnotationState::displayNode(context, node, rp);
							return NodeRendererResult::PASS_ON;
						} else {
//							node->setAttribute(renderResultId, Util::GenericAttribute::createString("render sampling mesh"));
							// call super function to set annotations for children
							BudgetAnnotationState::displayNode(context, node, rp);
							return NodeRendererResult::NODE_HANDLED;
						}
					}

				case _renderResult::RENDER_ORIGINAL_MESH:
					{
						if(triangleBudget>0){ // condition is necessary, otherwise VBOs might get drawn and triangle count increases
							Rendering::Mesh * m = geoNode->getMesh();
							uint32_t indexCount = std::min(3*triangleBudget, m->getIndexCount());

							context.getRenderingContext().pushMatrix_modelToCamera();
							context.getRenderingContext().multMatrix_modelToCamera(geoNode->getRelTransformationMatrix());

							// draw triangles
							if(renderTriangles)
								context.displayMesh(m, 0, indexCount);

							// draw points of triangles to fill holes
							if(m->getDrawMode()==Rendering::Mesh::DRAW_TRIANGLES && renderPoints){
								m->setDrawMode(Rendering::Mesh::DRAW_POINTS);
								context.displayMesh(m, 0, indexCount);
								m->setDrawMode(Rendering::Mesh::DRAW_TRIANGLES);
							}

							context.getRenderingContext().popMatrix_modelToCamera();
						}

//						node->setAttribute(renderResultId, Util::GenericAttribute::createString("render original mesh"));
						// call super function to set annotations for children
						BudgetAnnotationState::displayNode(context, node, rp);
						// render original mesh in addition to approximation?
						if(renderOriginal)
							return NodeRendererResult::PASS_ON;
						else
							return NodeRendererResult::NODE_HANDLED;
					}

				case _renderResult::PASS_ON:
//					node->setAttribute(renderResultId, Util::GenericAttribute::createString("pass on"));
					// call super function to set annotations for children
					BudgetAnnotationState::displayNode(context, node, rp);
					return NodeRendererResult::PASS_ON;

				default:
//					node->setAttribute(renderResultId, Util::GenericAttribute::createString("not handled"));
					// call super function to set annotations for children
					BudgetAnnotationState::displayNode(context, node, rp);
					return NodeRendererResult::NODE_HANDLED;
			}

		} catch(const std::exception & e) {
			WARN(std::string("Exception during rendering: ") + e.what());
			deactivate();

			// Do this here because doDisableState is not called for inactive states.
			doDisableState(context, node, rp);
		}

		return NodeRendererResult::NODE_HANDLED;
}

static void initNodesForDPC(Node * n, bool dpc) {
	if(dpc) {
		// add updatePrimitiveCountId attribute
		n->setAttribute(updatePrimitiveCountId, Util::GenericAttribute::createBool(false));

		// add and initialize primitiveCountId attribute
		auto geoNode = dynamic_cast<GeometryNode *>(n);
		uint32_t primitiveCount = 0;
		if(geoNode != nullptr) {
			const auto mesh = geoNode->getMesh();
			primitiveCount = mesh == nullptr ? 0 : mesh->getPrimitiveCount();
		} else {
			const auto children = getChildNodes(n);
			for(const auto & child : children) {
				primitiveCount += child->getAttribute(primitiveCountId)->toUnsignedInt();
			}
		}
		n->setAttribute(primitiveCountId, Util::GenericAttribute::createNumber(primitiveCount));
	} else {
		// remove updatePrimitiveCountId attribute
		n->unsetAttribute(updatePrimitiveCountId);
		// remove primitiveCountId attribute
		n->unsetAttribute(primitiveCountId);
	}
}

void Renderer::setDynamicPrimitiveCount(bool b) {
	if(dynamicPrimitiveCount != b){
		dynamicPrimitiveCount = b;

		// TODO: 	initialize primitiveCount/informParent needed when set to true
		//			reset primitive count/remove informParent when set to false
		forEachNodeBottomUp<Node>(nodeAttatchedTo, std::bind(&initNodesForDPC, std::placeholders::_1, dynamicPrimitiveCount));
	}
}

void Renderer::setTraverseAlgorithm(traverse_algorithm ta){
	if(ta==FIXED_PROJECTED_SIZE_PREPROCESSED_DPC &&
			!(getDistributionType()==BudgetAnnotationState::DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT ||
			getDistributionType()==BudgetAnnotationState::DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE)	){
		WARN(std::string("TraverseAlgorithm can only be set to 'FIXED_PROJECTED_SIZE_PREPROCESSED_DPC' if TriangleBudgetDistributionType is set to 'DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT(_ITERATIVE)'\n"));
	} else {
		traverseAlg = ta;
	}
}

void Renderer::setTriangleBudgetDistributionType(BudgetAnnotationState::distribution_type_t type){
	if(!(type==BudgetAnnotationState::DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT ||
		type==BudgetAnnotationState::DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE) &&
			traverseAlg==FIXED_PROJECTED_SIZE_PREPROCESSED_DPC){
		WARN(std::string("Before changing TriangleBudgetDistributionType from 'DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT(_ITERATIVE)' TraverseAlgorithm needs to be changes from 'FIXED_PROJECTED_SIZE_PREPROCESSED_DPC'\n"));
	} else {
		setDistributionType(type);
	}
}


Renderer * Renderer::clone() const {
	return new Renderer(*this);
}

}
}

#endif /* MINSG_EXT_THESISJONAS */
