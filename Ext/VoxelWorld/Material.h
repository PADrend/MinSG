/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD

#ifndef VOXEL_MATERIAL_H_
#define VOXEL_MATERIAL_H_

#include <Geometry/Vec2.h>
#include <Util/References.h>
#include <Util/Graphics/Color.h>
#include <cstdint>

namespace MinSG {
namespace VoxelWorld{


class Material{
	Util::Color4f color,light;
	float reflectance;
	Geometry::Vec2 textureOffset;
	bool solid;
public:
	Material() : light(0,0,0,0),reflectance(0.5),solid(false){}
	const Util::Color4f& getColor()const	{	return color;	}
	void setColor(const Util::Color4f& c)	{	color = c;	}

	const Util::Color4f& getLight()const	{	return light;	}
	void setLight(const Util::Color4f& c)	{	light = c;	}

	float getReflectance()const				{	return reflectance;	}
	void setReflectance(float f)			{	reflectance = f;	}
	
	
	bool isSolid()const						{	return solid;	}
	void setSolid(bool b)					{	solid = b;	}

};

}
}

#endif /* VOXEL_MATERIAL_H_ */

#endif /* MINSG_EXT_VOXEL_WORLD */
