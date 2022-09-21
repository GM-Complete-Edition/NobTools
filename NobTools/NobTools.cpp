#include <iostream>
#include "Nob.h"

#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#elif
#include <unistd.h>
#endif

void print_help()
{
	cout << "HELP\n";
	cout << "To unpack nob file write as arguments a path to your folder and filename: For Example:= D:\\TestNob\\ Ghost\n";
	cout << "To pack nob file write as arguments path to the files to pack and path for the file name: For Example:= D:\\TestNob\\UnpackedFiles\\ D:\\TestNob\\GhostNew\n";
}

int main(int argc, char *argvp[])
{
	if (argc == 4)
	{
		if (argvp[1][0] == 'u')
		{
			Nob::unpack_nob_file(argvp[2], argvp[3]);
		}
		else if (argvp[1][0] == 'w')
		{
			Nob::write_nob_file(argvp[2], argvp[3]);
		}
		else
		{
			print_help();
		}
	}
	else
	{
		print_help();
	}
	return 1;
	system("pause");
    //std::cout << "Hello World!\n";
}