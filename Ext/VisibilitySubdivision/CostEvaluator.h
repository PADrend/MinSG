/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
#ifdef MINSG_EXT_EVALUATORS

#ifndef MINSG_VISIBILITYSUBDIVISION_COSTEVALUATOR_H_
#define MINSG_VISIBILITYSUBDIVISION_COSTEVALUATOR_H_

#include "../Evaluator/Evaluator.h"

namespace MinSG {
namespace VisibilitySubdivision {

/***
 **   CostEvaluator ---|> Evaluator
 **/
/**
 * Evaluator to test the visibility of objects and return a map with rendering
 * costs, where each key in this map is the identifier of an object and the
 * value is it's rendering cost.
 *
 * @author Benjamin Eikel
 * @date 2009-01-25
 */
class CostEvaluator : public Evaluators::Evaluator {
		PROVIDES_TYPE_NAME(CostEvaluator)
	public:

		MINSGAPI CostEvaluator(DirectionMode mode);
		MINSGAPI virtual ~CostEvaluator();

		MINSGAPI void beginMeasure() override;
		MINSGAPI void measure(FrameContext & context, Node & node, const Geometry::Rect & r) override;
		MINSGAPI void endMeasure(FrameContext &) override;
};

}
}

#endif /* MINSG_VISIBILITYSUBDIVISION_COSTEVALUATOR_H_ */

#endif /* MINSG_EXT_EVALUATORS */
#endif /* MINSG_EXT_VISIBILITY_SUBDIVISION */
