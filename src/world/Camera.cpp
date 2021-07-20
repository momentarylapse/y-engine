/*----------------------------------------------------------------------------*\
| Camera                                                                       |
| -> representing the camera (view port)                                       |
| -> can be controlled by a camera script                                      |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.12.23 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "Camera.h"
#include "Entity3D.h"
#include "../y/Entity.h"
#include "../lib/math/vector.h"
#include "../lib/math/matrix.h"
#include "../y/EngineData.h"


const kaba::Class *Camera::_class = nullptr;


#define CPKSetCamPos	0
#define CPKSetCamPosRel	1
#define CPKSetCamAng	2
#define CPKSetCamPosAng	4
#define CPKCamFlight	10
#define CPKCamFlightRel	11



Array<Camera*> cameras;
Camera *cam; // "camera"
Camera *cur_cam; // currently rendering

Camera *add_camera(const vector &pos, const quaternion &ang, const rect &dest) {
	auto o = new Entity3D(pos, ang);

	auto c = new Camera(dest);
	o->_add_component_external_(c);
	cameras.add(c);
	return c;
}


void CameraInit() {
	CameraReset();
}

void CameraReset() {
	entity_del(cameras);

	// create the main-view ("cam")
	cam = nullptr;
	cur_cam = cam;
}

Camera::Camera(const rect &_dest) {
	component_type = _class;

	fov = pi / 4;
	exposure = 1.0f;
	bloom_radius = 10;
	bloom_factor = 0.2f;

	focus_enabled = false;
	focal_length = 2000;
	focal_blur = 2;

	//scale_x = 1;
	//z = 0.999999f;
	min_depth = 1.0f;
	max_depth = 1000000.0f;

	m_projection = matrix::ID;
	m_view = matrix::ID;
	m_all = matrix::ID;
	im_all = matrix::ID;

	enabled = true;
	show = true;

	dest = _dest;
}


//void Camera::__init__(const vector &_pos, const quaternion &_ang, const rect &_dest) {
	//new(this) Camera(_pos, _ang, _dest);
//}

void Camera::__delete__() {
	this->Camera::~Camera();
}


void CameraCalcMove(float dt) {
	for (Camera *v: cameras){
		if (!v->enabled)
			continue;
		v->on_iterate(dt);
	}
}

void Camera::on_iterate(float dt) {
}

matrix Camera::projection_matrix(float aspect_ratio) const {
	return matrix::perspective(fov, aspect_ratio, min_depth, max_depth) * matrix::rotation_x(pi);
}

matrix Camera::view_matrix() const {
	auto o = get_owner<Entity3D>();
	return matrix::rotation_q(o->ang).transpose() * matrix::translation(-o->pos);
}

void Camera::update_matrices(float aspect_ratio) {
	m_projection = projection_matrix(aspect_ratio);
	m_view = view_matrix();

	m_all = m_projection * m_view;
	im_all = m_all.inverse();
}

// into [0:R]x[0:1] system!
vector Camera::project(const vector &v) {
	//auto vv = m_all.project(v);
	float x = m_all._00 * v.x + m_all._01 * v.y + m_all._02 * v.z + m_all._03;
	float y = m_all._10 * v.x + m_all._11 * v.y + m_all._12 * v.z + m_all._13;
	float z = m_all._20 * v.x + m_all._21 * v.y + m_all._22 * v.z + m_all._23;
	float w = m_all._30 * v.x + m_all._31 * v.y + m_all._32 * v.z + m_all._33;
	if (w == 0)
		return vector(0, 0, -1);
	return vector((x/w * 0.5f + 0.5f) * engine.physical_aspect_ratio, 0.5f + y/w * 0.5f, z/w * 0.5f + 0.5f);
}

vector Camera::unproject(const vector &v) {
	float xx = (v.x/engine.physical_aspect_ratio - 0.5f) * 2;
	float yy = (v.y - 0.5f) * 2;
	float zz = (v.z - 0.5f) * 2;
	float x = im_all._00 * xx + im_all._01 * yy + im_all._02 * zz + im_all._03;
	float y = im_all._10 * xx + im_all._11 * yy + im_all._12 * zz + im_all._13;
	float z = im_all._20 * xx + im_all._21 * yy + im_all._22 * zz + im_all._23;
	float w = im_all._30 * xx + im_all._31 * yy + im_all._32 * zz + im_all._33;
	return vector(x, y, z) / w;
}

void CameraShiftAll(const vector &dpos) {
	for (auto *c: cameras)
		c->get_owner<Entity3D>()->pos += dpos;
}

