#include "ResourceManager.h"
#include "../lib/nix/nix.h"
#include "../lib/hui_minimal/Application.h"
#include "../lib/hui_minimal/error.h"


Path ResourceManager::shader_dir;
Path ResourceManager::texture_dir;
static shared_array<nix::Shader> shaders;
static const Path FAKE_ENGINE_PATH = ":E:";

nix::Shader* ResourceManager::load_shader(const Path& filename) {
	if (filename.is_empty())
		return nix::Shader::load("");

	Path fn;
	if (filename.is_absolute()) {
		fn = filename;
	} else if (filename.is_in(FAKE_ENGINE_PATH)) {
		fn = hui::Application::directory_static << filename.relative_to(FAKE_ENGINE_PATH);
	} else {
		fn = shader_dir << filename;
	}

	for (auto s : weak(shaders))
		if ((s->filename == fn) and (s->program >= 0))
			return s;
	//hui::ShowError(fn.str());
	return nix::Shader::load(fn);
}

nix::Texture* ResourceManager::load_texture(const Path& path) {
	return nix::Texture::load(path);
}

