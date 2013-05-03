
#include <fstream>
#include <iostream>
#include <cassert>
#include <windows.h>

#include "rm.h"

using namespace std;

void printListOfTables(RM *rm)
{
	vector<Tableinfo> tables = rm->getTables();
	for(unsigned int i = 0; i < tables.size();i++)
	{
		cout << tables[i].tableName;
		//printListOfAttribute
		vector<Attribute> listOfAttributes = tables[i].listOfAttributes;
		for(unsigned int j = 0 ; j < listOfAttributes.size(); j++)
		{
			cout << " length:"<<listOfAttributes[j].name << " type:"<<listOfAttributes[j].type<< " length:"<< listOfAttributes[j].length<<"\n";
		}
	}
}

void rmTest()
{
   RM *rm = RM::Instance();

  //create table test;
   //AttrType attrType;
   vector<Attribute> listOfAttributes;
   //create an attribute object to be added to tables later
   Attribute newAttribute;
   newAttribute.length = 3;
   newAttribute.type = TypeVarChar;
   newAttribute.name = "userId";
   //push the attribute to the tableinfo
   listOfAttributes.push_back(newAttribute);
   //create multiple tables
   rm->createTable("newTable",listOfAttributes);
   rm->createTable("newTable1",listOfAttributes);
   rm->createTable("newTable2",listOfAttributes);

   //delete Table test
   printListOfTables(rm);

   Sleep(5000);
   rm->deleteTable("newTable1");
   cout << "Table newTable1 deleted \n";

   printListOfTables(rm);
}

int main()
{
	rmTest();
	return 0;
}
