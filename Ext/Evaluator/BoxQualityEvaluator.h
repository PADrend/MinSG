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

#ifndef __BoxQualityEvaluator_H
#define __BoxQualityEvaluator_H

#include "Evaluator.h"
#include <map>

namespace Rendering{
class Shader;
}

namespace MinSG {
class GeometryNode;

namespace Evaluators {

/*! Measures: #as visible classified GeometryNodes (these have visible boundigBoxes)
 *            #visible GeometryNodes
 *            #triangles in visible boundingBoxes
 *            #triangles in visible GeometryNodes
 *
 *  BoxQualityEvaluator ---|> Evaluator
 */
class BoxQualityEvaluator : public Evaluator {
		PROVIDES_TYPE_NAME(BoxQualityEvaluator)
	public:
		MINSGAPI BoxQualityEvaluator();
		MINSGAPI virtual ~BoxQualityEvaluator();

	// ---|> Evaluator
		MINSGAPI virtual void beginMeasure() override;
		MINSGAPI virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & r) override;
		MINSGAPI virtual void endMeasure(FrameContext & context) override;

		// constants determining position of the corresponding results in GenericAttrubuteList values.
		static const uint8_t OBJECTS_CLASSIFIED_AS_VISIBLE = 0; //equivalent to visible boxes
		static const uint8_t OBJECTS_VISIBLE = 1;
		static const uint8_t TRIANGLES_IN_VISIBLE_BOXES = 2;
		static const uint8_t TRIANGLES_IN_VISIBLE_OBJECTS = 3;

	private:
		std::map<uintptr_t, GeometryNode*> objectsInVF;
		std::map<uintptr_t, GeometryNode*> objectsClassifiedAsV;
		std::map<uintptr_t, GeometryNode*> objectsVisible;
};
}
}
#endif // __BoxQualityEvaluator_H

#endif /* MINSG_EXT_EVALUATORS */
