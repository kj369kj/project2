using namespace std;
#include <iostream>
#include <sstream>
#include <string.h>
#include "rm.h"

RM* RM::_rm = 0;

RM* RM::Instance()
{
	if(!_rm)
		_rm = new RM();

	return _rm;
}

RM::RM()
{
	pm = PF_Manager::Instance();
	//Need: Table name (String), Attribute Name (string), Type (int), Length (int)
	Attribute tableName;
	tableName.name = "Table Name";
	tableName.type = TypeVarChar;
	tableName.length = PF_PAGE_SIZE/16;

	Attribute attributeName;
	attributeName.name = "Attribute Name";
	attributeName.type = TypeVarChar;
	attributeName.length = PF_PAGE_SIZE/16;

	Attribute type;
	type.name = "Type";
	type.type = TypeInt;
	type.length = sizeof(int);

	Attribute length;
	length.name = "Length";
	length.type = TypeInt;
	length.length = sizeof(int);

	vector<Attribute> a;
	a.push_back(tableName);
	a.push_back(attributeName);
	a.push_back(type);
	a.push_back(length);

	pm->CreateFile("System_Catalog");
	createTable("System_Catalog", a);
	pm->OpenFile("System_Catalog", scHandle);

}

RM::~RM()
{
}

RC RM::createTable(const string tableName, const vector<Attribute> &attrs)
{
	char pageToWrite[PF_PAGE_SIZE];
	for (int i = 0; i <PF_PAGE_SIZE; i++)
	{
		pageToWrite[i] = 0;
	}
	char *pageToWritePointer = (char*) pageToWrite;

	pm->CreateFile(tableName.c_str());
	int length1 = 0;
	int length2 = 0;

	for (unsigned int i = 0; i < attrs.size(); i++)
	{
		//Need: Table name (String), Attribute Name (string), Type (int), Length (int)
		//Table Name
		int tableNameSize = tableName.size();
		length1 += sizeof(tableNameSize);
		length1 += tableNameSize;
		int attributeNameSize = attrs[i].name.size();
		length1 += sizeof(attributeNameSize);
		length1 += attributeNameSize;
		AttrType type = attrs[i].type;
		length1 += sizeof(type);
		AttrLength length = attrs[i].length;
		length1 += sizeof(length);
		length1 += sizeof(length1);

	}

	for (unsigned int i = 0; i < attrs.size(); i++)
	{
		//Need: Overall Length (int), Table name (String), Attribute Name (string), Type (int), Length (int)

		//length1 = length1 + sizeof(length1);
		//cout << "Length: " << length1 << endl;
		//memcpy(pageToWritePointer, &length1, sizeof(length1));
		//pageToWritePointer = pageToWritePointer + sizeof(length1);


		//Total Length of attribute
		length1 = 0;
		length1 = 4 + 4 + tableName.size() + 4 + attrs[i].name.size() + 4  + 4 + 4;
		memcpy(pageToWritePointer, &length1, sizeof(length1));
		pageToWritePointer = pageToWritePointer + sizeof(length1);

		//Table Name
		int tableNameSize = tableName.size();
		memcpy(pageToWritePointer, &tableNameSize, sizeof(tableNameSize));
		pageToWritePointer = pageToWritePointer + sizeof(tableNameSize);
		length2 += sizeof(tableNameSize);

		memcpy(pageToWritePointer, tableName.c_str(), tableNameSize);
		pageToWritePointer = pageToWritePointer + tableNameSize;
		length2 += tableNameSize;

		//Attribute Name
		int attributeNameSize = attrs[i].name.size();
		memcpy(pageToWritePointer, &attributeNameSize, sizeof(attributeNameSize));
		pageToWritePointer = pageToWritePointer + sizeof(attributeNameSize);
		length2 += sizeof(attributeNameSize);

		memcpy(pageToWritePointer, attrs[i].name.c_str(), attributeNameSize);
		pageToWritePointer = pageToWritePointer + attributeNameSize;
		length2 += attributeNameSize;

		//Column Number
		int col = i;
		memcpy(pageToWritePointer, &col, sizeof(col));
		pageToWritePointer = pageToWritePointer + sizeof(col);
		length2 += sizeof(col);

		//Type
		AttrType type = attrs[i].type;
		memcpy(pageToWritePointer, &type, sizeof(type));
		pageToWritePointer = pageToWritePointer + sizeof(type);
		length2 += sizeof(type);

		//Length
		AttrLength length = attrs[i].length;
		memcpy(pageToWritePointer, &length, sizeof(length));
		pageToWritePointer = pageToWritePointer + sizeof(length);
		length2 += sizeof(length);
	}

	int numberOfPages = scHandle.GetNumberOfPages();
	int pageNum = -1;
	int offset = -1;
	bool found = false;

	if (numberOfPages == 0)
	{
		scHandle.AppendPage(pageToWrite);
	}
	else
	{
		for (int i = 0; i < numberOfPages; i++)
		{
			char data[PF_PAGE_SIZE];
			scHandle.ReadPage(i, (void*) data);
			int x= 0;
			while(x + 3 < PF_PAGE_SIZE && !found)
			{
				int size = 0;
				char temp[4];
				temp[0] = data[x];
				temp[1] = data[x+1];
				temp[2] = data[x+2];
				temp[3] = data[x+3];
				char *ptr = (char*) temp;
				memcpy(&size, ptr, 4);
				if (size == 0)
				{
					found = true;
				}
				x = x + size;
			}
			if (length1 + x >= PF_PAGE_SIZE)
			{
				found = false;
			}
			else
			{
				pageNum = i;
				offset = x;
			}
		}
		if (found)
		{
			char data[PF_PAGE_SIZE];
			scHandle.ReadPage(pageNum, (void*) data);
			for (int i = offset; i < PF_PAGE_SIZE; i++)
			{
				data[i] = pageToWrite[i-offset];
			}
			scHandle.WritePage(pageNum, data);
		}
		else
		{
			scHandle.AppendPage(pageToWrite);
		}
	}
	return 0;
}

