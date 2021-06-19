/*
 * Mouse.h
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */

#pragma once

class vector;
struct GLFWwindow;

namespace input {


void init_mouse(GLFWwindow *window);
void iterate_mouse();

extern vector mouse; //   [0:R]x[0:1] coord system
extern vector mouse01; // [0:1]x[0:1] coord system
extern vector dmouse;
extern vector scroll;


}
