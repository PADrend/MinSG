#version 120

/*
	This file is part of the MinSG library.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2011-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

struct FragmentColor {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

FragmentColor getBaseColor();
void calcShadedColor(inout FragmentColor color);
void addTexture(inout FragmentColor color);
void addShadow(inout FragmentColor color);
void addFragmentEffect(inout FragmentColor color);

vec4 combineFragmentColor(FragmentColor color);

void main (void) {
	FragmentColor color = getBaseColor();
	calcShadedColor(color);
	addTexture(color);
	addShadow(color);
	addFragmentEffect(color);
	gl_FragColor = combineFragmentColor(color);
}