RC RM::deleteTable(const string tableName)
{
	deleteTuples(tableName);
	pm->CloseFile(scHandle);
	pm->DestroyFile(tableName.c_str());
	string x ="System_Catalog";
	pm->OpenFile(x.c_str(), scHandle);
	return 0;
}

RC RM::getAttributes(const string tableName, vector<Attribute> &attrs)
{
	int pageNum = scHandle.GetNumberOfPages();
	for (int i = 0; i < pageNum; i++)
	{
		char data[PF_PAGE_SIZE];
		scHandle.ReadPage(i, (void*) data);

		int offset = 0;
		bool found = false;
		while (!found)
		{
			int attributeSize = 0;
			char temp[4];
			temp[0] = data[offset + 0];
			temp[1] = data[offset + 1];
			temp[2] = data[offset + 2];
			temp[3] = data[offset + 3];
			char *tempPtr = (char*) temp;
			memcpy(&attributeSize, tempPtr, 4);

			if(attributeSize == 0)
			{
				break;
			}

			temp[0] = data[offset + 4];
			temp[1] = data[offset + 5];
			temp[2] = data[offset + 6];
			temp[3] = data[offset + 7];
			int tableNameSize = 0;
			memcpy(&tableNameSize, tempPtr, 4);
			string tName = "";
			for (int k = 0; k < tableNameSize; k++)
			{
				tName = tName + data[offset+4+4+k];
			}

			if (strcmp(tName.c_str(), tableName.c_str()) == 0)
			{
				// Length of attribute, table name, attribute name, column number, type, length
				string aName = "";
				temp[0] = data[offset + 8 + tableNameSize];
				temp[1] = data[offset + 9 + tableNameSize];
				temp[2] = data[offset + 10 + tableNameSize];
				temp[3] = data[offset + 11 + tableNameSize];
				int attNameSize = 0;
				memcpy(&attNameSize, tempPtr, 4);
				for (int k = 0; k < attNameSize; k++)
				{
					aName = aName + data[offset+4+4+4+k+tableNameSize];
				}

				temp[0] = data[offset + 16 + tableNameSize + attNameSize];
				temp[1] = data[offset + 17 + tableNameSize + attNameSize];
				temp[2] = data[offset + 18 + tableNameSize + attNameSize];
				temp[3] = data[offset + 19 + tableNameSize + attNameSize];
				int type = 0;
				memcpy(&type, tempPtr, 4);

				temp[0] = data[offset + 20 + tableNameSize + attNameSize];
				temp[1] = data[offset + 21 + tableNameSize + attNameSize];
				temp[2] = data[offset + 22 + tableNameSize + attNameSize];
				temp[3] = data[offset + 23 + tableNameSize + attNameSize];
				int length = 0;
				memcpy(&length, tempPtr, 4);

				Attribute x;
				x.name = aName;
				if (type == 0)
				{
					x.type = TypeInt;
				}
				else if (type == 1)
				{
					x.type = TypeReal;
				}
				else
				{
					x.type = TypeVarChar;
				}
				x.length = (AttrLength) length;
				attrs.push_back(x);
			}

			offset = offset + attributeSize;
			if (offset > PF_PAGE_SIZE)
			{
				found = true;
			}
		}
	}
	return 0;
}

