//
// Created by michi on 9/10/25.
//

#pragma once

#include <lib/base/base.h>

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

private:
	Array<Entity*> entities;
};

