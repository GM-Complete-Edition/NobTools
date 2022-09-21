#include <iostream>
#include <fstream>
#include <iostream>
#include "Nob.h"
#include <list>
#include <shlobj_core.h>
#pragma warning(disable : 4996)

using namespace std;

Nob *main_nob_p;

#define BUFFER_SIZE 2048
#define MSG_BUFFER_SIZE 512
#define MAX_FILENAME 64
#define NOB_VERSION 0

//Writting part

void listFilesInDirectory(const char * relative_path, const char* absolute, vector<string> *listFiles)
{
	WIN32_FIND_DATA FindFileData;
	char directory[MAX_PATH] = { 0 };
	sprintf(directory, "%s*.*", relative_path);

	HANDLE hFind = FindFirstFile(directory, &FindFileData);

	int dwDots = 0;

	while (FindNextFile(hFind, &FindFileData))
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (strcmp(FindFileData.cFileName, "..") != 0)
			{
				char nRelative_path[MAX_PATH] = { 0 };
				sprintf(nRelative_path, "%s\%s\\", relative_path, FindFileData.cFileName);
				char nAbsolute_path[MAX_PATH] = { 0 };
				if (absolute)
					sprintf(nAbsolute_path, "%s\%s\\", absolute, FindFileData.cFileName);
				else
					sprintf(nAbsolute_path, "%s\\", FindFileData.cFileName);
				listFilesInDirectory(nRelative_path, nAbsolute_path, listFiles);
			}
		}
		else
		{
			char nPath[MAX_PATH] = { 0 };
			if (absolute)
			{
				
				sprintf(nPath, "%s%s", absolute, FindFileData.cFileName);
				listFiles->push_back(string(nPath));
			}
			else
			{
				sprintf(nPath, "\%s", FindFileData.cFileName);
				listFiles->push_back(string(nPath));
			}
			
		}
	}

}

u32 GetStreamSize(ifstream *myfile)
{
	const auto begin = myfile->tellg();
	myfile->seekg(0, ios::end);
	const auto end = myfile->tellg();
	const auto fsize = (end - begin);
	myfile->seekg(begin);
	return fsize;
}

int Nob::write_nob_file(const char* path, const char* path_to_save)
{
	char fat_filename[1024] = {0};
	sprintf(fat_filename, "%s.FAT", path_to_save);
	ofstream fat_outfile;
	fat_outfile.open(fat_filename, ios::binary | ios::out);

	char nob_filename[1024] = { 0 };
	sprintf(nob_filename, "%s.NOB", path_to_save);

	ofstream nob_outfile;
	nob_outfile.open(nob_filename, ios::binary | ios::out);

	vector<string> listFiles;
	listFilesInDirectory(path, NULL, &listFiles);

	u32 totalFileSize = 0;

	Nob *nfat = new Nob();

	nfat->num_files = listFiles.size();

	nfat->version = NOB_VERSION;

	nfat->fat_file_datas.resize(nfat->num_files);

	u32 i = 0;

	for (string str : listFiles)
	{
		char curr_file[1024] = { 0 };
		sprintf(curr_file, "%s%s", path, str.c_str());

		ifstream file(curr_file, ios::out | ios::binary);

		u32 streamSize = GetStreamSize(&file);

		u32 numBuffers = ceil((float)GetStreamSize(&file) / (float)BUFFER_SIZE);

		u32 allocated_size = ceil((float)GetStreamSize(&file) / (float)BUFFER_SIZE) * BUFFER_SIZE;

		char* buffer = new char[allocated_size] {0};
		file.read(buffer, streamSize);
		nob_outfile.write(buffer, allocated_size);

		nfat->size += allocated_size;
		nfat->fat_file_datas[i].size = streamSize;
		nfat->fat_file_datas[i].str_size = str.length() + 1;
		nfat->fat_file_datas[i].num_buffers = numBuffers;
		if (i != 0)
		{		
			nfat->fat_file_datas[i].str_offset = nfat->fat_file_datas[i-1].str_offset + nfat->fat_file_datas[i - 1].str_size;
			nfat->fat_file_datas[i].unk4 = nfat->fat_file_datas[i - 1].unk4 + nfat->fat_file_datas[i - 1].num_buffers;
		}
		else
		{
			nfat->fat_file_datas[i].str_offset = 0;
			nfat->fat_file_datas[i].unk4 = 0;
		}
		i++;
	}

	fat_outfile.write((char *)&nfat->size, sizeof(u32));
	fat_outfile.write((char *)&nfat->num_files, sizeof(u32));
	fat_outfile.write((char *)&nfat->version, sizeof(u32));
	for (int i = 0; i < nfat->num_files; i++)
	{
		fat_outfile.write((char *)&nfat->fat_file_datas[i].unk1, sizeof(u32));
		fat_outfile.write((char *)&nfat->fat_file_datas[i].unk2, sizeof(u32));
		fat_outfile.write((char *)&nfat->fat_file_datas[i].str_offset, sizeof(u32));
		fat_outfile.write((char *)&nfat->fat_file_datas[i].unk4, sizeof(u32));
		fat_outfile.write((char *)&nfat->fat_file_datas[i].size, sizeof(u32));
	}

	//two 0....
	fat_outfile.write((char *)&nfat->version, sizeof(u32));
	fat_outfile.write((char *)&nfat->version, sizeof(u32));

	for (string str : listFiles)
	{
		fat_outfile.write(str.c_str(), str.length() + 1);
	}


	free(nfat);
	fat_outfile.close();
	nob_outfile.close();

	return 1;
}

