#version 120

/*
	This file is part of the MinSG library.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

float sg_getPointSize();
vec3 sg_getVertexPosition_ms();
vec3 sg_getVertexNormal_ms();
void sg_provideVertexEffect(inout vec3 position_ms, inout vec3 normal_ms);
vec4 sg_calcPosition_hcs(in vec3 position_ms);
void provideBaseColor();
void provideShadingVars(in vec3 position_ms,in vec3 normal_ms);
void provideTextureVars();
void provideShadowVars();

void main (void) {
	gl_PointSize = sg_getPointSize();

    vec3 position_ms = sg_getVertexPosition_ms();
    vec3 normal_ms = sg_getVertexNormal_ms();
    sg_provideVertexEffect(position_ms, normal_ms);
	gl_Position = sg_calcPosition_hcs(position_ms);
	provideBaseColor();
	provideShadingVars(position_ms, normal_ms);
	provideTextureVars();
	provideShadowVars();
}
