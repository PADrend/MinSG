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

#ifndef __AreaEvaluator_H
#define __AreaEvaluator_H

#include "Evaluator.h"
#include <Util/References.h>

namespace Rendering {
class Shader;
}

namespace MinSG{
namespace Evaluators{

/***
 **   AreaEvaluator ---|> Evaluator
 **/
class AreaEvaluator : public Evaluator {
		PROVIDES_TYPE_NAME(AreaEvaluator)
	public:
		static Util::Reference<Rendering::Shader> whiteShader;

		AreaEvaluator(DirectionMode mode=SINGLE_VALUE);
		virtual ~AreaEvaluator();

	// ---|> Evaluator
		virtual void beginMeasure();
		virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & r);
		virtual void endMeasure(FrameContext & context);

	private:
		int whitePixel;
		int allPixel;
};
}
}

#endif // __AreaEvaluator_H

#endif /* MINSG_EXT_EVALUATORS */
