/*
	This file is part of the MinSG library extension Behaviours.
	Copyright (C) 2011 Sascha Brandt
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SRTBEHAVIOUR_H
#define SRTBEHAVIOUR_H

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include <Util/IO/FileName.h>
#include <Geometry/SRT.h>
#include <vector>

namespace MinSG {
/**
 * SRTBehaviour ---|> AbstractNodeBehaviour
 * @ingroup behaviour
 */
class SRTBehaviour : public AbstractNodeBehaviour {
	PROVIDES_TYPE_NAME(SRTBehaviour)

	struct BinSRT {
		float rot[9];
		float trans[3];
	};

	MINSGAPI void loadSRTs(const Util::FileName &filename);

	public:
		MINSGAPI SRTBehaviour(Node * node, const Util::FileName &filename);
		MINSGAPI virtual ~SRTBehaviour();

		MINSGAPI behaviourResult_t doExecute() override;
		size_t getSize() { return srts.size(); }
	protected:
		std::vector<Geometry::SRTf> srts;
};

}
#endif
