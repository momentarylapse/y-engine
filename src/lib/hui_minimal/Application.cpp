/*
 * HuiApplication.cpp
 *
 *  Created on: 13.07.2014
 *      Author: michi
 */

#include "Application.h"
#include "hui.h"
#include "../os/app.h"
#include "../os/filesystem.h"
#include "../os/msg.h"
#include "../os/config.h"

#ifdef OS_WINDOWS
#include <windows.h>
#endif

namespace hui
{

Configuration config;

extern Callback _idle_function_;


#ifdef HUI_API_GTK
	extern void *invisible_cursor;
#endif


base::map<string, string> Application::_properties_;


Array<string> Application::_args;



Application::Application(const string &app_name, const string &def_lang, int flags) {

#ifdef HUI_API_GTK
	g_set_prgname(app_name.c_str());
#endif

	os::app::detect(_args, app_name);

	SetDefaultErrorHandler(nullptr);

	if (os::fs::exists(os::app::directory_dynamic | "config.txt"))
		config.load(os::app::directory_dynamic | "config.txt");

}

Application::~Application() {
	//foreachb(Window *w, _all_windows_)
	//	delete(w);
	if (config.changed)
		config.save(os::app::directory_dynamic | "config.txt");
	if ((msg_inited) /*&& (HuiMainLevel == 0)*/)
		msg_end();
}

int Application::run() {
	return 0;
}

void Application::end() {
	SetIdleFunction(nullptr);
}

void Application::do_single_main_loop() {
}


void Application::set_property(const string &name, const string &value) {
	_properties_.set(name, value);
}

string Application::get_property(const string &name) {
	try {
		return _properties_[name];
	} catch(...) {
		return "";
	}
}

};
