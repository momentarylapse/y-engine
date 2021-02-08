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

#include "../file/file.h"


#include <stdio.h>
#include <signal.h>


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


int hui_main(const Array<string> &);

// for a system independent usage of this library

#ifdef OS_WINDOWS

#ifdef _CONSOLE

int _tmain(int NumArgs, _TCHAR *Args[]) {
	return hui_main(hui::make_args(NumArgs, Args));
}

#else

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	Array<string> a;
	a.add("-dummy-");
	string s = lpCmdLine;
	if (s.num > 0){
		if ((s[0] == '\"') and (s.back() == '\"'))
			s = s.substr(1, -2);
		a.add(s);
	}
	return hui_main(a);
}

#endif

#endif
#if  defined(OS_LINUX) || defined(OS_MINGW)

int main(int NumArgs, char *Args[])
{
	return hui_main(hui::make_args(NumArgs, Args));
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


