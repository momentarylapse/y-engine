/*
 * Light.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "Light.h"
#include "Camera.h"

Light::Light(const vector &p, const quaternion &q, const color &c, float r, float t) : Entity3D(Type::LIGHT) {
	pos = p;
	ang = q;
	light.pos = p;
	light.dir = q * vector::EZ;
	light.col = c;
	light.radius = r;
	light.theta = t;
	light.harshness = 0.8f;
	if (light.radius >= 0)
		light.harshness = 1;
	enabled = true;
	allow_shadow = false;
	user_shadow_control = false;
	user_shadow_theta = -1;
	type = LightType::DIRECTIONAL;
	if (light.radius > 0) {
		if (light.theta > 0)
			type = LightType::CONE;
		else
			type = LightType::POINT;
	}
}

void Light::__init_parallel__(const quaternion &ang, const color &c) {
	new(this) Light(v_0, ang, c, -1, -1);
}
void Light::__init_spherical__(const vector &p, const color &c, float r) {
	new(this) Light(p, quaternion::ID, c, r, -1);
}
void Light::__init_cone__(const vector &p, const quaternion &ang, const color &c, float r, float t) {
	new(this) Light(p, ang, c, r, t);
}


void Light::set_direction(const vector &dir) {
	ang = quaternion::rotation(dir.dir2ang());
}

void Light::update(Camera *cam, float shadow_box_size, bool using_view_space) {
	if (using_view_space) {
		light.pos = cam->m_view * pos;
		light.dir = cam->ang.bar() * ang * vector::EZ;
	} else {
		light.pos = pos;
		light.dir = ang * vector::EZ;
	}

	if (allow_shadow) {
		if (type == LightType::DIRECTIONAL) {
			vector center = cam->pos + cam->ang*vector::EZ * (shadow_box_size / 3.0f);
			float grid = shadow_box_size / 16;
			center.x -= fmod(center.x, grid) - grid/2;
			center.y -= fmod(center.y, grid) - grid/2;
			center.z -= fmod(center.z, grid) - grid/2;
			auto t = matrix::translation(- center);
			auto r = matrix::rotation(ang).transpose();
			float f = 1 / shadow_box_size;
			auto s = matrix::scale(f, f, f);
			// map onto [-1,1]x[-1,1]x[0,1]
			shadow_projection = matrix::translation(vector(0,0,-0.5f)) * s * r * t;
		} else {
			auto t = matrix::translation(- pos);
			vector dir = - (cam->ang * vector::EZ);
			if (type == LightType::CONE or user_shadow_control)
				dir = - (ang*vector::EZ);
			auto r = matrix::rotation(dir.dir2ang()).transpose();
			//auto r = matrix::rotation(light.dir.dir2ang()).transpose();
			float theta = 1.35f;
			if (type == LightType::CONE)
				theta = light.theta;
			if (user_shadow_control)
				theta = user_shadow_theta;
			auto p = matrix::perspective(2 * theta, 1.0f, light.radius * 0.01f, light.radius);
			shadow_projection = p * r * t;
		}
		if (using_view_space)
			light.proj = shadow_projection * cam->view_matrix().inverse();
		else
			light.proj = shadow_projection;
	}
}
