/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef DIRECTIONALINTERPOLATOR_H
#define DIRECTIONALINTERPOLATOR_H

#include <map>

namespace Geometry {
class Frustum;
}
namespace Rendering {
class RenderingContext;
}
namespace Util {
class GenericAttribute;
}
namespace MinSG {
class ValuatedRegionNode;

/**
 * Used to interpolate between the side-values of a ValuatedRegionNode
 * according to a given frustum.
 * \todo add possibility to change assignement_sides_valueIndice ??
 *
 *                     (Cl) Work in progress...
 *
 */
class DirectionalInterpolator{
	public:
		/*! The order of the sides is defined by the measuring script in
			ClassificationAlgo.escript. This is different from Box::side_t! */
		enum side_t {
			LEFT_SIDE = 0,
			FRONT_SIDE = 1,
			RIGHT_SIDE = 2,
			BACK_SIDE = 3,
			BOTTOM_SIDE = 4,
			TOP_SIDE = 5
		};

		/** Default constructor */
		MINSGAPI DirectionalInterpolator();
		/** Default destructor */
		MINSGAPI ~DirectionalInterpolator();

		MINSGAPI Util::GenericAttribute * calculateValue(Rendering::RenderingContext & renderingContext,
					ValuatedRegionNode * node,
					const Geometry::Frustum & frustum,
					float measurementApertureAngle_deg=90.0);

		/**
		 * Calculate the ratio of the visible sides of a cube.
		 * @param ratio The resulting ratios
		 * @param frustum
		 * @param measurementApertureAngle_deg Size in degree of the (possibly) overlapping cube sides
		 */
		MINSGAPI void calculateRatio(Rendering::RenderingContext & renderingContext, float ratio[6],const Geometry::Frustum & frustum,float measurementApertureAngle_deg=90.0 );


		MINSGAPI Util::GenericAttribute * getValueForSide(ValuatedRegionNode * node,side_t side);

		std::map<side_t, int> assignement_sides_valueIndice;
};

}
#endif // DIRECTIONALINTERPOLATOR_H
