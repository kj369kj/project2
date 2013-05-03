
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

}

