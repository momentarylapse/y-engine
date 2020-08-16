/*----------------------------------------------------------------------------*\
| Meta                                                                         |
| -> administration of animations, models, items, materials etc                |
| -> centralization of game data                                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
|  - paths get completed with the belonging root-directory of the file type    |
|    (model,item)                                                              |
|  - independent models                                                        |
|     -> equal loading commands create new instances                           |
|     -> equal loading commands copy existing models                           |
|         -> databases of original and copied models                           |
|         -> some data is referenced (skin...)                                 |
|         -> additionally created: effects, absolute physical data,...         |
|     -> each object has its own model                                         |
|  - independent items (managed by CMeta)                                      |
|     -> new items additionally saved as an "original item"                    |
|     -> an array of pointers points to each item                              |
|     -> each item has its unique ID (index in the array) for networking       |
|  - materials stay alive forever, just one instance                           |
|                                                                              |
| last updated: 2009.12.09 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "lib/file/file.h"
#include "meta.h"
//#include "lib/vulkan/vulkan.h"
#include "lib/nix/nix.h"
#include "lib/kaba/kaba.h"
#ifdef _X_ALLOW_X_
#include "world/model.h"
#if 0
#include "world/model_manager.h"
#endif
#include "world/material.h"
#include "world/world.h"
#include "world/camera.h"
#if 0
#include "gui/gui.h"
#include "gui/font.h"
#include "fx/fx.h"
#endif
#else // for use in Edward
#include "x/material.h"
#include "x/model.h"
#include "x/model_manager.h"
#include "x/font.h"
#endif
#include "y/EngineData.h"




// game configuration



Array<XContainer*> meta_delete_stuff_list;

bool AllowXContainer = true;



void MetaReset() {
	meta_delete_stuff_list.clear();

#if 0
	if (Gui::Fonts.num > 0)
		Engine.DefaultFont = Gui::Fonts[0];
#endif
	engine.shadow_lower_detail = false;
}

void MetaCalcMove() {
/*	for (int i=0;i<NumTextures;i++)
		NixTextureVideoMove(Texture[i],Elapsed);*/
	msg_todo("MetaCalcMove...");
	
	engine.detail_factor_inv = 100.0f/(float)engine.detail_level;
}


void MetaDeleteLater(XContainer *p) {
	meta_delete_stuff_list.add(p);
}

void MetaDeleteSelection() {
	for (auto *p: meta_delete_stuff_list)
		delete p;
	meta_delete_stuff_list.clear();
}