//Reading Part


//Creates decursively directories
int createDirectoryRecursively(LPCTSTR path)
{
	return SHCreateDirectoryEx(NULL, path, NULL);
}


//Creates new string without the file extension
char *GetFilePath(char *filename)
{
	//copy our string
	char *nString = strdup(filename);
	//check if it contains slashes 
	char *last_slash = strrchr(nString, '\\');
	if (last_slash)
	{
		//if yes then terminate next char
		*(last_slash + 1) = '\0';
	}
	return nString;
}

static void ReadU32(ifstream *s, u32 *file)
{
	s->read((char *)file, sizeof(u32));
}

static void WriteFileFromFat(ifstream *fat, ifstream *nob, fat_file_data *ffd, const char* path)
{
	streampos p = fat->tellg();

	u32 num_chars = 0;
	char currChar = 0;
	do
	{
		fat->read(&currChar, sizeof(char));
		num_chars++;
	} 
	while (currChar != '\0');

	fat->seekg(p);

	char curr_filename[MAX_FILENAME];

	fat->read(curr_filename, num_chars);

	//cout << "Writting file:= " << curr_filename << "...\n";

	u32 allocated_size = ceil((f32)ffd->size / (f32)BUFFER_SIZE) * BUFFER_SIZE;
	ofstream outfile;

	char nfile_name[256] = { 0 };
	sprintf(nfile_name,"%sUnpackedFiles\\%s", path, curr_filename);

	char* nfile_path = GetFilePath(nfile_name);

	createDirectoryRecursively(nfile_path);

	outfile.open(nfile_name, ios::binary | ios::out);

	//get buffer
	char* buffer = new char[allocated_size] {0};
	nob->read(buffer, allocated_size);
	outfile.write(buffer, ffd->size);
	outfile.close();
}

int Nob::open_nob_file(ifstream *nob_file, ifstream *fat_file, const char* path)
{
	main_nob_p = new Nob();

	ReadU32(fat_file, &main_nob_p->size);
	ReadU32(fat_file, &main_nob_p->num_files);
	ReadU32(fat_file, &main_nob_p->version);

	fat_file_data ffd;
	for (int i = 0; i < main_nob_p->num_files; i++)
	{
		ReadU32(fat_file, &ffd.unk1);
		ReadU32(fat_file, &ffd.unk2);
		ReadU32(fat_file, &ffd.str_offset);
		ReadU32(fat_file, &ffd.unk4);
		ReadU32(fat_file, &ffd.size);
		main_nob_p->fat_file_datas.push_back(ffd);
	}

	u32 temp;
	ReadU32(fat_file, &temp);
	ReadU32(fat_file, &temp);

	//read all text files:
	for (int i = 0; i < main_nob_p->num_files; i++)
	{
		WriteFileFromFat(fat_file, nob_file, &main_nob_p->fat_file_datas[i], path);
	}

	//main_nob_p->print_nob();

	return 1;
}

int Nob::unpack_nob_file(const char *path, char *filename)
{
	//read files

	char fat_fname[MAX_PATH] = { 0 };
	char error_msg[MSG_BUFFER_SIZE] = { 0 };

	strupr(filename);
	sprintf(fat_fname, "%s%s.FAT", path, filename);

	ifstream fat_file(fat_fname, ios::out | ios::binary);
	if (!fat_file)
	{
		sprintf(error_msg, "Error Opening '%s' Nob File Fat Info\n", fat_fname);
		std::cout << error_msg;
		return 0;
	}

	char nob_fname[MAX_PATH] = { 0 };

	sprintf(nob_fname, "%s%s.NOB", path, filename);

	ifstream nob_file(nob_fname, ios::out | ios::binary);

	if (!nob_file)
	{
		sprintf(error_msg, "Can't Open Nob File '%s'\n", nob_fname);
		std::cout << error_msg;
		return 0;
	}

	//Open Files:
	Nob::open_nob_file(&nob_file, &fat_file, path);

	fat_file.close();
	nob_file.close();


	return 1;
}

void Nob::print_nob()
{
	if (main_nob_p)
	{
		cout << "Nob File Content" << "\n";
		cout << "size:= " << main_nob_p->size << "\n";
		cout << "num_files:= " << main_nob_p->num_files << "\n";
		cout << "version:= " << main_nob_p->version << "\n\n";
		
		for (int i = 0; i < main_nob_p->num_files; i++)
		{
			cout << "File " << i + 1 << endl;
			cout << "unk1:= " << main_nob_p->fat_file_datas[i].unk1 << "\n";
			cout << "unk2:= " << main_nob_p->fat_file_datas[i].unk2 << "\n";
			cout << "str_offset:= " << main_nob_p->fat_file_datas[i].str_offset << "\n";
			cout << "unk4:= " << main_nob_p->fat_file_datas[i].unk4 << "\n";
			cout << "size:= " << main_nob_p->fat_file_datas[i].size << "\n\n";
		}
	}
}