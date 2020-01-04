/*----------------------------------------------------------------------------*\
| Camera                                                                       |
| -> representing the camera (view port)                                       |
| -> can be controlled by a camera script                                      |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.12.23 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "camera.h"
#include "../meta.h"



#define CPKSetCamPos	0
#define CPKSetCamPosRel	1
#define CPKSetCamAng	2
#define CPKSetCamPosAng	4
#define CPKCamFlight	10
#define CPKCamFlightRel	11



Array<Camera*> cameras;
Camera *cam; // "camera"
Camera *cur_cam; // currently rendering


void CameraInit() {
	CameraReset();
}

void CameraReset() {
	xcon_del(cameras);

	// create the main-view ("cam")
	cam = new Camera(v_0, quaternion::ID, rect::ID);
	cur_cam = cam;
}

void Camera::reset() {

	zoom = 1.0f;
	//scale_x = 1;
	//z = 0.999999f;
	min_depth = 1.0f;
	max_depth = 100000.0f;

	enabled = false;
	dest = rect::ID;

	show = false;
	
	pos = v_0;
	ang = quaternion::ID;

	m_projection = matrix::ID;
	m_view = matrix::ID;
	m_all = matrix::ID;
	im_all = matrix::ID;
}

Camera::Camera(const vector &_pos, const quaternion &_ang, const rect &_dest) {
	reset();
	enabled = true;
	show = true;

	// register
	xcon_reg(this, cameras);

	pos = _pos;
	ang = _ang;
	dest = _dest;
}

Camera::~Camera() {
	// unregister
	xcon_unreg(this, cameras);
}


void Camera::__init__(const vector &_pos, const quaternion &_ang, const rect &_dest) {
	new(this) Camera(_pos, _ang, _dest);
}

void Camera::__delete__() {
	this->~Camera();
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

void Camera::set_view(float aspect_ratio) {
	m_projection = matrix::perspective(pi/4 * zoom, aspect_ratio, min_depth, max_depth);
	m_projection = m_projection * matrix::rotation_x(pi);
	m_view = matrix::rotation_q(ang).transpose() * matrix::translation(-pos);

	m_all = m_projection * m_view;
	im_all = m_all.inverse();
}

vector Camera::project(const vector &v) {
	//auto vv = m_all.project(v);
	float x = m_all._00 * v.x + m_all._01 * v.y + m_all._02 * v.z + m_all._03;
	float y = m_all._10 * v.x + m_all._11 * v.y + m_all._12 * v.z + m_all._13;
	float z = m_all._20 * v.x + m_all._21 * v.y + m_all._22 * v.z + m_all._23;
	float w = m_all._30 * v.x + m_all._31 * v.y + m_all._32 * v.z + m_all._33;
	if (w == 0)
		return vector(0, 0, -1);
	return vector(x/w * 0.5f + 0.5f, 0.5f - y/w * 0.5f, z/w * 0.5f + 0.5f);
}

vector Camera::unproject(const vector &v) {
	float xx = (v.x - 0.5f) * 2;
	float yy = (0.5f - v.y) * 2;
	float zz = (v.z - 0.5f) * 2;
	float x = im_all._00 * xx + im_all._01 * yy + im_all._02 * zz + im_all._03;
	float y = im_all._10 * xx + im_all._11 * yy + im_all._12 * zz + im_all._13;
	float z = im_all._20 * xx + im_all._21 * yy + im_all._22 * zz + im_all._23;
	float w = im_all._30 * xx + im_all._31 * yy + im_all._32 * zz + im_all._33;
	return vector(x, y, z) / w;
}

void CameraShiftAll(const vector &dpos) {
	for (Camera *c: cameras)
		c->pos += dpos;
}

