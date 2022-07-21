/*----------------------------------------------------------------------------*\
| Meta                                                                         |
| -> administration of animations, models, items, materials etc                |
| -> centralization of game data                                               |
|                                                                              |
| last updated: 2009.11.22 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#pragma once

#include "lib/base/base.h"
#include "lib/os/path.h"



/*class Texture;
class Shader;
class VertexBuffer;
class FrameBuffer;
class DepthBuffer;
class CubeMap;
class UniformBuffer;*/




class Model;
class Object;
class Terrain;
class Light;






class TemplateDataScriptVariable {
public:
	string name, value;
};

class TemplateDataScript {
public:
	Path filename;
	Array<TemplateDataScriptVariable> variables;
};


