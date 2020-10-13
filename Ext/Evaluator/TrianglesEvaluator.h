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

/*
 * This file is part of Benjamin Eikel's Master's thesis.
 * Copyright 2009 Benjamin Eikel
 */

#ifndef __TrianglesEvaluator_H
#define __TrianglesEvaluator_H

#include "Evaluator.h"

namespace Rendering{
class Shader;
}

namespace MinSG {
class GeometryNode;

	namespace Evaluators {
		/**
		 * Evaluator to determine the number of visible triangles.
		 * First the visible GeometryNodes are determined and in a second pass every triangle is tested for visibility using OpenGL occlusion queries.
		 * The result is the overall number of visible triangles.
		 *
		 * @author Benjamin Eikel
		 * @date 2009-07-28
		 */
		class TrianglesEvaluator : public Evaluator {
			PROVIDES_TYPE_NAME(TrianglesEvaluator)
			public:
				//! This uses always Evaluator::SINGLE_VALUE mode.
				MINSGAPI TrianglesEvaluator();
				MINSGAPI virtual ~TrianglesEvaluator();

				// ---|> Evaluator
				MINSGAPI virtual void beginMeasure() override;
				MINSGAPI virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & r) override;
				MINSGAPI virtual void endMeasure(FrameContext & context) override;

				/**
				 * Return the number of visible triangles.
				 *
				 * @return Value of @a numTrianglesVisible
				 */
				MINSGAPI virtual const Util::GenericAttributeList * getResults() override;

			private:
				//! Stores the number of triangles rendered for GeometryNodes.
				size_t numTrianglesRendered;

				//! Stores the number of triangles which were detected visible.
				size_t numTrianglesVisible;

				/**
				 * Return the number of triangles that are visible for the given GeometryNode.
				 *
				 * @param context Current rendering context.
				 * @param node Node which contains the triangles to test.
				 * @return Number of visible triangles of @a node.
				 */
				MINSGAPI static size_t getNumTrianglesVisible(FrameContext & context, GeometryNode * node);
		};
	}
}
#endif // __TrianglesEvaluator_H

#endif /* MINSG_EXT_EVALUATORS */
