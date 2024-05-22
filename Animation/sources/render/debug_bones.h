#pragma once

void create_bone_render();
void draw_bone(const mat4 &transform, const vec3 &from, const vec3 &to, vec3 color, float size);
void draw_bone(const vec3 &from, const vec3 &to, vec3 color, float size);


void render_bones(const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light);
