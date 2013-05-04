
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
   PF_Manager *pf = PF_Manager::Instance();
   PF_FileHandle fileHandle;
   pf->OpenFile("newTable",fileHandle);
   char pageOfData[] = "101s is sample data to read from the page 56456418461484355555545484758614574715721476245762472452472415786242424862424155645641846148435555554548475861457471572147624576247245247241578624242486242415564564184614843555555454847586145747157214762457624724524724157862424248624241556456418461484355555545484758614574715721476245762472452472415786242424862424155645641846148435555554548475861457471572147624576247245247241578624242486242415564564184614843555555454847586145747157214762457624724524724157862424254856";
   //fileHandle.ReadPage(0,pageOfData);

   for(int i = 0; i < 513; i++)
   {
	   cout<<pageOfData[i];
   }

   cout <<"\n\n"<< rm->getShortFromPositionInPage(0,pageOfData);

   /*
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
   rm->deleteTable("newTable");
   cout << "Table newTable1 deleted \n";

   printListOfTables(rm);
   */
}

int main()
{
	rmTest();
	return 0;
}
