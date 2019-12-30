/*----------------------------------------------------------------------------*\
| Camera                                                                       |
| -> representing the camera (view port)                                       |
| -> can be controlled by a camera script                                      |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2009.11.22 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(CAMERA_H__INCLUDED_)
#define CAMERA_H__INCLUDED_


#include "../lib/base/base.h"
#include "../lib/math/math.h"
#include "../meta.h"
class Model;
namespace nix{
	class Texture;
	class Shader;
};

struct CamPoint
{
	int type;
	float duration;
	vector pos, vel, ang;
};

struct PartialModelView
{
	void *p;
	float z;
	int detail;

	bool operator < (const PartialModelView &pmv) const
	{	return z < pmv.z;	}
};

struct PartialModelViewData
{
	Array<PartialModelView> opaque;
	Array<PartialModelView> trans;
};

class Camera : public XContainer
{
public:
	Camera();
	Camera(const vector &pos, const quaternion &ang, const rect &dest);
	virtual ~Camera();
	void reset();
	
	rect dest;
	float z;
	int cam_point_nr;
	Array<CamPoint> cam_point;

	bool enabled;
	bool show;
	nix::Texture *output, *input;
	nix::Shader *shader;
	nix::Shader *override_shader;
	bool shaded_displays;
	float scale_x;

	float min_depth, max_depth;
	
	vector last_pos, pos, view_pos, vel, vel_rt, rot, pos_0, vel_0, ang_0, pos_1, vel_1, ang_1, a_pos, b_pos, a_ang, b_ang, script_rot_0, script_rot_1;
	quaternion ang;
	vector script_ang[2];
	float zoom, el, el_rt, flight_time, flight_time_el;
	matrix *post_projection_matrix;
	bool modal, automatic, real_time, jump_to_pos;
	int auto_over;

	PartialModelViewData pmvd;

	void start();
	void set_view();
	void set_view_local();
	// camera scripts
	void _cdecl start_script(const string &filename, const vector &dpos);
	void _cdecl stop_script();

	Array<plane> clipping_plane;
	Array<Model*> ignore;

	matrix m_projection, m_view;
	matrix m_all, im_all;
	vector _cdecl project(const vector &v);
	vector _cdecl unproject(const vector &v);

	void _cdecl on_iterate(float dt) override;

	void _cdecl __init__();
	void _cdecl __init_ext__(const vector &pos, const quaternion &ang, const rect &dest);
	void _cdecl __delete__() override;
};

void CameraInit();
void CameraReset();
void CameraCalcMove(float dt);
void CameraShiftAll(const vector &dpos);

extern Array<Camera*> cameras;
extern Camera *cam; // "camera"
extern Camera *cur_cam; // currently rendering

#endif

