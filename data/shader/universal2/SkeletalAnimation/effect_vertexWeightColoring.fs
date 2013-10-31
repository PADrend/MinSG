/*
	This file is part of the MinSG library.
	Copyright (C) 2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

struct FragmentColor {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

varying vec4 debugColor;
varying float debug;

void addFragmentEffect(inout FragmentColor color) {
	if(debug > 0.9) {
        color.ambient = debugColor*0.2;
        color.diffuse = debugColor;
        color.specular = debugColor;
    }
}
