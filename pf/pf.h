#ifndef _pf_h_
#define _pf_h_

typedef int RC;
typedef unsigned PageNum;

#define PF_PAGE_SIZE 4096


#include <cstdio>
#include <iostream>
#include <math.h>
using namespace std;

class PF_FileHandle;


class PF_Manager
{
public:
    static PF_Manager* Instance();                                      // Access to the _pf_manager instance

    RC CreateFile    (const char *fileName);                            // Create a new file
    RC DestroyFile   (const char *fileName);                            // Destroy a file
    RC OpenFile      (const char *fileName, PF_FileHandle &fileHandle); // Open a file
    RC CloseFile     (PF_FileHandle &fileHandle);                       // Close a file

protected:
    PF_Manager();                                                       // Constructor
    ~PF_Manager   ();                                                   // Destructor

private:
    static PF_Manager *_pf_manager;
};


class PF_FileHandle
{
public:
    PF_FileHandle();                                                    // Default constructor
    ~PF_FileHandle();                                                   // Destructor

    RC ReadPage(PageNum pageNum, void *data);                           // Get a specific page
    RC WritePage(PageNum pageNum, const void *data);                    // Write a specific page
    RC AppendPage(const void *data);                                    // Append a specific page
    unsigned GetNumberOfPages();                                        // Get the number of pages in the file
    void setAssociation(FILE *fileAssociation);
    void removeAssociation();
    FILE* getAssociation();
    bool isAssociated();
private:
    FILE *fileAssociation;
    bool associated;
    const size_t fourKilabyte = 4096;

 };

 #endif
