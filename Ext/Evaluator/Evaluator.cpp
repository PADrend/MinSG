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

#include "Evaluator.h"
#include <Util/GenericAttribute.h>
#include <cstddef>

namespace MinSG {
namespace Evaluators {

/**
 * [ctor]
 */
Evaluator::Evaluator(DirectionMode _mode/*=SINGLE_VALUE*/): mode(_mode), values(new Util::GenericAttributeList), maxValue() {
	setMaxValue_f(0.0f);
}

/**
 * [dtor]
 */
Evaluator::~Evaluator() = default;

/**
 *
 */
void Evaluator::setMaxValue_f(float _maxValue) {
	setMaxValue(Util::GenericAttribute::createNumber(_maxValue));
}
/**
 *
 */
void Evaluator::setMaxValue_i(unsigned int _maxValue) {
	setMaxValue(Util::GenericAttribute::createNumber(_maxValue));
}
/**
 *
 */
void Evaluator::setMaxValue(Util::GenericAttribute *  newMaxValue) {
	if(newMaxValue == maxValue.get()) {
		return;
	}
	maxValue.reset(newMaxValue);
}

}
}

#endif /* MINSG_EXT_EVALUATORS */
