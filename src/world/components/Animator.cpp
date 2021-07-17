/*
 * Animator.cpp
 *
 *  Created on: Jul 14, 2021
 *      Author: michi
 */

#include "Animator.h"
#include "../Model.h"
#include "../../lib/nix/nix.h"
#include "../../lib/file/msg.h"


const kaba::Class *Animator::_class = nullptr;



MetaMove::MetaMove() {
	num_frames_skeleton = 0;
	num_frames_vertex = 0;
}


Animator::Animator() {
	meta = nullptr;
	buf = nullptr;

	// "auto-animate"
	num_operations = -1;
	operation[0].move = 0;
	operation[0].time = 0;
	operation[0].command = MoveOperation::Command::SET;
	operation[0].param1 = 0;
	operation[0].param2 = 0;
	/*if (anim.meta) {
		for (int i=0; i<anim.meta->move.num; i++)
			if (anim.meta->move[i].num_frames > 0) {
				anim.operation[0].move = i;
				break;
			}
	}*/

}

Animator::~Animator() {
	if (buf)
		delete buf;
}

void Animator::on_init() {
	auto m = get_owner<Model>();

	meta = m->_template->animator->meta;
	buf = new nix::UniformBuffer;

	// skeleton
	dmatrix.resize(get_owner<Model>()->bone.num);
	for (int i=0; i<m->bone.num; i++) {
		dmatrix[i] = matrix::translation(m->bone[i].rest_pos);
	}
}




