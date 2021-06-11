#include "ResourceManager.h"
#include "../lib/nix/nix.h"
#include "../lib/hui_minimal/Application.h"
#include "../lib/hui_minimal/error.h"
#include "../y/EngineData.h"


Path ResourceManager::shader_dir;
Path ResourceManager::texture_dir;
static shared_array<nix::Shader> shaders;

nix::Shader* ResourceManager::load_shader(const Path& filename) {
	if (filename.is_empty())
		return nix::Shader::load("");

	Path fn;
	if (filename.is_absolute()) {
		fn = filename;
	} else if (file_exists(shader_dir << filename)) {
		fn = shader_dir << filename;
	} else if (file_exists(hui::Application::directory_static << filename)) {
		fn = hui::Application::directory_static << filename;
	} else {
		if (engine.ignore_missing_files) {
			msg_error("missing shader: " + filename.str());
			return nix::Shader::load("");
		}
		throw Exception("missing shader: " + filename.str());
		//fn = shader_dir << filename;
	}

	for (auto s : weak(shaders))
		if ((s->filename == fn) and (s->program >= 0))
			return s;
	return nix::Shader::load(fn);
}

nix::Texture* ResourceManager::load_texture(const Path& filename) {
	return nix::Texture::load(filename);
	if (filename.is_empty())
		return nullptr;
	try {
		return nix::Texture::load(filename);
	} catch (Exception& e) {
		if (engine.ignore_missing_files)
			msg_error("missing texture: " + filename.str());
		else
			throw;
	}
	return nullptr;
}

