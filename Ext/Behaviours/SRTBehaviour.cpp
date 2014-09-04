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
#include "SRTBehaviour.h"
#include "../../Core/Nodes/Node.h"
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <iostream>
#include <cstdint>
#include <vector>

namespace MinSG {

//! [ctor]
SRTBehaviour::SRTBehaviour(Node *node, const Util::FileName &filename):AbstractNodeBehaviour(node) {
	loadSRTs(filename);
}

//! [dtor]
SRTBehaviour::~SRTBehaviour(){
}

void SRTBehaviour::loadSRTs(const Util::FileName &filename) {
	const std::vector<uint8_t> fileData = Util::FileUtils::loadFile(filename);
	const BinSRT * srtArray = reinterpret_cast<const BinSRT *>(fileData.data());

	srts.clear();
	srts.reserve(fileData.size() / sizeof(BinSRT));

	for (size_t i = 0; i < fileData.size() / sizeof(BinSRT); ++i) {
		const Geometry::Vec3f translation(srtArray[i].trans[0], srtArray[i].trans[1], srtArray[i].trans[2]);
		const Geometry::Matrix3x3f rotation(	srtArray[i].rot[0], srtArray[i].rot[3], srtArray[i].rot[6],
												srtArray[i].rot[1], srtArray[i].rot[4], srtArray[i].rot[7],
												srtArray[i].rot[2], srtArray[i].rot[5], srtArray[i].rot[8]);
		srts.emplace_back(translation, rotation, 1.0f);
	}
	std::cout << "Camera path with " << fileData.size() / sizeof(BinSRT) << " elements loaded.\n";
}

//! ---|> AbstractBehaviour
AbstractBehaviour::behaviourResult_t SRTBehaviour::doExecute(){
	if(!getNode()) return FINISHED;

	int frame = static_cast<int>(floor(getCurrentTime())) % srts.size();
	getNode()->setRelTransformation(srts[frame]);
	return CONTINUE;
}

}
