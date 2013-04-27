#include "pf.h"
#include <stdio.h>
#include <iostream>
using namespace std;

PF_Manager* PF_Manager::_pf_manager = 0;


PF_Manager* PF_Manager::Instance()
{
	if(!_pf_manager)
		_pf_manager = new PF_Manager();

	return _pf_manager;
}


PF_Manager::PF_Manager()
{
}


PF_Manager::~PF_Manager()
{
}


RC PF_Manager::CreateFile(const char *fileName)
{
	FILE * newFile = fopen (fileName,"rb");
	if (newFile == NULL)
	{
		fclose (fopen (fileName, "wb+"));
		return 0;
	}
	else
	{
		fclose(newFile);
		return 1;
	}
}


RC PF_Manager::DestroyFile(const char *fileName)
{
	FILE * newFile = fopen (fileName,"rb");
	if (newFile == NULL)
	{
		return 1;
	}
	else
	{
		fclose(newFile);
		remove (fileName);
	}
	return 0;
}


RC PF_Manager::OpenFile(const char *fileName, PF_FileHandle &fileHandle)
{
	fileHandle.fileHandled = fopen (fileName, "rb+");
	fseek(fileHandle.fileHandled, 0, SEEK_END);
	int pageCount = ftell(fileHandle.fileHandled)/PF_PAGE_SIZE;
	fileHandle.SetPageCount(pageCount);
	fseek(fileHandle.fileHandled, 0, SEEK_SET);
	return 0;
}


RC PF_Manager::CloseFile(PF_FileHandle &fileHandle)
{
	fclose (fileHandle.fileHandled);
	return 0;
}


PF_FileHandle::PF_FileHandle()
{
	fileHandled = NULL;
	pageCount = 0;
}


PF_FileHandle::~PF_FileHandle()
{
}


RC PF_FileHandle::ReadPage(PageNum pageNum, void *data)
{
	if (pageNum <= (pageCount - 1))
	{
		int offset = pageNum * PF_PAGE_SIZE;
		fseek(fileHandled, offset, SEEK_SET);
		fread(data, PF_PAGE_SIZE, 1, fileHandled);
		return 0;
	}
	else
	{
		return 1;
	}
}


RC PF_FileHandle::WritePage(PageNum pageNum, const void *data)
{
	if (pageNum <= (pageCount - 1))
	{
		int offset = pageNum * PF_PAGE_SIZE;
		fseek(fileHandled, offset, SEEK_SET);
		fwrite(data, PF_PAGE_SIZE, 1, fileHandled);
		return 0;
	}
	else
	{
		return 1;
	}
}


RC PF_FileHandle::AppendPage(const void *data)
{
	pageCount++;
	fseek(fileHandled, 0, SEEK_END);
	fwrite(data, PF_PAGE_SIZE, 1, fileHandled);
	return 0;
}

void PF_FileHandle::SetPageCount(int x)
{
	pageCount = x;
}

unsigned PF_FileHandle::GetNumberOfPages()
{
	return pageCount;
}


