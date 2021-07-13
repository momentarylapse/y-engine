/*----------------------------------------------------------------------------*\
| Object                                                                       |
| -> physical entities of a model in the game                                  |
| -> manages physics on its own                                                |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.10.26 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "Object.h"
#include "Material.h"
#include "World.h"
#include "../y/EngineData.h"
#include "../lib/file/file.h"




// neutral object (for terrains,...)
Object::Object()
{
	//msg_right();
	//msg_write("terrain object");
	material.add(new Material);
	script_data.name = "-terrain-";
	visible = false;
	physics_data_.mass = 10000.0f;
//	physics_data.mass_inv = 0;
	prop.radius = 30000000;
	physics_data_.theta_0 = matrix3::ID;
	_matrix = matrix::ID;
	//theta = theta * 10000000.0f;
	physics_data_.active = false;
	physics_data_.passive = true;
//	rotating = false;
//	frozen = true;
//	time_till_freeze = -1;
	//SpecialID=IDFloor;
	//UpdateMatrix();
	//UpdateTheta();
	//msg_left();
}


void Object::make_visible(bool _visible_) {
	if (_visible_ == visible)
		return;
	if (_visible_)
		world.register_model(this);
	else
		world.unregister_model(this);
	visible = _visible_;
}

void Object::update_matrix() {
	auto rot = matrix::rotation_q(ang);
	auto trans = matrix::translation(pos);
	_matrix = trans * rot;
}
