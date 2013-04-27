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


typedef enum { TypeInt = 0, TypeReal, TypeVarChar } AttrType;

typedef unsigned AttrLength;

struct Attribute {
    string   name;     // attribute name
    AttrType type;     // attribute type
    AttrLength length; // attribute length
};

typedef struct
{
  unsigned pageNum;
  unsigned slotNum;
} RID;


class TableInfo {
public:
	TableInfo(string tableName, vector<Attribute> listOfAttributes);
	virtual ~TableInfo();
	void addAttribute(Attribute attribute);
	bool removeAttribute(Attribute attribute);
	vector<Attribute> getAttributes();
	int getRecordOffset(RID rid);
	hash_map<RID,int> getRecordHash();
	void changeLocation(RID rid, int newLocation);
	void addRecordLocation(RID rid,int offset);


private:
	vector<Attribute> listOfAttributes;
	string tableName;
	hash_map<RID,int> recordHash;

};

#endif /* TABLEINFO_H_ */
