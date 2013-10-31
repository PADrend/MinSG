/*
	This file is part of the MinSG library extension ThesisJonas.
	Copyright (C) 2013 Jonas Knoll

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_THESISJONAS

#ifndef MINSG_THESISJONAS_RENDERER_H_
#define MINSG_THESISJONAS_RENDERER_H_

#include "../States/BudgetAnnotationState.h"
#include "../../Core/States/NodeRendererState.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include <Util/TypeNameMacro.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <utility>

namespace MinSG {
class FrameContext;
class GeometryNode;
class GroupNode;
class Node;
class RenderParam;
class AbstractCameraNode;
namespace ThesisJonas{

class Renderer : public BudgetAnnotationState {
		PROVIDES_TYPE_NAME(ThesisJonas::Renderer)
	public:
		enum traverse_algorithm {
			NO_TRAVERSAL,
			INSIDE_BB,
			FIXED_PROJECTED_SIZE,
			FIXED_PROJECTED_SIZE_PREPROCESSED_DPC
		};

	private:
		NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		Node * nodeAttatchedTo;
		bool renderTriangles;
		bool renderPoints;
		bool renderOriginal;
		traverse_algorithm traverseAlg;
		float traversal_fixedProjectedSize;
		bool dynamicPrimitiveCount;
		bool frustumCulling;
		bool useTotalBudget;
		bool renderSMwhenTBtooLow;

	protected:
		Renderer(Node * n);

	public:
		static void addRendererToNode(Node * n);

		void setRenderTriangles(bool b) {renderTriangles = b;}
		bool getRenderTriangles() const {return renderTriangles;}

		void setRenderPoints(bool b) {renderPoints = b;}
		bool getRenderPoints() const {return renderPoints;}

		void setRenderOriginal(bool b) {renderOriginal = b;}
		bool getRenderOriginal() const {return renderOriginal;}

		void setTraverseAlgorithm(traverse_algorithm ta);
		traverse_algorithm getTraverseAlgorithm() const {return traverseAlg;}

		void setProjectedSize(float f) {traversal_fixedProjectedSize = f;}
		float getProjectedSize() const {return traversal_fixedProjectedSize;}

		void setDynamicPrimitiveCount(bool b);
		bool getDynamicPrimitiveCount() const {return dynamicPrimitiveCount;}

		void setFrustumCulling(bool b) {frustumCulling = b;}
		bool getFrustumCulling() const {return frustumCulling;}

		void setUseTotalBudget(bool b) {useTotalBudget = b;}
		bool getUseTotalBudget() const {return useTotalBudget;}

		void setRenderSMwhenTBtooLow(bool b) {renderSMwhenTBtooLow = b;}
		bool getRenderSMwhenTBtooLow() const {return renderSMwhenTBtooLow;}

		void setTriangleBudget(double d) {setBudget(d);}
		double getTriangleBudget() const {return getBudget();}

		void setTriangleBudgetDistributionType(BudgetAnnotationState::distribution_type_t type);
		BudgetAnnotationState::distribution_type_t getTriangleBudgetDistributionType() const {return getDistributionType();}

		Renderer * clone() const override;
};
}
}

#endif /* MINSG_THESISJONAS_RENDERER_H_ */

#endif /* MINSG_EXT_THESISJONAS */
