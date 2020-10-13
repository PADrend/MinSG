/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef QUADTREEBUILDER_H_
#define QUADTREEBUILDER_H_

#include "OcTreeBuilder.h"

namespace MinSG {

namespace TreeBuilder {

class QuadTreeBuilder: public OcTreeBuilder {

public:

	MINSGAPI QuadTreeBuilder(Util::GenericAttributeMap & options);
	MINSGAPI virtual ~QuadTreeBuilder();

protected:

	MINSGAPI list_t split(NodeWrapper & source) override;
};

}

}

#endif /* QUADTREEBUILDER_H_ */
