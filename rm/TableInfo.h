/*
 * TableInfo.h
 *
 *  Created on: Apr 24, 2013
 *      Author: Emma
 */

#include <ext/hash_map>
using namespace __gnu_cxx;
using namespace std;
#ifndef TABLEINFO_H_
#define TABLEINFO_H_


class TableInfo {
public:
	TableInfo(string tableName, vector<Attribute> listOfAttributes);
	virtual ~TableInfo();
	void addAttribute(Attribute attribute);
	bool removeAttribute(Attribute attribute);
	vector<Attribute> getAttributes();
	int getRecordOffset(int rid);
	hash_map<RID,int> getRecordHash();
	void changeLocation(RID rid, int newLocation)
	void addRecordLocation(RID rid,int offset);


private:
	vector<Attribute> listOfAttributes;
	string tableName;
	hash_map<RID,int> recordHash;

};

#endif /* TABLEINFO_H_ */
