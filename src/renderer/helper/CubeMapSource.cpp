#include "CubeMapSource.h"
#include <Config.h>
#include <graphics-impl.h>

const kaba::Class* CubeMapSource::_class = nullptr;;

CubeMapSource::CubeMapSource() {
	min_depth = 1.0f;
	max_depth = 100000.0f;
	resolution = config.get_int("cubemap.resolution");
	update_mode = 1;
	state = 0;
}

CubeMapSource::~CubeMapSource() = default;

