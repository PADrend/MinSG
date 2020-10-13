/*
	This file is part of the MinSG library extension KeyFrameAnimation.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2010 David Maicher
	Copyright (C) 2010-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef KEYFRAMEANIMATIONDATA_H_
#define KEYFRAMEANIMATIONDATA_H_

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <map>
#include <vector>

namespace MinSG {
	
//! @ingroup ext
class KeyFrameAnimationData{
	public:
		MINSGAPI KeyFrameAnimationData(Rendering::MeshIndexData _indexData, std::vector<Rendering::MeshVertexData>  _framesData,
				const std::map<std::string, std::vector<int> > _animationData);
		MINSGAPI ~KeyFrameAnimationData();

		const std::map<std::string, std::vector<int> > & getAnimationData() const {
			return animationData;
		}

		const std::vector<Rendering::MeshVertexData> & getFramesData() const {
			return framesData;
		}

		const Rendering::MeshIndexData & getIndexData() const {
			return indexData;
		}

		MINSGAPI KeyFrameAnimationData * clone()const;

	private:
		Rendering::MeshIndexData indexData;
		std::vector<Rendering::MeshVertexData> framesData;
		std::map<std::string, std::vector<int> > animationData;
};

}

#endif /* KEYFRAMEANIMATIONDATA_H_ */
