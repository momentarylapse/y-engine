/*
 * Entity.h
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#pragma once

#include "../lib/base/base.h"

class Component;

namespace kaba {
	class Class;
}


class Entity : public VirtualBase {
public:
	enum class Type {
		NONE,
		ENTITY3D,
		CONTROLLER,
		MODEL,
		CAMERA,
		LIGHT,
		LINK,
		SOUND,
		EFFECT,
		PARTICLE,
		BEAM,
		UI_NODE,
		UI_TEXT,
		UI_PICTURE,
		UI_MODEL,
	};

	Entity(Type t);
	virtual ~Entity();
	virtual void _cdecl on_iterate(float dt){}
	virtual void _cdecl on_init(){}
	virtual void _cdecl on_delete(){}
	void on_init_rec();
	void on_delete_rec();

	Type type;
	Array<Component*> components;
	Component *_get_component_untyped_(const kaba::Class *type) const;
	Component *add_component(const kaba::Class *type, const string &var);
	Component *add_component_no_init(const kaba::Class *type, const string &var);

	template<class C>
	C *get_component() const {
		return (C*)_get_component_untyped_(C::_class);
	}

	void _add_component_external_(Component *c);
};


class EntityManager {
public:
	static void reset();
	static void delete_later(Entity *p);
	static void delete_selection();
	static bool enabled;
private:
	static Array<Entity*> selection;
};

#define entity_reg(var, array) \
	if (EntityManager::enabled) \
		(array).add(var);

#define entity_unreg(var, array) \
	if (EntityManager::enabled) \
		for (int i=0;i<(array).num;i++) \
			if ((array)[i] == var) \
				(array).erase(i);

#define entity_del(array) \
	for (int i=(array).num-1; i>=0; i--) \
		delete((array)[i]); \
	(array).clear();
