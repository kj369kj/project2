
#include <fstream>
#include <iostream>
#include <cassert>

#include "rm.h"

using namespace std;

void rmTest()
{
   RM *rm = RM::Instance();

  //create table test;
   //AttrType attrType;
   vector<Attribute> listOfAttributes;
   Attribute newAttribute;
   newAttribute.length = 3;
   newAttribute.type = TypeVarChar;
   newAttribute.name = "userId";
   listOfAttributes.push_back(newAttribute);
   rm->createTable("newTable",listOfAttributes);


  // write your own testing cases here
}

int main()
{
	rmTest();
	return 0;
}
