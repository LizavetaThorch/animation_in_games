#pragma once

void create_arrow_render();
void draw_arrow(const mat4 &transform, const vec3 &from, const vec3 &to, vec3 color, float size);
void draw_arrow(const vec3 &from, const vec3 &to, vec3 color, float size);
void draw_arrow(const glm::mat4 &tm, vec3 color);


void render_arrows(const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light);