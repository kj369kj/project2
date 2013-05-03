
#include "rm.h"

RM* RM::_rm = 0;
PF_Manager *pf;

RM* RM::Instance()
{
    if(!_rm)
        _rm = new RM();
    pf = PF_Manager::Instance();

    return _rm;
}

RM::RM()
{
}

RM::~RM()
{
}

RC RM::createTable(const string tableName, const vector<Attribute> &attrs)
{
	//creates an empty table object then populates its data
	Tableinfo tableToInsert;
	tableToInsert.tableName = tableName;
	tableToInsert.listOfAttributes = attrs;

	//inserts table object into a log of tables in our system catalog
	listOfTables.push_back(tableToInsert);

	//need to create a file for the table created so that we have a place to store data later
	pf->CreateFile(tableName.c_str());

	return 0;
}

//bugg
//doesnt delete files that were created in previous run of the software
// will delete files created by this session
// removes from list correctly
RC RM::deleteTable(const string tableName)
{
	//remove table from listOfTables
	//must first search through current list of tables to find the tables index
	int tableIndex = getIndexOfTableNameInListOfTables(tableName);

	//now that index is found time to remove from list
	listOfTables.erase(listOfTables.begin()+tableIndex);

	//time to remove the file
	pf->DestroyFile(tableName.c_str());
	return 0;
}


RC RM::getAttributes(const string tableName, vector<Attribute> &attrs)
{
	//must search for table index
	int tableIndex = getIndexOfTableNameInListOfTables(tableName);
	if(tableIndex != -1)
	{
		attrs = listOfTables[tableIndex].listOfAttributes;
		return 0;
	}

	return 1;

}

RC RM::insertTuple(const string tableName, const void *data, RID &rid)
{
	PF_FileHandle fileHandle;
	pf->OpenFile(tableName.c_str(),fileHandle);

	//must read in the current page and make appropriate changes
	//before writing back to disk. Based on data structure discussed in email.
	bitset<PF_PAGE_SIZE> pageReadIn; //malloc allocates memory on ram for storage of data
	fileHandle.ReadPage(rid.pageNum, pageReadIn);

	//now that i have the page i need to make changes to the data to reflect the insert

	//need to update N
	//need to calculate space for inserted record; this can probably be done by looking at *data in paramater
	//need to attach record


}


int RM::getIndexOfTableNameInListOfTables(const string tableName)
{
	//searches through list of tables by name for correct index
	//method created because the search was needed to be done multiple times
	for(unsigned int i = 0; i < listOfTables.size();i++)
	{
		if(listOfTables[i].tableName.compare(tableName)==0)
		{
			return i;
		}
	}

	return -1;
}



//helper method for testing
vector<Tableinfo> RM::getTables()
{
	return listOfTables;
}

