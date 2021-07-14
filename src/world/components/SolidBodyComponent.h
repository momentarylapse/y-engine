/*
 * SolidBodyComponent.h
 *
 *  Created on: Jul 13, 2021
 *      Author: michi
 */

#pragma once

#include "../../y/Component.h"
#include "../../lib/math/matrix3.h"
#include "../../lib/math/vector.h"

class Model;

class btRigidBody;
class btCollisionShape;


class SolidBodyComponent : public Component {
public:
	SolidBodyComponent(Model *m);
	virtual ~SolidBodyComponent();


	float mass, mass_inv, g_factor;
	matrix3 theta_0, theta, theta_inv;
	bool active, passive;
	bool test_collisions;


	vector force_int, torque_int;
	vector force_ext, torque_ext;

	vector vel, vel_surf, rot, acc;

	bool rotating, moved, frozen;
	float time_till_freeze;


	btRigidBody* body;
	btCollisionShape* col_shape;

	void on_init() override;


	void _cdecl update_data(); // script...

	void update_theta();
	void do_physics(float dt);

	void _cdecl add_force(const vector &f, const vector &rho);
	void _cdecl add_impulse(const vector &p, const vector &rho);
	void _cdecl add_torque(const vector &t);
	void _cdecl add_torque_impulse(const vector &l);

	void _cdecl make_visible(bool visible);
	void update_motion();
	void update_mass();

	static const kaba::Class *_class;
};
