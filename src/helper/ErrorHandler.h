/*
 * ErrorHandler.h
 *
 *  Created on: 19.08.2020
 *      Author: michi
 */

#ifndef SRC_HELPER_ERRORHANDLER_H_
#define SRC_HELPER_ERRORHANDLER_H_

class ErrorHandler {
public:
	static void init();
	static void show_backtrace();
	static void signal_handler(int signum);
};

#endif /* SRC_HELPER_ERRORHANDLER_H_ */
