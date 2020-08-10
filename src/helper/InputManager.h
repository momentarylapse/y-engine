/*
 * InputManager.h
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#ifndef SRC_HELPER_INPUTMANAGER_H_
#define SRC_HELPER_INPUTMANAGER_H_

#include <GLFW/glfw3.h>

class vector;

class InputManager {
public:
	static void init(GLFWwindow *window);

	struct State {
		bool key[256];
		bool button[3];
		float mx, my;
		float dx, dy;
		float scroll_x, scroll_y;

		void reset();
	};

	static State state;
	static State state_prev;

	static vector mouse;
	static vector dmouse;
	static vector scroll;
	static bool ignore_velocity;

	static void iterate();

	static bool get_key(int k);
	static bool get_key_down(int k);
	static bool get_key_up(int k);

private:
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

#endif /* SRC_HELPER_INPUTMANAGER_H_ */
