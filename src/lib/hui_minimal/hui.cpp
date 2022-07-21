/*----------------------------------------------------------------------------*\
| CHui                                                                         |
| -> Heroic User Interface                                                     |
| -> abstraction layer for GUI                                                 |
|   -> Windows (default api) or Linux (Gtk+)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.06.21 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/


#include "../hui_minimal/hui.h"

#include "../os/msg.h"


#include <stdio.h>
#include <signal.h>

#ifdef OS_WINDOWS
#include <windows.h>
#endif


namespace hui
{


Array<string> make_args(int num_args, char *args[])
{
	Array<string> a;
	for (int i=0; i<num_args; i++)
		a.add(args[i]);
	return a;
}


//----------------------------------------------------------------------------------
// system independence of main() function



}


int hui_main(const Array<string>&);

// for a system independent usage of this library

#ifdef OS_WINDOWS

#ifdef _CONSOLE

int _tmain(int num_args, _TCHAR* args[]) {
	return hui_main(hui::make_args(num_args, args));
}

#else

// split by space... but parts might be in quotes "a b"
Array<string> parse_command_line(const string& s) {
	Array<string> a;
	a.add("-dummy-");

	for (int i = 0; i < s.num; i++) {
		if (s[i] == '\"') {
			string t;
			bool escape = false;
			i++;
			for (int j = i; j < s.num; j++) {
				i = j;
				if (escape) {
					escape = false;
				}
				else {
					if (s[j] == '\\')
						escape = true;
					else if (s[j] == '\"')
						break;
				}
				t.add(s[j]);
			}
			a.add(t.unescape());
			i++;
		}
		else if (s[i] == ' ') {
			continue;
		}
		else {
			string t;
			for (int j = i; j < s.num; j++) {
				i = j;
				if (s[j] == ' ')
					break;
				t.add(s[j]);
			}
			a.add(t);
		}
	}
	return a;
}

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	return hui_main(parse_command_line(lpCmdLine));
}

#endif

#endif
#if  defined(OS_LINUX) || defined(OS_MINGW)

int main(int num_args, char* args[]) {
	return hui_main(hui::make_args(num_args, args));
}

#endif


// usage:
//
// int hui_main(const Array<string> &arg)
// {
//     HuiInit();
//     ....
//     return HuiRun();
// }


