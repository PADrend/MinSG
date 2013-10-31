#version 130

/*
	This file is part of the MinSG library.
	Copyright (C) 2011 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

const float c1 = 0.01*0.01; // (dynamic range * 0.01)^2
const float c2 = 0.03*0.03; // (dynamic range * 0.03)^2

out vec4 fragColor;

uniform sampler2D A;
uniform sampler2D B;
uniform sampler2D AA;
uniform sampler2D BB;
uniform sampler2D AB;

void main(void){

	ivec2 pos = ivec2(gl_FragCoord.xy);

	vec4 a = texelFetch(A, pos, 0);
	vec4 b = texelFetch(B, pos, 0);
	vec4 aa = texelFetch(AA, pos, 0);
	vec4 bb = texelFetch(BB, pos, 0);
	vec4 ab = texelFetch(AB, pos, 0);

	vec4 sx = aa - a*a;
	vec4 sy = bb - b*b;
	vec4 sxy = ab - a*b;

	fragColor = ((a*b*2 + c1)*(sxy*2 + c2)) / ((a*a + b*b + c1)*(sx + sy + c2));
}
