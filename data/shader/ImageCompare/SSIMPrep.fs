#version 130

/*
	This file is part of the MinSG library.
	Copyright (C) 2011 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

out vec4 fragColor[3];

uniform sampler2D A, B;

void main(void){

	ivec2 pos = ivec2(gl_FragCoord.xy);

	vec4 a = texelFetch(A, pos, 0);
	vec4 b = texelFetch(B, pos, 0);
	
	fragColor[0] = a*a;
	fragColor[1] = b*b;
	fragColor[2] = a*b;
}
