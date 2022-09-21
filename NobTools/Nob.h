#pragma once

#ifndef NOB_H
#define NOB_H

#include <iostream>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

typedef unsigned int u32;
typedef float f32;

struct fat_file_data
{
	u32 unk1;
	u32 unk2;
	u32 str_offset;
	u32 unk4;
	u32 size;
	u32 str_size;
	u32 num_buffers;
};


class Nob
{
public:
	u32 size;
	u32 num_files;
	u32 version;
	vector<fat_file_data> fat_file_datas;

	void print_nob();

	static int unpack_nob_file(const char *path, char *filename);
	static int open_nob_file(ifstream *nob_file, ifstream *fat_file, const char* path);
	static int write_nob_file(const char* path, const char* path_to_save);
};

extern Nob *main_nob_p;

extern char * GetFilePath(char *filename);

#endif