//  Format of the data passed into the function is the following:
//  1) data is a concatenation of values of the attributes
//  2) For int and real: use 4 bytes to store the value;
//     For varchar: use 4 bytes to store the length of characters, then store the actual characters.
//  !!!The same format is used for updateTuple(), the returned data of readTuple(), and readAttribute()
RC RM::insertTuple(const string tableName, const void *data, RID &rid)
{
	PF_FileHandle fh;
	pm->OpenFile(tableName.c_str(), fh);
	vector<Attribute> att;
	getAttributes(tableName, att);
	int totalRecordSize = 0;
	for (unsigned int i = 0; i < att.size(); i++)
	{
		totalRecordSize += att[i].length;
	}
	int numberOfPages = fh.GetNumberOfPages();
	int pageNum = -1;
	bool found = false;

	char recordToAdd[PF_PAGE_SIZE];
	char *recToAddPtr22 = (char*) recordToAdd;
	char *dataPtr22 = (char*) data;
	memcpy(recToAddPtr22, dataPtr22, totalRecordSize);
	int recordSize = 0;
	int slot = 0;
	for (unsigned int i = 0; i < att.size(); i++)
	{
		if (att[i].type == TypeVarChar)
		{
			char temp[4];
			char *tempPtr = (char*) temp;
			temp[0] = recordToAdd[recordSize];
			temp[1] = recordToAdd[recordSize+1];
			temp[2] = recordToAdd[recordSize+2];
			temp[3] = recordToAdd[recordSize+3];
			int tempNum = 0;
			memcpy(&tempNum, tempPtr, sizeof(int));

			recordSize += sizeof(int) + tempNum;
		}
		else
		{
			recordSize += sizeof(int);
		}
	}
	for (int i = recordSize; i < PF_PAGE_SIZE; i++)
	{
		recordToAdd[i] = 0;
	}
	if (numberOfPages == 0)
	{
		fh.AppendPage(recordToAdd);
		pageNum = 0;
		found = true;
	}
	else
	{
		for (int i = 0; i < numberOfPages; i++)
		{
			int slotCount = 0;
			char pageRead[PF_PAGE_SIZE];
			fh.ReadPage(i, pageRead);
			int maxRecords = PF_PAGE_SIZE/totalRecordSize;
			for (int z = 0; z < maxRecords; z++)
			{
				char temp[4];
				char *tempPtr = (char*) temp;
				temp[0] = pageRead[z*totalRecordSize];
				temp[1] = pageRead[z*totalRecordSize+1];
				temp[2] = pageRead[z*totalRecordSize+2];
				temp[3] = pageRead[z*totalRecordSize+3];
				int t = 0;
				memcpy(&t, tempPtr, sizeof(int));
				if (t == 0 && !found)
				{
					found = true;
					for (int w = 0; w < recordSize; w++)
					{
						pageRead[z*totalRecordSize+w] = recordToAdd[w];
						fh.WritePage(i, pageRead);
						pageNum = i;
						slot = slotCount;
					}
				}
				slotCount++;
			}

		}
		if (!found)
		{
			fh.AppendPage(recordToAdd);
			pageNum = fh.GetNumberOfPages();
			slot = 0;
		}
	}
	rid.pageNum = pageNum;
	rid.slotNum = slot;
	pm->CloseFile(fh);
	return 0;
}

