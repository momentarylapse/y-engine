/*----------------------------------------------------------------------------*\
| Nix textures                                                                 |
| -> texture loading and handling                                              |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2008.11.09 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if HAS_LIB_GL

#include "nix.h"
#include "nix_common.h"
#include "../image/image.h"

// management:
//  Texture.load()
//    -> managed, shared<>, DON'T DELETE
//  new Texture()
//    -> unmanaged, needs manual delete

namespace nix{

shared_array<Texture> textures;
Texture *default_texture = nullptr;
Texture *tex_text = nullptr;
int tex_cube_level = -1;


const unsigned int NO_TEXTURE = 0xffffffff;


//--------------------------------------------------------------------------------------------------
// common stuff
//--------------------------------------------------------------------------------------------------

void init_textures() {

	default_texture = new Texture;
	Image image;
	image.create(16, 16, White);
	default_texture->overwrite(image);

	tex_text = new Texture;
}

void release_textures() {
	for (Texture *t: weak(textures)) {
		//glBindTexture(GL_TEXTURE_2D, t->texture);
		glDeleteTextures(1, &t->texture);
	}
}

void reincarnate_textures() {
	for (Texture *t: weak(textures)) {
		//glGenTextures(1, &t->texture);
		t->reload();
	}
}


struct FormatData {
	unsigned int internal_format;
	unsigned int components, x;
};

FormatData parse_format(const string &_format) {
	if (_format == "r:i8")
		return {GL_R8, GL_RED, GL_UNSIGNED_BYTE};
	if (_format == "rgb:i8")
		return {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE};
	if (_format == "rgba:i8")
		return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
	if (_format == "r:f32")
		return {GL_R32F, GL_RED, GL_FLOAT};
	if (_format == "rgba:f32")
		return {GL_RGBA32F, GL_RGBA, GL_FLOAT};
	if (_format == "r:f16")
		return {GL_R16F, GL_RED, GL_HALF_FLOAT};
	if (_format == "rgba:f16")
		return {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT};

	msg_error("unknown format: " + _format);
	return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
}

Texture::Texture() {
	filename = "-empty-";
	type = Type::NONE;
	internal_format = 0;
	valid = true;
	width = height = nz = samples = 0;
	texture = NO_TEXTURE;
}


void Texture::_create_2d(int w, int h, const string &_format) {
	msg_write(format("creating texture [%d x %d: %s] ", w, h, _format));
	width = w;
	height = h;
	type = Type::DEFAULT;

	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	auto d = parse_format(_format);
	internal_format = d.internal_format;
	glTextureStorage2D(texture, 1, internal_format, width, height);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

Texture::Texture(int w, int h, const string &_format) : Texture() {
	_create_2d(w, h, _format);
}

Texture::Texture(int w, int h, int _nz, const string &_format) : Texture() {
	msg_write(format("creating texture [%d x %d x %d: %s] ", w, h, _nz, _format));
	width = w;
	height = h;
	nz = _nz;
	type = Type::VOLUME;

	glCreateTextures(GL_TEXTURE_3D, 1, &texture);
	glBindTexture(GL_TEXTURE_3D, texture);
	auto d = parse_format(_format);
	internal_format = d.internal_format;
	glTextureStorage3D(texture, 1, internal_format, width, height, nz);
	//glTexImage3D(GL_TEXTURE_3D, 0, internal_format, width, height, nz, 0, d.components, d.x, 0);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_REPEAT);
}

Texture::~Texture() {
	unload();
	/*foreachi(auto t, textures, i)
		if (t == this) {
			textures.erase(i);
			break;
		}*/
}

void Texture::__init__(int w, int h, const string &f) {
	new(this) Texture(w, h, f);
}

void Texture::__init3__(int nx, int ny, int nz, const string &f) {
	new(this) Texture(nx, ny, nz, f);
}

void Texture::__delete__() {
	this->~Texture();
}

void TextureClear() {
	msg_error("Texture Clear");
	for (Texture *t: weak(textures))
		msg_write(t->filename.str());
}

Texture *Texture::load(const Path &filename) {
	if (filename.is_empty())
		return nullptr;

	// test existence
	if (!file_exists(filename))
		throw Exception("texture file does not exist: " + filename.str());

	Texture *t = new Texture;
	try {
		t->filename = filename;
		t->reload();
		textures.add(t);
		return t;
	} catch (...) {
		delete t;
	}
	return nullptr;
}

void Texture::reload() {
	msg_write("loading texture: " + filename.str());

	// test the file's existence
	if (!file_exists(filename))
		throw Exception("texture file does not exist!");

	string extension = filename.extension();
	auto image = Image::load(filename);
	overwrite(*image);
	delete image;
}

