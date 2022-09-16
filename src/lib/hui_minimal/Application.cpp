/*
 * HuiApplication.cpp
 *
 *  Created on: 13.07.2014
 *      Author: michi
 */

#include "Application.h"
#include "hui.h"
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

Path Application::filename;
Path Application::directory;
Path Application::directory_static;
Path Application::initial_working_directory;
bool Application::installed;

Array<string> Application::_args;



Application::Application(const string &app_name, const string &def_lang, int flags) {

#ifdef HUI_API_GTK
	g_set_prgname(app_name.c_str());
#endif

	guess_directories(_args, app_name);

	SetDefaultErrorHandler(nullptr);

	if (os::fs::exists(directory << "config.txt"))
		config.load(directory << "config.txt");

}

Application::~Application() {
	//foreachb(Window *w, _all_windows_)
	//	delete(w);
	if (config.changed)
		config.save(directory << "config.txt");
	if ((msg_inited) /*&& (HuiMainLevel == 0)*/)
		msg_end();
}

Path strip_dev_dirs(const Path &p) {
	if (p.basename() == "build")
		return p.parent();
	if (p.basename() == "Debug")
		return strip_dev_dirs(p.parent());
	if (p.basename() == "Release")
		return strip_dev_dirs(p.parent());
	if (p.basename() == "x64")
		return p.parent();
	if (p.basename() == "Unoptimized")
		return p.parent();
	return p;
}

//   filename -> executable file
//   directory ->
//      NONINSTALLED:  binary dir
//      INSTALLED:     ~/.MY_APP/
//   directory_static ->
//      NONINSTALLED:  binary dir/static/
//      INSTALLED:     /usr/local/share/MY_APP/
//   initial_working_directory -> working dir before running this program
void Application::guess_directories(const Array<string> &arg, const string &app_name) {

	initial_working_directory = os::fs::current_directory();
	installed = false;


	// executable file
#if defined(OS_LINUX) || defined(OS_MINGW) //defined(__GNUC__) || defined(OS_LINUX)
	if (arg.num > 0)
		filename = arg[0];
#else // OS_WINDOWS
	char *ttt = nullptr;
	int r = _get_pgmptr(&ttt);
	filename = ttt;
	//hui_win_instance = (void*)GetModuleHandle(nullptr);
#endif


	// first, assume a local/non-installed version
	directory = strip_dev_dirs(filename.parent());
	directory_static = directory << "static";


	#if defined(OS_LINUX) || defined(OS_MINGW) //defined(__GNUC__) || defined(OS_LINUX)
		// installed version?
		if (filename.is_in("/usr/local") or (filename.str().find("/") < 0)) {
			installed = true;
			directory_static = Path("/usr/local/share") << app_name;
		} else if (filename.is_in("/usr")) {
			installed = true;
			directory_static = Path("/usr/share") << app_name;
		} else if (filename.is_in("/opt")) {
			installed = true;
			directory_static = Path("/opt") << app_name;
		//} else if (f) {
		}

		if (installed) {
			directory = format("%s/.%s/", getenv("HOME"), app_name);
			os::fs::create_directory(directory);
		}
	#endif
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