RC RM::deleteTuples(const string tableName)
{
	PF_FileHandle fg;
	pm->OpenFile(tableName.c_str(), fg);
	int numberOfPages = fg.GetNumberOfPages();

	for (int i = 0; i < numberOfPages; i++)
	{
		char page[PF_PAGE_SIZE];
		fg.ReadPage(i, page);
		for (int x = 0; x < PF_PAGE_SIZE; x++)
		{
			page[x] = 0;
		}
		fg.WritePage(i, page);
	}
	pm->CloseFile(fg);
	//pm->DestroyFile(tableName.c_str());
	//pm->CreateFile(tableName.c_str());
	return 0;
}

RC RM::deleteTuple(const string tableName, const RID &rid)
{
	int slot = rid.slotNum;
	int page = rid.pageNum;
	vector<Attribute> attr;
	getAttributes(tableName, attr);
	int totalRecordSize = 0;
	for (unsigned int i = 0; i < attr.size(); i++)
	{
		totalRecordSize += attr[i].length;
	}

	PF_FileHandle fg;
	pm->OpenFile(tableName.c_str(), fg);
	char pageRead[PF_PAGE_SIZE];
	fg.ReadPage(page, pageRead);

	for (int i = slot * totalRecordSize; i < slot*totalRecordSize + totalRecordSize; i++)
	{
		pageRead[i] = 0;
	}
	fg.WritePage(page, pageRead);
	pm->CloseFile(fg);
	return 0;
}

// Assume the rid does not change after update
RC RM::updateTuple(const string tableName, const void *data, const RID &rid)
{
	int slot = rid.slotNum;
	int page = rid.pageNum;
	vector<Attribute> attr;
	getAttributes(tableName, attr);

	int totalRecordSize = 0;
	for (unsigned int i = 0; i < attr.size(); i++)
	{
		totalRecordSize += attr[i].length;
	}

	PF_FileHandle fg;
	pm->OpenFile(tableName.c_str(), fg);
	char pageRead[PF_PAGE_SIZE];
	fg.ReadPage(page, pageRead);

	char updateInfo[PF_PAGE_SIZE];
	char *a = (char*) updateInfo;
	char *b = (char*) data;
	memcpy(a, b, PF_PAGE_SIZE);

	for (int i = 0; i < totalRecordSize; i++)
	{
		pageRead[slot*totalRecordSize+i] = updateInfo[i];
	}
	fg.WritePage(page, pageRead);
	pm->CloseFile(fg);
	return 0;
}

RC RM::readTuple(const string tableName, const RID &rid, void *data)
{
	int slot = rid.slotNum;
	int page = rid.pageNum;
	vector<Attribute> attr;
	getAttributes(tableName, attr);
	int totalRecordSize = 0;
	for (unsigned int i = 0; i < attr.size(); i++)
	{
		totalRecordSize += attr[i].length;
	}

	PF_FileHandle fT;
	pm->OpenFile(tableName.c_str(), fT);
	char pageRead[PF_PAGE_SIZE];
	fT.ReadPage(page, pageRead);

	int start = slot * totalRecordSize;
	int end = slot * totalRecordSize + totalRecordSize;
	void *dataPtr = (void *) data;
	char *pageReadPtr = (char *) pageRead;
	pageReadPtr += start;
	int offset = end - start;
	memcpy(dataPtr, pageReadPtr, offset);
	char temp[4];
	char *tempPtr = (char*) temp;
	temp[0] = pageRead[start];
	temp[1] = pageRead[start+1];
	temp[2] = pageRead[start+2];
	temp[3] = pageRead[start+3];
	int x = 0;
	memcpy(&x, tempPtr, sizeof(int));
	if (x == 0)
	{
		return -1;
	}

	pm->CloseFile(fT);
	return 0;
}