void Texture::_overwrite(int target, int subtarget, const Image &image) {
	if (image.error)
		return;

	if (width != image.width or height != image.height) {
		//msg_write("texture resize..." + filename.str());
		glDeleteTextures(1, &texture);
		_create_2d(image.width, image.height, "rgba:i8");
		//glTextureStorage2D(texture, 1, internal_format, width, height);
	}

	image.set_mode(Image::Mode::RGBA);

	if (type == Type::CUBE)
		target = GL_TEXTURE_CUBE_MAP;

	//glEnable(target);
//	glBindTexture(target, texture);


	glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.data.data);

	//if (image.alpha_used) {
//		internal_format = GL_RGBA8;
//		glTexImage2D(subtarget, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data.data);
	//} else {
	//	internal_format = GL_RGB8;
	//	glTexImage2D(subtarget, 0, GL_RGB8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data.data);
	//}
	if (type == Type::DEFAULT)
		glGenerateTextureMipmap(texture);
		//glGenerateMipmap(GL_TEXTURE_2D);

//msg_todo("32 bit textures for OpenGL");
	//gluBuild2DMipmaps(subtarget,4,NixImage.width,NixImage.height,GL_RGBA,GL_UNSIGNED_BYTE,NixImage.data);
	//glTexImage2D(subtarget,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,NixImage.data);
	//glTexImage2D(subtarget,0,4,256,256,0,4,GL_UNSIGNED_BYTE,NixImage.data);
}

void Texture::set_options(const string &options) const {
	for (auto &x: options.explode(",")) {
		auto y = x.explode("=");
		if (y.num != 2)
			throw Exception("key=value expected: " + x);
		string key = y[0];
		string value = y[1];
		if (key == "wrap") {
			if (value == "repeat") {
				//glBindTexture(0, texture);
				glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
			} else if (value == "clamp") {
				glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			} else {
				throw Exception("unknown value for key: " + x);
			}
		} else if ((key == "magfilter") or (key == "minfilter")) {
			auto filter = (key == "magfilter") ? GL_TEXTURE_MAG_FILTER : GL_TEXTURE_MIN_FILTER;
			if (value == "linear") {
				glTextureParameteri(texture, filter, GL_LINEAR);
			} else if (value == "nearest") {
				glTextureParameteri(texture, filter, GL_NEAREST);
			} else if (value == "trilinear") {
				glTextureParameteri(texture, filter, GL_LINEAR_MIPMAP_LINEAR);
			} else {
				throw Exception("unknown value for key: " + x);
			}
		} else {
			throw Exception("unknown key: " + key);
		}
	}
}

void Texture::overwrite(const Image &image) {
	if (type == Type::NONE)
		_create_2d(image.width, image.height, "rgba:i8");

	_overwrite(GL_TEXTURE_2D, GL_TEXTURE_2D, image);
}

void Texture::read(Image &image) {
	set_texture(this);
	image.create(width, height, Black);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data.data);
}

void Texture::read_float(Array<float> &data) {
	set_texture(this);
	if ((internal_format == GL_R8) or (internal_format == GL_R32F)) {
		data.resize(width * height);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, data.data); // 1 channel
	} else {
		data.resize(width * height * 4);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data.data); // 4 channels
	}
}

void Texture::write_float(Array<float> &data, int nx, int ny, int nz) {
	set_texture(this);
	if (type == Type::VOLUME) {
		if ((internal_format == GL_R8) or (internal_format == GL_R32F)) {
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, nx, ny, nz, 0, GL_RED, GL_FLOAT, &data[0]);
		} else {
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, nx, ny, nz, 0, GL_RGBA, GL_FLOAT, &data[0]);
		}
	} else {
		if ((internal_format == GL_R8) or (internal_format == GL_R32F)) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, nx, ny, 0, GL_RED, GL_FLOAT, &data[0]);
			//glSetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, data.data); // 1 channel
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nx, ny, 0, GL_RGBA, GL_FLOAT, &data[0]);
			//glSetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data.data); // 4 channels
		}
	}
}

void Texture::unload() {
	if (type != Type::NONE) {
		msg_write("unloading texture: " + filename.str());
		glDeleteTextures(1, &texture);
	}
}

void set_texture(Texture *t) {
	//refresh_texture(t);
	if (!t)
		t = default_texture;

	tex_cube_level = -1;
	/*glActiveTexture(GL_TEXTURE0);
	if (t->type == Texture::Type::CUBE){
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, t->texture);
		tex_cube_level = 0;
	} else if (t->type == Texture::Type::IMAGE){
		glBindTexture(GL_TEXTURE_2D, t->texture);
		glBindImageTexture(0, t->texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, t->internal_format);
	} else if (t->type == Texture::Type::VOLUME){
		glBindTexture(GL_TEXTURE_3D, t->texture);
	} else if (t->type == Texture::Type::MULTISAMPLE){
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, t->texture);
	} else {
		glBindTexture(GL_TEXTURE_2D, t->texture);
	}*/

	if (t->type == t->Type::CUBE)
		tex_cube_level = 0;
	glBindTextureUnit(0, t->texture);
}

