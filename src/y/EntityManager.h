//
// Created by michi on 9/10/25.
//

#pragma once

#include <lib/base/base.h>
#include <lib/base/pointer.h>
#include "ComponentManager.h"

struct vec3;
struct quaternion;
class Entity;

class EntityManager {
public:
	EntityManager();
	~EntityManager();

	Entity* create_entity(const vec3& pos, const quaternion& ang);
	void delete_entity(Entity* entity);
	void reset();
	void shift_all(const vec3& dpos);

	template<class C>
	Array<C*>& get_component_list() {
		return (Array<C*>&) component_manager->_get_list(C::_class);
	}

	template<class C>
	Array<C*>& get_component_list_family() {
		return (Array<C*>&) component_manager->_get_list_family(C::_class);
	}

	static EntityManager* global;
	owned<ComponentManager> component_manager;

private:
	Array<Entity*> entities;
};

