/*
 * Config.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "Config.h"
#include "lib/file/file.h"

Config config;

Config::Config() {
}

void Config::load(const Array<string> &arg) {
	// fixed config
	hui::Configuration::load("game.ini");

	// local config override
	hui::Configuration local;
	local.load("game-local.ini");
	for (auto &k: local.keys())
		set_str(k, local.get_str(k, ""));

	// cli arguments override
	for (auto &a: arg.sub_ref(1)) {
		if (a.head(2).lower() == "-c") {
			auto xx = a.sub_ref(2).explode("=");
			set_str(xx[0].trim(), xx[1].trim());
		} else if (a == "--fw") {
			set_str("renderer.path", "forward");
		} else if (a == "--def") {
			set_str("renderer.path", "deferred");
		} else if (a == "--direct") {
			set_str("renderer.path", "direct");
		} else if (a.head(7) == "--scale") {
			set_str("renderer.resolution-scale-min", a.sub_ref(7));
			set_str("renderer.resolution-scale-max", a.sub_ref(7));
		} else if (a.head(1) != "-") {
			set_str("default-world", a);
		}
	}

	default_world = get_str("default-world", "");
	second_world = get_str("second-world", "");
	main_script = get_str("main-script", "");
	default_font = get_str("default-font", "");
	default_material = get_str("default-material", "");
	debug = get_bool("debug", true);

	string aa = get_str("renderer.antialiasing", "");
	if (aa == "") {
	} else if (aa == "MSAA") {
		antialiasing_method = AntialiasingMethod::MSAA;
	} else if (aa == "TAA") {
		antialiasing_method = AntialiasingMethod::TAA;
	} else {
		msg_error("unknown antialiasing method: " + aa);
	}

	resolution_scale_min = get_float("renderer.resolution-scale-min", 0.5f);
	resolution_scale_max = get_float("renderer.resolution-scale-max", 1.0f);
	target_framerate = get_float("renderer.target-framerate", 60.0f);

	ambient_occlusion_radius = get_float("renderer.ssao.radius", 10);
	if (!get_bool("renderer.ssao.enabled", true))
		ambient_occlusion_radius = -1;
}
