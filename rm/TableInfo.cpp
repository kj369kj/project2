/*
 * TableInfo.cpp
 *
 *  Created on: Apr 24, 2013
 *      Author: Emma
 */

#include "TableInfo.h"

TableInfo::TableInfo(string tableName, vector<Attribute> listOfAttributes) {
	// TODO Auto-generated constructor stub
	this->listOfAttributes = listOfAttributes;
	this->tableName = tableName;

}

TableInfo::~TableInfo() {
	// TODO Auto-generated destructor stub
}

vector<Attribute> TableInfo::getAttributes(){
	return listOfAttributes;
}

int TableInfo::getRecordOffset(int rid)
{
	return recordHash[rid];
}

hash_map<RID,int> TableInfo::getRecordHash()
{
	return recordHash;
}

void TableInfo::addRecordLocation(RID rid,int offset)
{
	recordHash[rid]=offset;
}

void TableInfo::changeLocation(RID rid, int newLocation)
{
	recordHash[rid] = newLocation;
}