void Animator::do_animation(float elapsed) {
	auto m = get_owner<Model>();

	// recursion
	for (auto &b: m->bone)
		if (b.model) {
			b.model->_matrix = m->_matrix * b.dmatrix;

			// done by global Animator[] list iteration
			//b.model->do_animation(elapsed);
		}

	if (!meta)
		return;


	// for handling special cases (-1,-2)
	int num_ops = num_operations;

	// "auto-animated"
	if (num_operations == -1){
		// default: just run a single animation
		_add_time(0, elapsed, 0, true);
		num_ops = 1;
	}

	// skeleton edited by script...
	if (num_operations == -2){
		num_ops = 0;
	}

	// make sure we have something to store the animated data in...
#if 0
	for (int s=0;s<MODEL_NUM_MESHES;s++)
		if (_detail_needed_[s]) {
///			anim.vertex[s].resize(mesh[s]->vertex.num);
			//memset(anim.vertex[s], 0, sizeof(vector) * Skin[s]->vertex.num);
///			memcpy(&anim.vertex[s][0], &mesh[s]->vertex[0], sizeof(vector) * mesh[s]->vertex.num);
			int nt = get_num_trias(mesh[s]);
//			anim.normal[s].resize(nt * 3);
			int offset = 0;
			for (int i=0;i<material.num;i++){
				memcpy(&anim.normal[s][offset], &mesh[s]->sub[i].normal[0], sizeof(vector) * mesh[s]->sub[i].num_triangles * 3);
				offset += mesh[s]->sub[i].num_triangles;
			}
		}
#endif


// vertex animation

	bool vertex_animated = false;
	for (int op=0;op<num_ops;op++){
		if (operation[op].move < 0)
			continue;
		Move *m = &meta->move[operation[op].move];
		//msg_write(GetFilename() + format(" %d %d %p %d", op, anim.operation[op].move, m, m->num_frames));
		if (m->num_frames == 0)
			continue;


		if (m->type == AnimationType::VERTEX){
			vertex_animated=true;

			// frame data
			int fr=(int)(operation[op].time); // current frame (relative)
			float dt = operation[op].time-(float)fr;
			int f1 = m->frame0 + fr; // current frame (absolute)
			int f2 = m->frame0 + (fr+1)%m->num_frames; // next frame (absolute)

#if 0
			// transform vertices
			for (int s=0;s<MODEL_NUM_MESHES;s++)
				if (_detail_needed_[s]){
					auto *sk = mesh[s];
					auto *a = anim.mesh[s];
					for (int p=0;p<sk->vertex.num;p++){
						vector dp1 = anim.meta->mesh[s + 1].dpos[f1 * sk->vertex.num + p]; // first value
						vector dp2 = anim.meta->mesh[s + 1].dpos[f2 * sk->vertex.num + p]; // second value
						a->vertex[p] = sk->vertex[p] + dp1*(1-dt) + dp2*dt;
					}
					for (int i=0;i<sk->sub.num;i++)
						sk->sub[i].force_update = true;
				}
#endif
		}
	}

// skeletal animation

	for (int i=0;i<m->bone.num;i++){
		Bone *b = &m->bone[i];

		// reset (only if not being edited by script)
		/*if (Numanim.operations != -2){
			b->cur_ang = quaternion::ID;//quaternion(1, v_0);
			b->cur_pos = b->Pos;
		}*/

		// operations
		for (int iop=0;iop<num_ops;iop++){
			MoveOperation *op = &operation[iop];
			if (op->move < 0)
				continue;
			Move *move = &meta->move[op->move];
			if (move->num_frames == 0)
				continue;
			if (move->type != AnimationType::SKELETAL)
				continue;
			quaternion w,w0,w1,w2,w3;
			vector p,p1,p2;

		// calculate the alignment belonging to this argument
			int fr = (int)(op->time); // current frame (relative)
			int f1 = move->frame0 + fr; // current frame (absolute)
			int f2 = move->frame0 + (fr+1)%move->num_frames; // next frame (absolute)
			float df = op->time-(float)fr; // time since start of current frame
			w1 = meta->skel_ang[f1 * m->bone.num + i]; // first value
			p1 = meta->skel_dpos[f1*m->bone.num + i];
			w2 = meta->skel_ang[f2*m->bone.num + i]; // second value
			p2 = meta->skel_dpos[f2*m->bone.num + i];
			move->inter_quad = false;
			/*if (m->InterQuad){
				w0=m->ang[i][(f-1+m->NumFrames)%m->NumFrames]; // last value
				w3=m->ang[i][(f+2             )%m->NumFrames]; // third value
				// interpolate the current alignment
				w = quaternion::interpolate(w0,w1,w2,w3,df);
				p=(1.0f-df)*p1+df*p2 + SkeletonDPos[i];
			}else*/{
				// interpolate the current alignment
				w = quaternion::interpolate(w1,w2,df);
				p=(1.0f-df)*p1+df*p2 + b->delta_pos;
			}


		// execute the operations

			// overwrite
			if (op->command == op->Command::SET){
				b->cur_ang = w;
				b->cur_pos = p;

			// overwrite, if current doesn't equal 0
			}else if (op->command == op->Command::SET_NEW_KEYED){
				if (w.w!=1)
					b->cur_ang=w;
				if (p!=v_0)
					b->cur_pos=p;

			// overwrite, if last equals 0
			}else if (op->command == op->Command::SET_OLD_KEYED){
				if (b->cur_ang.w==1)
					b->cur_ang=w;
				if (b->cur_pos==v_0)
					b->cur_pos=p;

			// w = w_old         + w_new * f
			}else if (op->command == op->Command::ADD_1_FACTOR){
				w = w.scale_angle(op->param1);
				b->cur_ang = w * b->cur_ang;
				b->cur_pos += op->param1 * p;

			// w = w_old * (1-f) + w_new * f
			}else if (op->command == op->Command::MIX_1_FACTOR){
				b->cur_ang = quaternion::interpolate( b->cur_ang, w, op->param1);
				b->cur_pos = (1 - op->param1) * b->cur_pos + op->param1 * p;

			// w = w_old * a     + w_new * b
			}else if (op->command == op->Command::MIX_2_FACTOR){
				b->cur_ang = b->cur_ang.scale_angle(op->param1);
				w = w.scale_angle(op->param2);
				b->cur_ang = quaternion::interpolate( b->cur_ang, w, 0.5f);
				b->cur_pos = op->param1 * b->cur_pos + op->param2 * p;
			}
		}

		// bone has root -> align to root
		//vector dpos = b->delta_pos;
		auto t0 = matrix::translation(-b->rest_pos);
		if (b->parent >= 0) {
			b->cur_pos = m->bone[b->parent].dmatrix * b->delta_pos;
			//dpos = b->cur_pos - b->rest_pos;
		}

		// create matrices (model -> skeleton)
		auto t = matrix::translation(b->cur_pos);
		auto r = matrix::rotation_q(b->cur_ang);
		b->dmatrix = t * r;
		dmatrix[i] = t * r * t0;
	}

#if 0

	// create the animated data
	if (bone.num > 0)
		for (int s=0;s<MODEL_NUM_MESHES;s++){
			if (!_detail_needed_[s])
				continue;
			auto sk = mesh[s];
			// transform vertices
			for (int p=0;p<sk->vertex.num;p++){
				int b = sk->bone_index[p];
				vector pp = sk->vertex[p] - bone[b].rest_pos;
				// interpolate vertex
				anim.mesh[s]->vertex[p] = bone[b].dmatrix * pp;
				//anim.vertex[s][p]=pp;
			}
			// normal vectors
			int offset = 0;
			for (int mm=0;mm<sk->sub.num;mm++){
				auto sub = &sk->sub[mm];
			#ifdef DynamicNormalCorrect
				for (int t=0;t<sub->num_triangles*3;t++)
					anim.mesh[s]->sub[mm].normal[t] = bone[sk->bone_index[sub->triangle_index[t]]].dmatrix.transform_normal(sub->normal[t]);
					//anim.normal[s][t + offset] = ...
					//anim.normal[s][t + offset]=sub->Normal[t];
			#else
				memcpy(&anim.normal[s][offset], &sub->Normal[0], sub->num_triangles * 3 * sizeof(vector));
			#endif
				sub->force_update = true;
				offset += sub->num_triangles;
			}
		}
#endif



	// update effects
	/*for (int i=0;i<NumFx;i++){
		sEffect **pfx=(sEffect**)fx;
		int nn=0;
		while( (*pfx) ){
			vector vp;
			VecTransform(vp, Matrix, Skin[0]->vertex[(*pfx)->vertex]);
			FxUpdateByModel(*pfx,vp,vp);
			pfx++;
		}
	}*/

}