RC RM::readAttribute(const string tableName, const RID &rid, const string attributeName, void *data)
{
	vector<Attribute> attr;
	getAttributes(tableName, attr);
	int totalRecordSize = 0;
	for (unsigned int i = 0; i < attr.size(); i++)
	{
		totalRecordSize += attr[i].length;
	}
	char data2[PF_PAGE_SIZE];
	char result[PF_PAGE_SIZE];
	char *resultPtr = (char*) result;
	readTuple(tableName, rid, data2);
	int counter = 0;
	for (unsigned int i = 0; i < attr.size(); i++)
	{
		if (attr[i].type == TypeVarChar)
		{
			char temp[4];
			char *tempPtr = (char*) temp;
			temp[0] = data2[counter];
			temp[1] = data2[counter+1];
			temp[2] = data2[counter+2];
			temp[3] = data2[counter+3];
			int t = 0;
			memcpy(&t, tempPtr, sizeof(int));
			if (strcmp(attr[i].name.c_str(), attributeName.c_str()) == 0)
			{
				for (int z = 0; z < t; z++)
				{
					result[z] = data2[counter+3+z];
				}
				void *ptr = (void*) data;
				memcpy(ptr, resultPtr, t);
				return 0;
			}
			counter += t + sizeof(int);
		}
		else
		{
			char temp[4];
			char *tempPtr = (char*) temp;
			temp[0] = data2[counter];
			temp[1] = data2[counter+1];
			temp[2] = data2[counter+2];
			temp[3] = data2[counter+3];
			int t = 0;
			memcpy(&t, tempPtr, sizeof(int));
			if (strcmp(attr[i].name.c_str(), attributeName.c_str()) == 0)
			{
				result[0] = data2[counter];
				result[1] = data2[counter+1];
				result[2] = data2[counter+2];
				result[3] = data2[counter+3];
				void *ptr = (void*) data;
				memcpy(ptr, resultPtr, sizeof(int));
				return 0;
			}
			counter += sizeof(int);
		}
	}
	return -1;
}

RC RM::reorganizePage(const string tableName, const unsigned pageNumber)
{
	vector<Attribute> att;
	getAttributes(tableName, att);
	int recordSize = 0;
	for (unsigned int i = 0; i < att.size(); i++)
	{
		recordSize += att[i].length;
	}

	PF_FileHandle fh;
	pm->OpenFile(tableName.c_str(), fh);
	int recordsPerPage = PF_PAGE_SIZE/recordSize;

	char pageRead[PF_PAGE_SIZE];
	fh.ReadPage(pageNumber, pageRead);
	for (int r1 = 0; r1 < recordsPerPage; r1++)
	{
		char firstIntR1[4];
		firstIntR1[0] = pageRead[r1*recordSize];
		firstIntR1[1] = pageRead[r1*recordSize+1];
		firstIntR1[2] = pageRead[r1*recordSize+2];
		firstIntR1[3] = pageRead[r1*recordSize+3];
		int firstInt = 0;
		char *ptr = (char*) firstIntR1;
		memcpy(&firstInt, ptr, sizeof(int));

		if (firstInt == 0)
		{
			for (int r2 = r1; r2 < recordsPerPage - 1; r2++)
			{
				for (int i = 0; i < recordSize; i++)
				{
					char temp[recordSize];
					temp[i] = pageRead[r2*recordSize+i+recordSize];
					pageRead[r2*recordSize+i+recordSize] = pageRead[r2*recordSize+i];
					pageRead[r2*recordSize+i] = temp[i];
				}
			}
			fh.WritePage(pageNumber, pageRead);
		}
	}
	pm->CloseFile(fh);
	return 0;
}

