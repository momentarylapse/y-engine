/*----------------------------------------------------------------------------*\
| Meta                                                                         |
| -> administration of animations, models, items, materials etc                |
| -> centralization of game data                                               |
|                                                                              |
| last updated: 2009.11.22 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(META_H__INCLUDED_)
#define META_H__INCLUDED_

#include "lib/base/base.h"
#include "lib/image/color.h"
#include "lib/hui/hui.h"

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
	string Path;
	Array<TemplateDataScriptVariable> variables;
};



void MetaReset();
void MetaCalcMove();



enum {
	ScriptLocationCalcMovePrae,
	ScriptLocationCalcMovePost,
	ScriptLocationRenderPrae,
	ScriptLocationRenderPost,
	ScriptLocationGetInputPrae,
	ScriptLocationGetInputPost,
	ScriptLocationNetworkSend,
	ScriptLocationNetworkRecieve,
	ScriptLocationNetworkAddClient,
	ScriptLocationNetworkRemoveClient,
	ScriptLocationWorldInit,
	ScriptLocationWorldDelete,
	ScriptLocationOnKeyDown,
	ScriptLocationOnKeyUp,
	ScriptLocationOnLeftButtonDown,
	ScriptLocationOnLeftButtonUp,
	ScriptLocationOnMiddleButtonDown,
	ScriptLocationOnMiddleButtonUp,
	ScriptLocationOnRightButtonDown,
	ScriptLocationOnRightButtonUp,
};



class XContainer : public hui::EventHandler {
public:
	virtual ~XContainer(){}
	virtual void _cdecl on_iterate(float dt){}
	virtual void _cdecl on_init(){}
	virtual void _cdecl on_delete(){}
};


void _cdecl MetaDeleteLater(XContainer *p);
void _cdecl MetaDeleteSelection();

extern bool AllowXContainer;

#define xcon_reg(var, array) \
	if (AllowXContainer) \
		(array).add(var);

#define xcon_unreg(var, array) \
	if (AllowXContainer) \
		for (int i=0;i<(array).num;i++) \
			if ((array)[i] == var) \
				(array).erase(i);

#define xcon_del(array) \
	for (int i=(array).num-1; i>=0; i--) \
		delete((array)[i]); \
	(array).clear();

#endif

