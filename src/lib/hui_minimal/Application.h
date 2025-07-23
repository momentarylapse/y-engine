/*
 * HuiApplication.h
 *
 *  Created on: 13.07.2014
 *      Author: michi
 */

#ifndef HUIAPPLICATION_H_
#define HUIAPPLICATION_H_

#include "../base/base.h"
#include "../base/map.h"
#include "../os/path.h"

namespace hui {

enum {
	FLAG_LOAD_RESOURCE = 1,
	FLAG_SILENT = 2,
	FLAG_UNIQUE = 16,
};

class Application : public VirtualBase {
public:
	Application(const string &app_name, const string &def_lang, int flags);
	~Application() override;

	virtual bool on_startup(const Array<string> &arg) = 0;
	virtual void on_end() {}

	static void end();

	int run();
	static void do_single_main_loop();


	static void _cdecl set_property(const string &name, const string &value);
	static string get_property(const string &name);

	static base::map<string, string> _properties_;

	static Array<string> _args;
};

}

#define HUI_EXECUTE(APP_CLASS) \
namespace os::app { \
int hui_main(const Array<string> &arg) { \
	APP_CLASS::_args = arg; \
	APP_CLASS *app = new APP_CLASS; \
	int r = 0; \
	if (app->on_startup(arg)) \
		r = app->run(); \
	delete app; \
	return r; \
} \
}

#endif /* HUIAPPLICATION_H_ */
