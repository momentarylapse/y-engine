/*
 * TimeTable.h
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#ifndef SRC_HELPER_TIMETABLE_H_
#define SRC_HELPER_TIMETABLE_H_

#include <lib/base/base.h>
//#include <lib/base/callable.h>
#include <functional>

class TimeTable {
public:
	TimeTable();

	struct Event {
		float t;
		//Callable<void()> f;
		std::function<void()> f;
	};
	Array<Event> events;
	float t = 0;

	void iterate(float dt);
	void at(float t, std::function<void()> f);
};

#endif /* SRC_HELPER_TIMETABLE_H_ */
