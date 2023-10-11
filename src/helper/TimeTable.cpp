/*
 * TimeTable.cpp
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#include "TimeTable.h"

TimeTable::TimeTable() {
}

void TimeTable::at(float t, std::function<void()> f) {
	events.add({t, f});
}

void TimeTable::iterate(float dt) {
	float t_next = t + dt;
	for (auto& e: events)
		if (e.t > t and e.t <= t_next)
			e.f();
	t = t_next;
}

