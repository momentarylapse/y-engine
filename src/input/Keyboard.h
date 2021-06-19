/*
 * Keyboard.h
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */


#pragma once

struct GLFWwindow;

namespace input {

void init_keyboard(GLFWwindow *window);
void iterate_keyboard_pre();
void iterate_keyboard();

bool get_key(int k);
bool get_key_down(int k);
bool get_key_up(int k);


}