// scan returns an iterator to allow the caller to go through the results one by one.
RC RM::scan(const string tableName,
		const string conditionAttribute,
		const CompOp compOp,                  // comparision type such as "<" and "="
		const void *value,                    // used in the comparison
		const vector<string> &attributeNames, // a list of projected attributes
		RM_ScanIterator &rm_ScanIterator)
{
	PF_FileHandle fg;
	pm->OpenFile(tableName.c_str(), fg);
	int valueInt = 0;
	void *valuePtr = (void*) value;
	if (value != NULL)
	{
		memcpy(&valueInt, valuePtr, sizeof(int));
	}
	vector<string> vec = attributeNames;
	vector<Attribute> att;
	getAttributes(tableName, att);
	int recordSize = 0;
	for (unsigned int i = 0; i < att.size(); i++)
	{
		recordSize += att[i].length;
	}
	int maxNumberOfRecords = PF_PAGE_SIZE/recordSize;
	int pageNum = fg.GetNumberOfPages();
	vector<RID> results;
	for (int p = 0; p < pageNum; p++)
	{
		int slot = 0;
		char currentPage[PF_PAGE_SIZE];
		fg.ReadPage(p, currentPage);
		for (int r = 0; r < maxNumberOfRecords; r++)
		{
			int count = 0;
			for (unsigned int a = 0; a < att.size(); a++)
			{
				if (att[a].type == TypeVarChar)
				{
					char temp[4];
					temp[0] = currentPage[r*recordSize+count];
					temp[1] = currentPage[r*recordSize+count+1];
					temp[2] = currentPage[r*recordSize+count+2];
					temp[3] = currentPage[r*recordSize+count+3];
					int num = 0;
					char *tempPtr = (char*) temp;
					memcpy(&num, tempPtr, sizeof(int));

					if (strcasecmp(att[a].name.c_str(), conditionAttribute.c_str()) == 0 || (strcmp(conditionAttribute.c_str(), "") == 0 && compOp == NO_OP))
					{
						if (compOp == EQ_OP)
						{
							if (num == valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp == LT_OP)
						{
							if (num < valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}

						}
						else if (compOp == GT_OP)
						{
							if (num > valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp == LE_OP)
						{
							if (num <= valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp == GE_OP)
						{
							if (num >= valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp == NE_OP)
						{
							if (num != valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp == NO_OP)
						{
							if (num != 0)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}

					}
					count = num + sizeof(int);
				}
				else
				{
					char temp[4];
					temp[0] = currentPage[r*recordSize+count];
					temp[1] = currentPage[r*recordSize+count+1];
					temp[2] = currentPage[r*recordSize+count+2];
					temp[3] = currentPage[r*recordSize+count+3];
					int num = 0;
					char *tempPtr = (char*) temp;
					memcpy(&num, tempPtr, sizeof(int));

					count += sizeof(int);

					if (strcasecmp(att[a].name.c_str(), conditionAttribute.c_str()) == 0 || (strcmp(conditionAttribute.c_str(), "") == 0 && compOp == NO_OP && a == 0))
					{
						if (compOp == EQ_OP)
						{
							if (num == valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp > LT_OP)
						{
							if (num == valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp < GT_OP)
						{
							if (num == valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp <= LE_OP)
						{
							if (num == valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp >= GE_OP)
						{
							if (num == valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp != NE_OP)
						{
							if (num == valueInt)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}
						else if (compOp == NO_OP)
						{
							if (num != 0)
							{
								RID rid;
								rid.pageNum = p;
								rid.slotNum = slot;
								results.push_back(rid);
							}
						}

					}
				}
			}
			slot++;
		}
	}
	rm_ScanIterator.set(tableName, results, attributeNames, 0);
	pm->CloseFile(fg);
	return 0;
}

RC RM_ScanIterator::getNextTuple(RID &rid, void *data) // { return RM_EOF; };
{
	int recordSize = 0;
	char recordBuffer[PF_PAGE_SIZE];
	if (counter == listOfRIDs.size())
	{
		return RM_EOF;
	}
	else
	{
		RID rid;
		rid = listOfRIDs[counter];
		RM *rm = RM::Instance();
		vector<Attribute> attrs;
		rm->getAttributes(tableName.c_str(), attrs);
		char recordTemp[PF_PAGE_SIZE];
		rm->readTuple(tableName, rid, recordTemp);

		char record[PF_PAGE_SIZE];
		rm->readTuple(tableName, rid, record);
		recordSize = 0;
		int counterX = 0;
		for (unsigned int i = 0; i < attrs.size(); i++)
		{
			if (attrs[i].type == TypeVarChar)
			{
				char temp[4];
				temp[0] = recordTemp[counterX];
				temp[1] = recordTemp[counterX+1];
				temp[2] = recordTemp[counterX+2];
				temp[3] = recordTemp[counterX+3];
				int num = 0;
				char *tempPtr = (char*) temp;
				memcpy(&num, tempPtr, sizeof(int));

				for (unsigned int z = 0; z < attributeList.size(); z++)
				{
					if (strcmp(attributeList[z].c_str(), attrs[i].name.c_str()) == 0)
					{
						for (unsigned int x = 0; x < num + sizeof(int); x++)
						{
							recordBuffer[recordSize+x] = record[counterX+x];
						}
						recordSize += sizeof(int) + num;
					}
				}
				counterX += num + sizeof(int);
			}
			else
			{
				for (unsigned int z = 0; z < attributeList.size(); z++)
				{
					if (strcmp(attributeList[z].c_str(), attrs[i].name.c_str()) == 0)
					{
						char temp[4];
						temp[0] = recordTemp[counterX];
						temp[1] = recordTemp[counterX+1];
						temp[2] = recordTemp[counterX+2];
						temp[3] = recordTemp[counterX+3];
						int num = 0;
						char *tempPtr = (char*) temp;
						memcpy(&num, tempPtr, sizeof(int));

						for (unsigned int x = 0; x < sizeof(int); x++)
						{
							recordBuffer[recordSize+x] = record[counterX+x];
						}
						recordSize += sizeof(int);
					}
				}
				counterX += sizeof(int);
			}
		}

	}
	char results[recordSize];



	char *resultsPTR = (char*) results;
	for (int i = 0; i < recordSize; i++)
	{
		results[i] = recordBuffer[i];
	}

	char temp[4];
	temp[0] = results[0];
	temp[1] = results[1];
	temp[2] = results[2];
	temp[3] = results[3];
	int num = 0;
	char *tempPtr = (char*) temp;
	memcpy(&num, tempPtr, sizeof(int));
	counter++;

	void *dataPTR = (void*) data;
	memcpy(dataPTR, resultsPTR, recordSize);
	return 0;
}

RC RM_ScanIterator::set(string tName, vector<RID> ridList, vector<string> aList, int cc)
{
	listOfRIDs = ridList;
	attributeList = aList;
	tableName = tName;
	counter = cc;
	return 0;
}
// Extra credit
RC RM::dropAttribute(const string tableName, const string attributeName)
{
	return -1;
}

RC RM::addAttribute(const string tableName, const Attribute attr)
{
	return -1;
}

RC RM::reorganizeTable(const string tableName)
{
	vector<Attribute> att;
	getAttributes(tableName, att);
	int recordSize = 0;
	for (unsigned int i = 0; i < att.size(); i++)
	{
		recordSize += att[i].length;
	}

	PF_FileHandle fh;
	pm->OpenFile(tableName.c_str(), fh);
	int recordsPerPage = PF_PAGE_SIZE/recordSize;
	int numOfPages = fh.GetNumberOfPages();

	for (int p = 0; p < numOfPages; p++)
	{
		char pageRead[PF_PAGE_SIZE];
		fh.ReadPage(p, pageRead);
		for (int r1 = 0; r1 < recordsPerPage; r1++)
		{
			char firstIntR1[4];
			firstIntR1[0] = pageRead[r1*recordSize];
			firstIntR1[1] = pageRead[r1*recordSize+1];
			firstIntR1[2] = pageRead[r1*recordSize+2];
			firstIntR1[3] = pageRead[r1*recordSize+3];
			int firstInt = 0;
			char *ptr = (char*) firstIntR1;
			memcpy(&firstInt, ptr, sizeof(int));

			if (firstInt == 0)
			{
				for (int r2 = r1; r2 < recordsPerPage - 1; r2++)
				{
					for (int i = 0; i < recordSize; i++)
					{
						char temp[recordSize];
						temp[i] = pageRead[r2*recordSize+i+recordSize];
						pageRead[r2*recordSize+i+recordSize] = pageRead[r2*recordSize+i];
						pageRead[r2*recordSize+i] = temp[i];
					}
				}
				fh.WritePage(p, pageRead);
			}
		}
	}
	pm->CloseFile(fh);
	return 0;
}
