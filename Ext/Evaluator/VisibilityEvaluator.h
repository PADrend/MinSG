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

#ifndef __VisibilityEvaluator_H
#define __VisibilityEvaluator_H

#include "Evaluator.h"
#include <map>

namespace Rendering{
class Shader;
}

namespace MinSG{
namespace Evaluators{

/*! Measures the number of visible GeometryNodes or the number of polygons contained in visible GeometryNodes.
	If mode is SINGLE_VALUE, each Node is at most counted once, even if it
	is visible from multiple sides.

	VisibilityEvaluator ---|> Evaluator
 */
class VisibilityEvaluator : public Evaluator {
		PROVIDES_TYPE_NAME(VisibilityEvaluator)
	public:
		MINSGAPI static Rendering::Shader * whiteShader;

		MINSGAPI explicit VisibilityEvaluator(DirectionMode mode=SINGLE_VALUE,bool _countPolygons=false);
		MINSGAPI virtual ~VisibilityEvaluator();

		bool doesCountPolygons()const	{	return countPolygons;	}
		void setCountPolygons(bool b)	{	countPolygons=b;	}

	// ---|> Evaluator
		MINSGAPI virtual void beginMeasure() override;
		MINSGAPI virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & r) override;
		MINSGAPI virtual void endMeasure(FrameContext & context) override;

	private:
		std::map<uintptr_t,Node *> objectsInVF;
		std::map<uintptr_t,Node *> visibleObjects;
		bool countPolygons;

};
}
}
#endif // __VisibilityEvaluator_H

#endif /* MINSG_EXT_EVALUATORS */
