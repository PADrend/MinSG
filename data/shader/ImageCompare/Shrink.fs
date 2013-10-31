#version 130

/*
	This file is part of the MinSG library.
	Copyright (C) 2011 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

out vec4 fragColor;

uniform sampler2D A;

void main(void){

	ivec2 pos = ivec2(gl_FragCoord.xy)*2;
		
	vec4 color = texelFetch(A, pos + ivec2(0,0), 0);
	color += texelFetch(A, pos + ivec2(0,1), 0);
	color += texelFetch(A, pos + ivec2(1,0), 0);
	color += texelFetch(A, pos + ivec2(1,1), 0);
	
	fragColor = color/4.0;
}
