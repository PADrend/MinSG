/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_EVALUATORS

#ifndef __OccOverheadEvaluator_H
#define __OccOverheadEvaluator_H

#include "Evaluator.h"

namespace MinSG{
namespace Evaluators{

/***
 **   OccOverheadEvaluator ---|> Evaluator
 **  (very specialized!) Returns  (normal time rendering octree)-(time rendering only visibleoctree nodes)
 **/
class OccOverheadEvaluator : public Evaluator {
		PROVIDES_TYPE_NAME(OccOverheadEvaluator)
	public:

		MINSGAPI OccOverheadEvaluator(DirectionMode mode=SINGLE_VALUE);
		MINSGAPI virtual ~OccOverheadEvaluator();
	// ---|> Evaluator
		MINSGAPI virtual void beginMeasure() override;
		MINSGAPI virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & r) override;
		MINSGAPI virtual void endMeasure(FrameContext & context) override;

	private:
		float renderingTime;
		unsigned int renderingFlags; //TODO
};
}
}
#endif // __OccOverheadEvaluator_H

#endif /* MINSG_EXT_EVALUATORS */
