#include "pf.h"
//change
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

	//check for created
	FILE * file = fopen(fileName,"r");
	if(file == 0 )
	{
		fclose(fopen(fileName,"w"));
		return 0;
	}
	else
	{
		//file already exist so return error -1
		return -1;
	}
}


RC PF_Manager::DestroyFile(const char *fileName)
{
	return remove(fileName);
}


RC PF_Manager::OpenFile(const char *fileName, PF_FileHandle &fileHandle)
{
	FILE * openedFile = fopen(fileName,"r+");
	if(openedFile != 0)
	{
		//check fileHandle make sure its not associated with other files
		if(!fileHandle.isAssociated())
		{
			fileHandle.setAssociation(openedFile);
			return 0;
		}

	}
    return -1;
}


RC PF_Manager::CloseFile(PF_FileHandle &fileHandle)
{
	if(fileHandle.isAssociated())
	{
		//flush the stream
		fflush(fileHandle.getAssociation());
		//close the stream
		fclose(fileHandle.getAssociation());
		//might need to destroy filehandle
		return 0;
	}
    return -1;
}


PF_FileHandle::PF_FileHandle()
{
	fileAssociation = 0;
	associated = false;
}
 

PF_FileHandle::~PF_FileHandle()
{
}

void PF_FileHandle::setAssociation(FILE *fileAssociations)
{
	this->fileAssociation = fileAssociations;
	associated = true;
}

FILE* PF_FileHandle::getAssociation()
{
	return fileAssociation;
}

void PF_FileHandle::removeAssociation()
{
	fileAssociation = 0;
	associated = false;
}

bool PF_FileHandle::isAssociated()
{
	return associated;
}

RC PF_FileHandle::ReadPage(PageNum pageNum, void *data)
{
	if(associated)
	{
		fseek(fileAssociation,pageNum*fourKilabyte,0);
		fread(data,fourKilabyte,1,fileAssociation);
		return 0;
	}
    return -1;
}


RC PF_FileHandle::WritePage(PageNum pageNum, const void *data)
{
	if(associated)
	{
		fseek(fileAssociation,pageNum*fourKilabyte,0);
		fwrite(data,fourKilabyte,1,fileAssociation);
		return 0;
	}
    return -1;
}


RC PF_FileHandle::AppendPage(const void *data)
{
	if(associated)
	{
		fseek(fileAssociation,0,SEEK_END);
		fwrite(data,fourKilabyte,1,fileAssociation);
		return 0;
	}
    return -1;
}


unsigned PF_FileHandle::GetNumberOfPages()
{
	if(associated)
	{
		fseek(fileAssociation, 0,SEEK_END);
		double position = ftell(fileAssociation);
		fseek(fileAssociation, 0,0);
		return ceil(position/fourKilabyte);

	}
    return -1;
}


