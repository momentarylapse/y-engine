/*
 * Component.h
 *
 *  Created on: Jul 12, 2021
 *      Author: michi
 */

#pragma once

#include <lib/base/base.h>

#include "BaseClass.h"

struct Entity;
struct CollisionData;
struct ScriptInstanceDataVariable;
struct ScriptInstanceData;
namespace kaba {
	class Class;
}

struct Component : VirtualBase {
	Component();
	~Component() override;
	virtual void on_init() {}
	virtual void on_delete() {}
	virtual void on_iterate(float dt) {}

	virtual void on_collide(const CollisionData& col) {}

	void set_variables(const Array<ScriptInstanceDataVariable>& variables);

	Entity* owner;
	const kaba::Class* component_type;

	template<class T>
	T* as() {
		if (component_type == T::_class)
			return static_cast<T*>(this);
		return nullptr;
	}
};

struct NameTag : Component {
	string name;
	static const kaba::Class* _class;
};

struct EgoMarker : Component {
	static const kaba::Class* _class;
};

struct Template {
	Array<ScriptInstanceData> components;
};

struct TemplateRef : Component {
	Template* _template = nullptr;
	static const kaba::Class* _class;
};

