/*
 * EngineData.h
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#ifndef SRC_Y_ENGINEDATA_H_
#define SRC_Y_ENGINEDATA_H_


#include "../lib/base/base.h"
#include "../lib/file/path.h"


namespace Gui {
	class Font;
}

class EngineData {
public:
	EngineData();

	string app_name, version;
	bool debug, show_timings, console_enabled, wire_mode;
	float detail_level;
	float detail_factor_inv;
	int shadow_level;
	bool shadow_lower_detail;

	int multisampling;
	bool CullingEnabled, SortingEnabled, ZBufferEnabled;
	bool resetting_game;
	Gui::Font *default_font;
	Path initial_world_file, second_world_file;
	bool physics_enabled, collisions_enabled;
	int mirror_level_max;

	int num_real_col_tests;

	float fps_max, fps_min;
	float time_scale, elapsed, elapsed_rt;

	// the "real world" aspect ratio of the output image (screen or window)
	float physical_aspect_ratio;

	bool first_frame;
	bool game_running;

	bool file_errors_are_critical;

	void set_dirs(const Path &texture_dir, const Path &map_dir, const Path &object_dir, const Path &sound_dir, const Path &script_dir, const Path &material_dir, const Path &font_dir);

	Path map_dir, sound_dir, script_dir, object_dir;
};
extern EngineData engine;



#endif /* SRC_Y_ENGINEDATA_H_ */