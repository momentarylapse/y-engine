/*
 * jitter.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#pragma once

class matrix;

matrix jitter(float w, float h, int uid);
void jitter_iterate();

