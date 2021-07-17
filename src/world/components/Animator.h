/*
 * Animator.h
 *
 *  Created on: Jul 14, 2021
 *      Author: michi
 */
#pragma once

#include "../../y/Component.h"
#include "../../lib/base/base.h"

class Model;
class vector;
class matrix;
namespace nix {
	class Buffer;
}
class MetaMove;


#define MODEL_MAX_MOVE_OPS				8


// commands for animation (move operations)
class MoveOperation {
public:
	// move operations
	enum class Command {
		SET,			// overwrite
		SET_NEW_KEYED,	// overwrite, if current doesn't equal 0
		SET_OLD_KEYED,	// overwrite, if last equals 0
		ADD_1_FACTOR,	// w = w_old         + w_new * f
		MIX_1_FACTOR,	// w = w_old * (1-f) + w_new * f
		MIX_2_FACTOR	// w = w_old * a     + w_new * b
	};
	int move;
	Command command;
	float time, param1, param2;
};

class Animator : public Component {
public:
	Animator();
	~Animator();

	void on_init() override;

	// move operations
	int num_operations;
	MoveOperation operation[MODEL_MAX_MOVE_OPS];
	MetaMove *meta; // shared

	// dynamical data (own)
	//Mesh *mesh[MODEL_NUM_MESHES]; // here the animated vertices are stored before rendering

	Array<matrix> dmatrix;
	nix::Buffer *buf;

	// animation
	void _cdecl reset();
	bool _cdecl is_done(int operation_no);
	bool _cdecl add_x(MoveOperation::Command cmd, float param1, float param2, int move_no, float &time, float dt, float vel_param, bool loop);
	bool _cdecl add(MoveOperation::Command cmd, int move_no, float &time, float dt, bool loop);
	int _cdecl get_frames(int move_no);
	void _cdecl begin_edit();
	void do_animation(float elapsed);

	void _add_time(int operation_no, float elapsed, float v, bool loop);



	vector get_vertex(int index);

	static const kaba::Class *_class;
};