// reset all animation data for a model (needed in each frame before applying animations!)
void Animator::reset() {
	num_operations = 0;
}

// did the animation reach its end?
bool Animator::is_done(int operation_no) {
	int move_no = operation[operation_no].move;
	if (move_no < 0)
		return true;
	// in case animation doesn't exist
	if (meta->move[move_no].num_frames == 0)
		return true;
	return (operation[operation_no].time >= (float)(meta->move[move_no].num_frames - 1));
}


// dumbly add the correct animation time, ignore animation's ending
void Animator::_add_time(int operation_no, float elapsed, float v, bool loop) {
	auto &op = operation[operation_no];
	int move_no = op.move;
	if (move_no < 0)
		return;
	Move *move = &meta->move[move_no];
	if (move->num_frames == 0)
		return; // in case animation doesn't exist

	// how many frames have passed
	float dt = elapsed * ( move->frames_per_sec_const + move->frames_per_sec_factor * v );

	// add the correct time
	op.time += dt;
	// time may now be way out of range of the animation!!!

	if (is_done(operation_no)) {
		if (loop)
			op.time -= float(move->num_frames) * (int)((float)(op.time / move->num_frames));
		else
			op.time = (float)(move->num_frames) - 1;
	}
}

// apply an animate to a model
//   a new animation "layer" is being added for mixing several animations
//   the variable <time> is being increased
bool Animator::add_x(MoveOperation::Command cmd, float param1, float param2, int move_no, float &time, float dt, float vel_param, bool loop) {
	if (!meta)
		return false;
	if (num_operations < 0){
		num_operations = 0;
	}else if (num_operations >= MODEL_MAX_MOVE_OPS - 1){
		msg_error("Animator.add(): no more than " + i2s(MODEL_MAX_MOVE_OPS) + " animation layers allowed");
		return false;
	}
	int index = -1;
	foreachi (auto &m, meta->move, i)
		if (m.id == move_no)
			index = i;
	if (index < 0) {
		msg_error("move id not existing: " + i2s(move_no));
		return false;
	}
	int n = num_operations ++;
	operation[n].move = index;
	operation[n].command = cmd;
	operation[n].param1 = param1;
	operation[n].param2 = param2;
	operation[n].time = time;

	_add_time(n, dt, vel_param, loop);
	time = operation[n].time;
	return is_done(n);
}

bool Animator::add(MoveOperation::Command cmd, int move_no, float &time, float dt, bool loop) {
	return add_x(cmd, 0, 0, move_no, time, dt, 0, loop);

}

// get the number of frames for a particular animation
int Animator::get_frames(int move_no) {
	if (!meta)
		return 0;
	return meta->move[move_no].num_frames;
}

// edit skelettal animation via script
void Animator::begin_edit() {
	if (!meta)
		return;
	num_operations = -2;
	auto m = get_owner<Model>();
	for (int i=0;i<m->bone.num;i++){
		m->bone[i].cur_ang = quaternion::ID;
		m->bone[i].cur_pos = m->bone[i].delta_pos;
	}
}


vector Animator::get_vertex(int index) {
	auto m = get_owner<Model>();
	auto s = m->mesh[MESH_HIGH];
	int b = s->bone_index[index].i;
	return m->_matrix * dmatrix[b] * s->vertex[index] - m->bone[b].rest_pos;
}
