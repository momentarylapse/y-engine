//
// Created by michi on 7/30/25.
//

#pragma once

#include <lib/ygraphics/graphics-fwd.h>
#include <lib/base/pointer.h>
#include <lib/base/map.h>
#include <lib/os/path.h>


class ShaderManager {
public:
	explicit ShaderManager(Context *ctx);
	Context *ctx;

	shared<Shader> load_shader(const Path& path);
	xfer<Shader> create_shader(const string &source);
	shared<Shader> load_surface_shader(const Path& path, const string &render_path, const string &vertex_module, const string &geometry_module);
	string expand_vertex_shader_source(const string &source, const string &variant);
	string expand_fragment_shader_source(const string &source, const string &render_path);
	string expand_geometry_shader_source(const string &source, const string &variant);
	void load_shader_module(const Path& path);

	xfer<Shader> __load_shader(const Path& path, const string &overwrite_bindings, int overwrite_push_size);
	xfer<Shader> __create_shader(const string& source, const string &overwrite_bindings, int overwrite_push_size);

	Path shader_dir;
	Path default_shader;
	void clear();


	shared_array<Shader> shaders;
	Array<Path> shader_modules;
	base::map<Path,Shader*> shader_map;
	bool ignore_missing_files = false;
};
