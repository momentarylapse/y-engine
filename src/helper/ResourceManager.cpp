#include "ResourceManager.h"
#include "../lib/file/file.h"
#include "../lib/nix/nix.h"
#ifdef _X_USE_HUI_
	#include "../lib/hui/hui.h"
#else
	#include "../lib/hui_minimal/Application.h"
	#include "../lib/hui_minimal/error.h"
#endif
#include "../y/EngineData.h"
#include "../graphics-impl.h"


Path ResourceManager::shader_dir;
Path ResourceManager::texture_dir;
Path ResourceManager::default_shader;
static Array<Shader*> shaders;
static Array<Texture*> textures;

Path guess_absolute_path(const Path &filename, const Array<Path> dirs) {
	if (filename.is_absolute())
		return filename;

	for (auto &d: dirs)
		if (file_exists(d << filename))
			return d << filename;

	return Path::EMPTY;
	/*if (engine.ignore_missing_files) {
		msg_error("missing shader: " + filename.str());
		return Shader::load("");
	}
	throw Exception("missing shader: " + filename.str());
	return filename;*/
}

Shader* ResourceManager::load_shader(const Path& filename) {
	if (filename.is_empty())
		return Shader::load("");

	Path fn = guess_absolute_path(filename, {shader_dir, hui::Application::directory_static << "shader"});
	if (fn.is_empty()) {
		if (engine.ignore_missing_files) {
			msg_error("missing shader: " + filename.str());
			return Shader::load("");
		}
		throw Exception("missing shader: " + filename.str());
		//fn = shader_dir << filename;
	}

	for (auto s : shaders)
		if ((s->filename == fn) and (s->program >= 0))
			return s;

	auto s = Shader::load(fn);
	if (!s)
		return nullptr;
	s->link_uniform_block("BoneData", 7);

	shaders.add(s);
	return s;
}

string ResourceManager::expand_vertex_shader_source(const string &source, const string &variant) {
	if (source.find("<VertexShader>") >= 0)
		return source;
	//msg_write("INJECTING " + variant);
	return source + format("\n<VertexShader>\n#import vertex-%s\n</VertexShader>", variant);
}

string ResourceManager::expand_fragment_shader_source(const string &source, const string &render_path) {
	return source.replace("#import surface", "#import surface-" + render_path);
}

Shader* ResourceManager::load_surface_shader(const Path& _filename, const string &render_path, const string &variant) {
	//select_default_vertex_module("vertex-" + variant);
	//return load_shader(filename);
	auto filename = _filename;
	if (filename.is_empty())
		filename = default_shader;



	if (filename.is_empty())
		return Shader::load("");

	Path fn = guess_absolute_path(filename, {shader_dir, hui::Application::directory_static << "shader"});
	if (fn.is_empty()) {
		if (engine.ignore_missing_files) {
			msg_error("missing shader: " + filename.str());
			return Shader::load("");
		}
		throw Exception("missing shader: " + filename.str());
		//fn = shader_dir << filename;
	}

	Path fnx = fn.with(":" + variant + ":" + render_path);

	for (auto s : shaders)
		if ((s->filename == fnx) and (s->program >= 0))
			return s;


	msg_write("loading shader: " + fnx.str());

	string source = expand_vertex_shader_source(FileReadText(fn), variant);
	source = expand_fragment_shader_source(source, render_path);
	auto shader = Shader::create(source);
	shader->filename = fnx;

	//auto s = Shader::load(fn);
	if (variant == "animated")
		if (!shader->link_uniform_block("BoneData", 7))
			msg_error("BoneData not found...");


	if (variant == "instanced")
		if (!shader->link_uniform_block("Multi", 5))
			msg_error("Multi not found...");

	shaders.add(shader);
	return shader;
}

Texture* ResourceManager::load_texture(const Path& filename) {
	if (filename.is_empty())
		return nullptr;

	Path fn = guess_absolute_path(filename, {texture_dir});
	if (fn.is_empty()) {
		if (engine.ignore_missing_files) {
			msg_error("missing texture: " + filename.str());
			return nullptr;
		}
		throw Exception("missing texture: " + filename.str());
	}

	for (auto t: textures)
		if (fn == t->filename)
			return t->valid ? t : nullptr;

	try {
		auto t = Texture::load(fn);
		textures.add(t);
		return t;
	} catch(Exception &e) {
		if (!engine.ignore_missing_files)
			throw;
		msg_error(e.message());
		return nullptr;
	}
}