void set_textures(const Array<Texture*> &textures) {
	/*for (int i=0;i<num_textures;i++)
		if (texture[i] >= 0)
			refresh_texture(texture[i]);*/

	tex_cube_level = -1;
	for (int i=0; i<textures.num; i++) {
		auto t = textures[i];
		if (!t)
			t = default_texture;

		if (t->type == t->Type::CUBE)
			tex_cube_level = i;

		/*glActiveTexture(GL_TEXTURE0+i);
		if (t->type == t->Type::CUBE) {
			glBindTexture(GL_TEXTURE_CUBE_MAP, t->texture);
			tex_cube_level = i;
		} else if (t->type == Texture::Type::IMAGE){
			glBindTexture(GL_TEXTURE_2D, t->texture);
			glBindImageTexture(0, t->texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, t->internal_format);
		} else if (t->type == t->Type::VOLUME) {
			glBindTexture(GL_TEXTURE_3D, t->texture);
		} else if (t->type == t->Type::MULTISAMPLE) {
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, t->texture);
		} else {
			glBindTexture(GL_TEXTURE_2D, t->texture);
		}*/
		glBindTextureUnit(i, t->texture);
	}
}



TextureMultiSample::TextureMultiSample(int w, int h, int _samples, const string &_format) : Texture() {
	msg_write(format("creating texture [%d x %d, %d samples: %s]", w, h, _samples, _format));
	width = w;
	height = h;
	samples = _samples;
	type = Type::MULTISAMPLE;
	filename = "-multisample-";

	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &texture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
	auto d = parse_format(_format);
	internal_format = d.internal_format;
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internal_format, width, height, GL_TRUE);
	//glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, d.components, d.x, 0);
	//glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
}

ImageTexture::ImageTexture(int _width, int _height, const string &_format) {
	msg_write(format("creating image texture [%d x %d: %s] ", _width, _height, _format));
	filename = "-image-";
	width = _width;
	height = _height;
	type = Type::IMAGE;

	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	auto d = parse_format(_format);
	internal_format = d.internal_format;
	glTextureStorage2D(texture, 1, internal_format, width, height);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void ImageTexture::__init__(int width, int height, const string &format) {
	new(this) ImageTexture(width, height, format);
}


DepthBuffer::DepthBuffer(int _width, int _height) {
	msg_write(format("creating depth texture [%d x %d] ", _width, _height));
	filename = "-depth-";
	width = _width;
	height = _height;
	type = Type::DEPTH;
	internal_format = GL_DEPTH_COMPONENT;


	// as renderbuffer -> can't sample from it!
	/*glGenRenderbuffers(1, &texture);
	glBindRenderbuffer(GL_RENDERBUFFER, texture);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);*/

	// as texture -> can sample!
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
//	glTextureStorage2D(texture, 1, internal_format, width, height);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTextureParameterfv(texture, GL_TEXTURE_BORDER_COLOR, borderColor);
}

void DepthBuffer::__init__(int width, int height) {
	new(this) DepthBuffer(width, height);
}

RenderBuffer::RenderBuffer(int w, int h) : RenderBuffer(w, h, 0) {}

RenderBuffer::RenderBuffer(int w, int h, int _samples) {
	filename = "-render-buffer-";
	type = Type::RENDERBUFFER;
	width = w;
	height = h;
	samples = _samples;
	internal_format = GL_DEPTH24_STENCIL8;

	glGenRenderbuffers(1, &texture);
	glBindRenderbuffer(GL_RENDERBUFFER, texture);
	if (samples > 0)
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, internal_format, width, height);
	else
		glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
}


static int NixCubeMapTarget[] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

CubeMap::CubeMap(int size, const string &_format) {
	msg_write(format("creating cube map [%d x %d x 6]", size, size));
	width = size;
	height = size;
	type = Type::CUBE;
	filename = "-cubemap-";

	auto d = parse_format(_format);
	internal_format = d.internal_format;


	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(texture, 6, internal_format, width, height);

	if (false) {
		Image im;
		im.create(size, size, Red);
		for (int i=0; i<6; i++)
			overwrite_side(i, im);
	}
}

void CubeMap::__init__(int size, const string &format) {
	new(this) CubeMap(size, format);
}

void CubeMap::fill_side(int side, Texture *source) {
	if (!source)
		return;
	if (source->type == Type::CUBE)
		return;
	Image image;
	image.load(source->filename);
	overwrite_side(side, image);
}

void CubeMap::overwrite_side(int side, const Image &image) {
	//_overwrite(GL_TEXTURE_CUBE_MAP, NixCubeMapTarget[side], image);
	if (image.error)
		return;
	if (width != image.width or height != image.height)
		return;

	image.set_mode(Image::Mode::RGBA);

	glTextureSubImage3D(texture, 0, 0, 0, side, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, image.data.data);
}


};
#endif
