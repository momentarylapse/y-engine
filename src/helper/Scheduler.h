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
	static void init();
	static void reset();
	static void subscribe(float dt, kaba::Function *f);
	static void iterate_subscriptions(float dt);

	static void handle_iterate_pre(float dt);
	static void handle_iterate(float dt);
	static void handle_input();
	static void handle_draw_pre();
	static void handle_render_inject();
	static void handle_render_inject2();
};
