#include "stdafx.h"
#include <iostream>
#include "point_search.h"
#include <windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
	HINSTANCE hGetProcIDDLL = LoadLibraryEx("adams.dll", NULL, NULL);

	if (!hGetProcIDDLL) {
		std::cout << "could not load the dynamic library" << std::endl;
		return EXIT_FAILURE;
	}

	T_create f = (T_create)GetProcAddress(hGetProcIDDLL, "create");


	if (!f) {
		std::cout << "could not locate the function" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "f() returned " << f(NULL, NULL) << std::endl;

	return EXIT_SUCCESS;
}

