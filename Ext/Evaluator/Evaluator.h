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

#ifndef __Evaluator_H
#define __Evaluator_H

#include <Util/TypeNameMacro.h>
#include <memory>

namespace Geometry {
template<typename _T> class _Rect;
typedef _Rect<float> Rect;
}

namespace Util {
class GenericAttribute;
class GenericAttributeList;
}

namespace MinSG {
class Node;
class FrameContext;

//! @ingroup ext
namespace Evaluators {

/***
 **   [abstract] Evaluator
 **   Abstract base class for all evaluators.
 **/
class Evaluator {
		PROVIDES_TYPE_NAME(Evaluator)
	public:
		enum DirectionMode {
			SINGLE_VALUE = 0,
			DIRECTION_VALUES = 1
		};

		MINSGAPI Evaluator(DirectionMode mode = SINGLE_VALUE);
		MINSGAPI virtual ~Evaluator();

		inline void setMode(DirectionMode _mode) {
			mode = _mode;
		}
		inline DirectionMode getMode() const {
			return mode;
		}
		const Util::GenericAttribute * getMaxValue() const {
			return maxValue.get();
		}
		// ---o
		virtual void beginMeasure() = 0;
		virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & r) = 0;
		virtual void endMeasure(FrameContext & context) = 0;
		virtual const Util::GenericAttributeList * getResults() {
			return values.get();
		}

	protected:
		MINSGAPI void setMaxValue_i(unsigned int maxValue);
		MINSGAPI void setMaxValue_f(float maxValue);
		MINSGAPI void setMaxValue(Util::GenericAttribute * maxValue);

		DirectionMode mode;

		std::unique_ptr<Util::GenericAttributeList> values;

	private:
		std::unique_ptr<Util::GenericAttribute> maxValue;
};
}
}
#endif // __Evaluator_H

#endif /* MINSG_EXT_EVALUATORS */
