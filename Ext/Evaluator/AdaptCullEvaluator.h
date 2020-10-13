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

#ifndef __AdaptCullEvaluator_H
#define __AdaptCullEvaluator_H

#include "Evaluator.h"
#include <map>

namespace Rendering{
class Shader;
}

namespace MinSG{
namespace Evaluators{

/***
 **   AdaptCullEvaluator ---|> Evaluator
 **/
class AdaptCullEvaluator : public Evaluator {
		PROVIDES_TYPE_NAME(AdaptCullEvaluator)
	public:
		MINSGAPI static Rendering::Shader * whiteShader;

		MINSGAPI AdaptCullEvaluator(DirectionMode mode=SINGLE_VALUE);
		MINSGAPI virtual ~AdaptCullEvaluator();

	// ---|> Evaluator
		MINSGAPI virtual void beginMeasure() override;
		MINSGAPI virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & r) override;
		MINSGAPI virtual void endMeasure(FrameContext & context) override;

	private:
		std::map<unsigned int,Node *> objectsInVF;
		std::map<unsigned int,Node *> visibleObjects;
};
}
}
#endif // __AdaptCullEvaluator_H

#endif /* MINSG_EXT_EVALUATORS */
