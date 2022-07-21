/*
 * hui_error.cpp
 *
 *  Created on: 26.06.2013
 *      Author: michi
 */

#include "hui.h"
#include "../os/msg.h"



#ifdef _X_USE_NET_
	#include "../net/net.h"
#endif

#include <signal.h>

namespace hui
{

extern Callback _idle_function_, _error_function_;

void _HuiSignalHandler(int) {
	_error_function_();
}

// apply a function to be executed when a critical error occures
void SetErrorFunction(const Callback &function) {
	_error_function_ = function;
	if (function) {
		signal(SIGSEGV, &_HuiSignalHandler);
		/*signal(SIGINT, &_HuiSignalHandler);
		signal(SIGILL, &_HuiSignalHandler);
		signal(SIGTERM, &_HuiSignalHandler);
		signal(SIGABRT, &_HuiSignalHandler);*/
		/*signal(SIGFPE, &_HuiSignalHandler);
		signal(SIGBREAK, &_HuiSignalHandler);
		signal(SIGABRT_COMPAT, &_HuiSignalHandler);*/
	}
}

static Callback _eh_cleanup_function_;



void hui_default_error_handler() {
	_idle_function_ = nullptr;

	msg_reset_shift();
	msg_write("");
	msg_write("================================================================================");
	msg_write("program has crashed, error handler has been called... maybe SegFault... m(-_-)m");
	//msg_write("---");
	msg_write("      trace:");
	msg_write(msg_get_trace());

	if (_eh_cleanup_function_){
		msg_write("i'm now going to clean up...");
		_eh_cleanup_function_();
		msg_write("...done");
	}

	//HuiEnd();
	exit(1);
}

void SetDefaultErrorHandler(const Callback &error_cleanup_function) {
	_eh_cleanup_function_ = error_cleanup_function;
	SetErrorFunction(&hui_default_error_handler);
}

void RaiseError(const string &message) {
	msg_error(message + " (HuiRaiseError)");
	/*int *p_i=NULL;
	*p_i=4;*/
	hui_default_error_handler();
}

#ifdef OS_WINDOWS
#include <windows.h>
void ShowError(const string& msg) {
	MessageBox(NULL, msg.c_str(), "Error", MB_ICONERROR | MB_OK);
}
#else
void ShowError(const string& msg) {
	msg_error(msg);
}
#endif


};

