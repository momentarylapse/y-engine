/*
 * Scheduler.h
 *
 *  Created on: Jul 24, 2021
 *      Author: michi
 */

#pragma

namespace kaba {
	class Function;
}

class Scheduler {
public:
	static void reset();
	static void subscribe(float dt, kaba::Function *f);
	static void iterate(float dt);
};
