/*
 * Mouse.h
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */

#pragma once

class vec2;
struct GLFWwindow;

namespace input {


void init_mouse(GLFWwindow *window);
void iterate_mouse_pre();
void iterate_mouse();

extern vec2 mouse; //   [0:R]x[0:1] coord system
extern vec2 mouse01; // [0:1]x[0:1] coord system
extern vec2 dmouse;
extern vec2 scroll;

bool get_button(int index);


}
