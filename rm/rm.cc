
#include "rm.h"
#include <string.h>
#include <iostream>
#include <sstream>

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
	//Create system catalog and set the system catalog handle
	pm->CreateFile("System_Catalog");
	pm->OpenFile("System_Catalog", scHandle);
}

RM::~RM()
{
}

RC RM::createTable(const string tableName, const vector<Attribute> &attrs)
{
	//create a blank page
	char pageToWrite[PF_PAGE_SIZE];
	for (int i = 0; i <PF_PAGE_SIZE; i++)
	{
		pageToWrite[i] = 0;
	}
	char *pageToWritePointer = (char*) pageToWrite;

	//prevent duplicate tables from being created
	if(pm->CreateFile(tableName.c_str()) != 0)
	{
		return 0;
	}
	int length1 = 0;
	int length2 = 0;

	/*//Calculate total length of expected attribute
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
	}*/


	//For each attribute set the amount of space needed
	//Also copy the data to a buffer to be appended to the page
	for (unsigned int i = 0; i < attrs.size(); i++)
	{
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

	//If table is empty (page-less), simply append the page
	if (numberOfPages == 0)
	{
		scHandle.AppendPage(pageToWrite);
	}
	else
	{
		//Otherwise find the next available free space and add the new attribute record
		for (int i = 0; i < numberOfPages; i++)
		{
			char data[PF_PAGE_SIZE];
			scHandle.ReadPage(i, (void*) data);
			int x= 0;
			while(x + 3 < PF_PAGE_SIZE && !found)
			{
				int size = 0;
				char temp[sizeof(int)];
				for (unsigned int z = 0; z < sizeof(int); z++)
				{
					temp[z] = data[x+z];
				}
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
			//If there is enough space on the current page, write the new attribute record to it
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
			// Otherwise append it to a new page
			scHandle.AppendPage(pageToWrite);
		}
	}
	return 0;
}

RC RM::deleteTable(const string tableName)
{
	//clear the table
	deleteTuples(tableName);
	//close scHandle (in case the system catalog is the one being destroyed)
	pm->CloseFile(scHandle);
	//destroy file
	pm->DestroyFile(tableName.c_str());
	string x ="System_Catalog";
	//try to reopen system catalog
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
			//check the size of the attribute currently being read
			int attributeSize = 0;
			char temp[sizeof(int)];
			for (unsigned int z = 0; z < sizeof(int); z++)
			{
				temp[z] = data[offset+z];
			}
			char *tempPtr = (char*) temp;
			memcpy(&attributeSize, tempPtr, sizeof(int));
			// if 0, attribute is not on this page...  break
			if(attributeSize == 0)
			{
				break;
			}
			//get table name's size
			for (unsigned int z = 0; z < sizeof(int); z++)
			{
				temp[z] = data[offset+z+sizeof(int)];
			}
			int tableNameSize = 0;
			memcpy(&tableNameSize, tempPtr, sizeof(int));
			string tName = "";
			//get table name
			for (int k = 0; k < tableNameSize; k++)
			{
				tName = tName + data[offset+2*sizeof(int)+k];
			}
			//compare table name of current attribute being read with the one being requested. If match...
			if (strcmp(tName.c_str(), tableName.c_str()) == 0)
			{
				//Extract all data:
				// Length of attribute, table name, attribute name, column number, type, length
				string aName = "";
				for (unsigned int z = 0; z < sizeof(int); z++)
				{
					temp[z] = data[offset+z+2*sizeof(int)+tableNameSize];
				}
				int attNameSize = 0;
				memcpy(&attNameSize, tempPtr, sizeof(int));
				for (int k = 0; k < attNameSize; k++)
				{
					aName = aName + data[offset+sizeof(int)*3+k+tableNameSize];
				}
				for (unsigned int z = 0; z < sizeof(int); z++)
				{
					temp[z] = data[offset+z+4*sizeof(int)+tableNameSize+attNameSize];
				}
				int type = 0;
				memcpy(&type, tempPtr, sizeof(int));

				for (unsigned int z = 0; z < sizeof(int); z++)
				{
					temp[z] = data[offset+z+5*sizeof(int)+tableNameSize+attNameSize];
				}
				int length = 0;
				memcpy(&length, tempPtr, sizeof(int));

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
				//Turn information gathered into an actual attribute
				//Push the attribute into a result vector
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
	int numberOfPages = fh.GetNumberOfPages();

	int slot = -1;
	int page = -1;

	char recordToAdd[PF_PAGE_SIZE];
	char *recordToAddPtr = (char*) recordToAdd;
	char *dataPtr = (char*) data;

	//Check the total size of the record being added
	vector<Attribute> attrs;
	getAttributes(tableName, attrs);
	int tempInt = 0;
	for (unsigned int a = 0; a<attrs.size(); a++)
	{
		tempInt += attrs[a].length;
	}

	memcpy(recordToAddPtr, dataPtr, tempInt);
	unsigned int recordLength = 0;
	for (unsigned int a = 0; a < attrs.size(); a++)
	{
		Attribute attr = attrs[a];
		if (attr.type == TypeVarChar)
		{
			//Get the length of a type var char
			char *tempPtr = (char*) recordToAdd;
			int num = -1;
			tempPtr += recordLength;
			memcpy(&num, tempPtr, sizeof(int));
			recordLength += sizeof(int) + num;
		}
		else
		{
			recordLength += sizeof(int);
		}
	}

	//Make sure page is clean
	for (int i = recordLength; i < PF_PAGE_SIZE; i++)
	{
		recordToAdd[i] = 0;
	}

	// If there are no pages currently, set up slots and other information
	if (numberOfPages == 0)
	{
		char *ptr = (char*) recordToAdd;
		ptr += (PF_PAGE_SIZE - 4 *sizeof(int));
		//Set slot 1 offset to zero
		int zero = 0;
		memcpy(ptr, &zero, sizeof(int));
		// Set next free offset
		ptr += sizeof(int);
		memcpy(ptr, &recordLength, sizeof(int));
		// Set amount of space available
		ptr += sizeof(int);
		int freeSpaceAvailable = PF_PAGE_SIZE - sizeof(int) * 4 - recordLength;
		memcpy(ptr, &freeSpaceAvailable, sizeof(int));
		// Set number of slots
		ptr += sizeof(int);
		int one = 1;
		memcpy(ptr, &one, sizeof(int));
		page = 0;
		slot = 1;
		rid.pageNum = page;
		rid.slotNum = slot;
		fh.AppendPage(recordToAdd);
		pm->CloseFile(fh);
		return 0;
	}
	else
	{
		for (int p = 0; p < numberOfPages; p++)
		{
			char currentPage[PF_PAGE_SIZE];
			fh.ReadPage(p, currentPage);

			//Get amount of free space in current page;
			int num = -1;
			char *tempPtr1 = (char*) currentPage;
			tempPtr1 += (PF_PAGE_SIZE - 2 * sizeof(int));
			memcpy(&num, tempPtr1, sizeof(int));
			unsigned int freeSpaceOnPage = num;

			// If there is free space available, add
			if (recordLength + sizeof(int) < freeSpaceOnPage)
			{
				//Get number of slots currently on page
				char tempArray2[sizeof(int)];
				char *tempArrayPtr2 = (char*) tempArray2;
				int num2 = -1;
				for (unsigned int i = 0; i < sizeof(int); i++)
				{
					tempArray2[i] = currentPage[PF_PAGE_SIZE - 1 * sizeof(int) + i];
				}
				memcpy(&num2, tempArrayPtr2, sizeof(int));
				int slotsOnPage = num2;

				//Calculate offset
				char tempArray3[sizeof(int)];
				char *tempArrayPtr3 = (char*) tempArray3;
				int num3 = -1;
				for (unsigned int i = 0; i < sizeof(int); i++)
				{
					tempArray3[i] = currentPage[PF_PAGE_SIZE - 3 * sizeof(int) + i];
				}
				memcpy(&num3, tempArrayPtr3, sizeof(int));
				int nextAvailableOffset = num3;


				//Put offset in slot pointer of new record
				slotsOnPage++;
				slot = slotsOnPage;
				page = p;
				char *currentPagePtr = (char*) currentPage;
				char *dataPtr = (char*) recordToAdd;
				currentPagePtr += nextAvailableOffset;
				//Put record information in correct position
				memcpy(currentPagePtr, dataPtr, recordLength);
				//Put slot off set in correct slot pointer
				currentPagePtr += (PF_PAGE_SIZE - 3 * sizeof(int) - slotsOnPage * sizeof(int) - nextAvailableOffset);
				memcpy(currentPagePtr, &nextAvailableOffset, sizeof(int));
				//Set next free offset
				nextAvailableOffset += recordLength;
				currentPagePtr += (slotsOnPage * sizeof(int));
				memcpy(currentPagePtr, &nextAvailableOffset, sizeof(int));
				//Set amount of free space avilable
				currentPagePtr += sizeof(int);
				freeSpaceOnPage -= (recordLength + sizeof(int));
				memcpy(currentPagePtr, &freeSpaceOnPage, sizeof(int));
				//Set slot count
				currentPagePtr += sizeof(int);
				memcpy(currentPagePtr, &slotsOnPage, sizeof(int));

				fh.WritePage(p, currentPage);
				pm->CloseFile(fh);
				rid.pageNum = page;
				rid.slotNum = slot;
				return 0;
			}
		}
		//If no place to add... append a new page
		//Initialize directory information
		char *ptr = (char*) recordToAdd;
		ptr += (PF_PAGE_SIZE - 4 *sizeof(int));
		int zero = 0;
		memcpy(ptr, &zero, sizeof(int));
		ptr += sizeof(int);
		memcpy(ptr, &recordLength, sizeof(int));
		ptr += sizeof(int);
		int freeSpaceAvailable = PF_PAGE_SIZE - sizeof(int) * 4 - recordLength;
		memcpy(ptr, &freeSpaceAvailable, sizeof(int));
		ptr += sizeof(int);
		int one = 1;
		memcpy(ptr, &one, sizeof(int));
		page = 0;
		slot = 1;
		rid.pageNum = page;
		rid.slotNum = slot;

		fh.AppendPage(recordToAdd);
		pm->CloseFile(fh);
		return 0;
	}
	pm->CloseFile(fh);
	return 0;
}

RC RM::deleteTuple(const string tableName, const RID &rid)
{
	int page = rid.pageNum;
	int slot = rid.slotNum;
	vector<Attribute> attrs;
	getAttributes(tableName, attrs);

	PF_FileHandle fh;
	pm->OpenFile(tableName.c_str(), fh);
	char pageRead[PF_PAGE_SIZE];
	fh.ReadPage(page, pageRead);
	int start = -1;
	char *pageReadPtr = (char*) pageRead;
	//Set variable "start" to offset of rid
	pageReadPtr +=  (PF_PAGE_SIZE - ((3 + slot) * sizeof(int)));
	memcpy(&start, pageReadPtr, sizeof(int));
	int recordLength = 0;
	//Go through the record's attributes, set them to blank as they come up, until there are no attributes left
	for (unsigned int a = 0; a <attrs.size(); a++)
	{
		if (attrs[a].type == TypeVarChar)
		{
			int stringLength = 0;
			char temp[sizeof(int)];
			char *tempPtr = (char*) temp;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp[i] = pageRead[start + recordLength + i];
				pageRead[start + recordLength + i] = 0;
			}
			memcpy(&stringLength, tempPtr, sizeof(int));
			recordLength += sizeof(int);
			for (int i = 0; i < stringLength; i++)
			{
				pageRead[start + recordLength + i] = 0;
			}
			recordLength += stringLength;
		}
		else
		{
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				pageRead[start + recordLength + i] = 0;
			}
			recordLength += sizeof(int);
		}
	}
	fh.WritePage(page, pageRead);
	pm->CloseFile(fh);
	return 0;
}

RC RM::deleteTuples(const string tableName)
{
	//Select all pages, set all bytes to 0
	PF_FileHandle fh;
	pm->OpenFile(tableName.c_str(), fh);
	int numberOfPages = fh.GetNumberOfPages();
	for (int p = 0; p < numberOfPages; p++)
	{
		char page[PF_PAGE_SIZE];
		for (int i = 0; i < PF_PAGE_SIZE; i++)
		{
			page[i] = 0;
		}
		fh.WritePage(p, page);
	}
	pm->CloseFile(fh);
	return 0;
}


// Assume the rid does not change after update
RC RM::updateTuple(const string tableName, const void *data, const RID &rid)
{
	int page = rid.pageNum;
	int slot = rid.slotNum;
	vector<Attribute> attrs;
	getAttributes(tableName, attrs);

	PF_FileHandle fh;
	pm->OpenFile(tableName.c_str(), fh);
	char pageRead[PF_PAGE_SIZE];
	fh.ReadPage(page, pageRead);
	int start = -1;
	char *pageReadPtr = (char*) pageRead;
	pageReadPtr +=  (PF_PAGE_SIZE - ((3 + slot) * sizeof(int)));
	memcpy(&start, pageReadPtr, sizeof(int));
	int recordLength = 0;
	//get a general size of expected record
	for (unsigned int a = 0; a <attrs.size(); a++)
	{
		if (attrs[a].type == TypeVarChar)
		{
			int stringLength = 0;
			char temp[sizeof(int)];
			char *tempPtr = (char*) temp;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp[i] = pageRead[start + recordLength + i];
			}
			memcpy(&stringLength, tempPtr, sizeof(int));
			recordLength += sizeof(int);
			recordLength += stringLength;
		}
		else
		{
			recordLength += sizeof(int);
		}
	}

	//copy data to a buffer
	char dataPage[PF_PAGE_SIZE];
	char *dataPagePtr = (char*) dataPage;
	char *dataPtr = (char*) data;
	memcpy(dataPagePtr, dataPtr, PF_PAGE_SIZE);

	//check the length of the new record
	int newRecordLength = 0;
	for (unsigned int a = 0; a <attrs.size(); a++)
	{
		if (attrs[a].type == TypeVarChar)
		{
			int stringLength = 0;
			char temp[sizeof(int)];
			char *tempPtr = (char*) temp;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp[i] = dataPage[newRecordLength + i];
			}
			memcpy(&stringLength, tempPtr, sizeof(int));
			newRecordLength += sizeof(int);
			newRecordLength += stringLength;
		}
		else
		{
			newRecordLength += sizeof(int);
		}
	}

	// Check if the new record record is too big to fit
	//If it is...
	if(newRecordLength > recordLength)
	{
		RID newRID;
		int newPage =newRID.pageNum;
		int newSlot = newRID.slotNum;
		//Insert in to a new page
		insertTuple(tableName, data,newRID);
		//Set a "tombstone" with the rid (first int = 0 [so rm knows that it is a tombstone], 2nd int = page, 3rd int = slot)
		int zero = 0;
		char *pageReadPtr2 = (char*) pageRead;
		pageReadPtr2 += start;
		memcpy(pageReadPtr, &zero, sizeof(int));
		pageReadPtr2 += sizeof(int);
		memcpy(pageReadPtr, &newPage, sizeof(int));
		pageReadPtr2 += sizeof(int);
		memcpy(pageReadPtr, &newSlot, sizeof(int));

		for (int i = sizeof(int) * 3; i < recordLength; i++)
		{
			pageRead[start+i] = 0;
		}

		fh.WritePage(page, pageRead);
	}
	//If there's enough room
	else
	{
		//Update the record
		char *pageReadPtr2 = (char*) pageRead;
		pageReadPtr2 += start;
		memcpy(pageReadPtr2, dataPage, newRecordLength);
		// if the record is smaller, set the excess space to 0
		for (int i =0; i <recordLength - newRecordLength; i++)
		{
			pageRead[start+newRecordLength+i] = 0;
		}
		fh.WritePage(page, pageRead);
	}
	pm->CloseFile(fh);
	return 0;
}


RC RM::readTuple(const string tableName, const RID &rid, void *data)
{
	int slot = rid.slotNum;
	int page = rid.pageNum;

	PF_FileHandle fh;
	pm->OpenFile(tableName.c_str(), fh);
	char pageRead[PF_PAGE_SIZE];
	fh.ReadPage(page, pageRead);

	//If there are no pages, return -1
	if (fh.GetNumberOfPages() == 0)
	{
		pm->CloseFile(fh);
		return -1;
	}

	char resultsBuffer[PF_PAGE_SIZE];
	int start = -1;
	int recordLength = 0;
	char *pageReadPtr = (char*) pageRead;
	pageReadPtr +=  (PF_PAGE_SIZE - ((3 + slot) * sizeof(int)));
	memcpy(&start, pageReadPtr, sizeof(int));
	//Get attributes for the record
	vector<Attribute> attrs;
	getAttributes(tableName, attrs);

	//For each attribute, read the information into the result buffer
	for (unsigned int a = 0; a <attrs.size(); a++)
	{
		if (attrs[a].type == TypeVarChar)
		{
			int stringLength = 0;
			char temp[sizeof(int)];
			char *tempPtr = (char*) temp;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp[i] = pageRead[start + recordLength + i];
				resultsBuffer[recordLength + i] = pageRead[start + recordLength + i];
			}
			memcpy(&stringLength, tempPtr, sizeof(int));
			recordLength += sizeof(int);
			for (int i = 0; i < stringLength; i++)
			{
				resultsBuffer[recordLength + i] = pageRead[start + recordLength + i];
			}
			recordLength += stringLength;
		}
		else
		{
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				resultsBuffer[recordLength + i] = pageRead[start + recordLength + i];
			}
			recordLength += sizeof(int);
		}
	}

	char *dataPtr = (char*) data;
	char *rbPtr = (char*) resultsBuffer;
	memcpy(dataPtr, rbPtr, recordLength);

	//testInt/testInt2/testInt3 checks if the record is blank or if it has a tombstone. (if testInt2 and testInt3 do not equal 0, then it is a tombstone)
	int testInt = 0;
	char temp[sizeof(int)];
	char *tempPtr = (char*) temp;
	for (unsigned int i = 0; i < sizeof(int); i++)
	{
		temp[i] = resultsBuffer[i];
	}
	memcpy(&testInt, tempPtr, sizeof(int));

	int testInt2 = 0;
	char temp2[sizeof(int)];
	char *tempPtr2 = (char*) temp2;
	for (unsigned int i = sizeof(int); i < sizeof(int) * 2; i++)
	{
		temp2[i] = resultsBuffer[i];
	}
	memcpy(&testInt2, tempPtr2, sizeof(int));

	if (testInt == 0 && testInt2 == 0)
	{
		pm->CloseFile(fh);
		return -1;
	}
	else if (testInt == 0)
	{
		int testInt3 = 0;
		char temp3[sizeof(int)];
		char *tempPtr3 = (char*) temp3;
		for (unsigned int i = sizeof(int) * 2; i < sizeof(int) * 3; i++)
		{
			temp3[i] = resultsBuffer[i];
		}
		memcpy(&testInt3, tempPtr3, sizeof(int));

		int tempPage = testInt2;
		int tempSlot = testInt3;
		RID tempRID;
		tempRID.pageNum = tempPage;
		tempRID.slotNum = tempSlot;
		//If it is a tombstone, read that tombstone's rid
		readTuple(tableName, tempRID, data);
		pm->CloseFile(fh);
		return 0;
	}
	pm->CloseFile(fh);
	return 0;
}


RC RM::readAttribute(const string tableName, const RID &rid, const string attributeName, void *data)
{
	vector<Attribute> attrs;
	getAttributes(tableName, attrs);

	//Get an expected size of the record
	int tempSize = 0;
	for (unsigned int a = 0; a < attrs.size(); a++)
	{
		tempSize += attrs[a].length;
	}

	char resultsBuffer[tempSize];
	char *rbPtr = (char*) resultsBuffer;
	char *dataPtr = (char*) data;

	//Get the specific record with rid
	readTuple(tableName, rid, resultsBuffer);

	int recordLength = 0;
	//Go through attributes
	for (unsigned int a = 0; a < attrs.size(); a++)
	{
		if (attrs[a].type == TypeVarChar)
		{
			int stringLength = 0;
			char temp[sizeof(int)];
			char *tempPtr = (char*) temp;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp[i] = resultsBuffer[recordLength + i];
			}
			memcpy(&stringLength, tempPtr, sizeof(int));
			//check if the attribute name matches, if yes, add it to the results
			if (strcmp(attributeName.c_str(), attrs[a].name.c_str()) == 0)
			{
				memcpy(dataPtr, rbPtr, sizeof(int) + stringLength);
			}
			recordLength += sizeof(int) +stringLength;
			rbPtr+= sizeof(int)+stringLength;
		}
		else
		{
			//check if the attribute name matches, if yes, add it to the results
			if (strcmp(attributeName.c_str(), attrs[a].name.c_str()) == 0)
			{
				memcpy(dataPtr, rbPtr, sizeof(int));
			}
			recordLength += sizeof(int);
			rbPtr+= sizeof(int);
		}
	}
	return 0;
}


RC RM::reorganizePage(const string tableName, const unsigned pageNumber)
{
	vector<Attribute> attrs;
	getAttributes(tableName, attrs);
	PF_FileHandle fh;
	pm->OpenFile(tableName.c_str(), fh);

	//Determine max record size:
	int maxSize = 0;
	for (unsigned int a = 0; a <attrs.size(); a++)
	{
		maxSize += attrs[a].length;
	}
	char currentPage[PF_PAGE_SIZE];
	fh.ReadPage(pageNumber, currentPage);
	int numberOfSlots = 0;
	char temp[sizeof(int)];
	char *tempPtr = (char*) temp;
	//Get number of slots on page
	for (unsigned int i = 0; i < sizeof(int); i++)
	{
		temp[i] = currentPage[PF_PAGE_SIZE-sizeof(int)+i];
	}
	memcpy(&numberOfSlots, tempPtr, sizeof(int));

	//Information offset (directory information and pointers)
	int informationOffset = PF_PAGE_SIZE - sizeof(int) * (3 + numberOfSlots);

	//For each slot s, check if it is blank
	for (int s = 1; s < numberOfSlots; s++)
	{
		RID rid;
		rid.pageNum = pageNumber;
		rid.slotNum = s;
		cout << "maxSize: " << maxSize << endl;
		char currentRecord[maxSize];
		if (readTuple(tableName, rid, currentRecord) == -1)
		{
			//Get current slot's offset
			int currentSlotOffset = 0;
			char temp[sizeof(int)];
			char *tempPtr = (char*) temp;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp[i] = currentPage[PF_PAGE_SIZE-sizeof(int) * (3 + s)+i];
			}
			memcpy(&currentSlotOffset, tempPtr, sizeof(int));


			//Get next slot's offset
			int nextSlotOffset = 0;
			char temp2[sizeof(int)];
			char *tempPtr2 = (char*) temp2;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp2[i] = currentPage[PF_PAGE_SIZE-sizeof(int) * (3 + s + 1)+i];
			}
			memcpy(&nextSlotOffset, tempPtr2, sizeof(int));


			//Preserve first 3 int's in case it is a tombstone
			currentSlotOffset += 3 *sizeof(int);

			//Check amount of data to be deleted
			int amountToShift = nextSlotOffset - currentSlotOffset;

			for (int i = currentSlotOffset; i < informationOffset; i++)
			{
				if (i+amountToShift >= informationOffset)
				{
					currentPage[i] = 0;
				}
				else
				{
					currentPage[i] = currentPage[i + amountToShift];
				}
			}

			//Shift all remaining slots down, and properly update their offsets
			for (int remainingSlots = s + 1; remainingSlots <= numberOfSlots; remainingSlots++)
			{
				char temp8[sizeof(int)];
				char *temp8Ptr = (char*) temp8;
				int rsOffset = 0;
				for (unsigned int i = 0; i < sizeof(int); i++)
				{
					temp8[i] = currentPage[PF_PAGE_SIZE - sizeof(int) * (3 + remainingSlots) + i];
				}
				memcpy(&rsOffset, temp8Ptr, sizeof(int));
				char *currentPagePtr = (char*) currentPage;
				currentPagePtr += PF_PAGE_SIZE - sizeof(int) * (3 + remainingSlots);
				memcpy(currentPagePtr, &rsOffset, sizeof(int));
			}
			//Set free space available
			char temp9[sizeof(int)];
			char *temp9Ptr = (char*) temp9;
			int freeSpace = 0;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp9[i] = currentPage[PF_PAGE_SIZE - sizeof(int) * 2 + i];
			}
			memcpy(&freeSpace, temp9Ptr, sizeof(int));
			freeSpace -= amountToShift;
			char *currentPagePtr = (char*) currentPage;
			currentPagePtr += PF_PAGE_SIZE - sizeof(int) * 2;
			memcpy(currentPagePtr, &freeSpace, sizeof(int));

			//Set next available offset
			char temp10[sizeof(int)];
			char *temp10Ptr = (char*) temp10;
			int nextAvailableOffset = 0;
			for (unsigned int i = 0; i < sizeof(int); i++)
			{
				temp10[i] = currentPage[PF_PAGE_SIZE - sizeof(int) * 3 + i];
			}
			memcpy(&nextAvailableOffset, temp10Ptr, sizeof(int));
			nextAvailableOffset -= amountToShift;
			char *currentPagePtr2 = (char*) currentPage;
			currentPagePtr2 += PF_PAGE_SIZE - sizeof(int) * 3;
			memcpy(currentPagePtr2, &nextAvailableOffset, sizeof(int));

			fh.WritePage(pageNumber, currentPage);
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
	int pageNum = fg.GetNumberOfPages();
	vector<RID> results;
	// go through every page of the table
	for (int p = 0; p < pageNum; p++)
	{
		int slot = 1;
		char currentPage[PF_PAGE_SIZE];
		fg.ReadPage(p, currentPage);
		//check how many records are in the current page
		int maxNumberOfRecords = 0;
		char tempArray[sizeof(int)];
		char *tempArrayPtr = (char*) tempArray;
		for (int i = PF_PAGE_SIZE - sizeof(int); i < PF_PAGE_SIZE; i++)
		{
			tempArray[i-PF_PAGE_SIZE+sizeof(int)] = currentPage[i];
		}
		memcpy(&maxNumberOfRecords, tempArrayPtr, sizeof(int));

		//for each record, see if it satisfies the iterator request
		for (int r = 0; r < maxNumberOfRecords; r++)
		{
			int count = 0;
			char currentRecord[recordSize];
			RID tempRID;
			tempRID.pageNum = p;
			tempRID.slotNum = slot;
			readTuple(tableName,tempRID, currentRecord);
			for (unsigned int a = 0; a < att.size(); a++)
			{
				if (att[a].type == TypeVarChar)
				{
					char temp[sizeof(int)];
					for (unsigned int i =0 ; i <sizeof(int); i++)
					{
						temp[i] = currentRecord[count+i];
					}
					int num = 0;
					char *tempPtr = (char*) temp;
					memcpy(&num, tempPtr, sizeof(int));
					// If attribute name being compared matches, compare the values
					// If the values satisfy the comparison property, the RID is added to a list
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
					char temp[sizeof(int)];
					for (unsigned int i =0 ; i <sizeof(int); i++)
					{
						temp[i] = currentRecord[count+i];
					}
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
	// list of RIDs = RIDs that have satisfied the comparison
	int recordSize = 0;
	char recordBuffer[PF_PAGE_SIZE];
	// return RM_EOF, end of file, if there are no more RIDs to extract
	if (counter == listOfRIDs.size())
	{
		return RM_EOF;
	}
	else
	{
		//Extract attributes desried from record RID
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
		//Go through each attribute
		for (unsigned int i = 0; i < attrs.size(); i++)
		{
			if (attrs[i].type == TypeVarChar)
			{
				char temp[sizeof(int)];
				for (unsigned int z =0 ; z <sizeof(int); z++)
				{
					temp[z] = recordTemp[counterX+z];
				}
				int num = 0;
				char *tempPtr = (char*) temp;
				memcpy(&num, tempPtr, sizeof(int));

				for (unsigned int z = 0; z < attributeList.size(); z++)
				{
					//If the attribute is a desired attribute, add it to the result buffer
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
						char temp[sizeof(int)];
						for (unsigned int i =0 ; i <sizeof(int); i++)
						{
							temp[i] = recordTemp[counterX+i];
						}
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

	//Set the results to the resultBuffer;
	char *resultsPTR = (char*) results;
	for (int i = 0; i < recordSize; i++)
	{
		results[i] = recordBuffer[i];
	}

	char temp[sizeof(int)];
	for (unsigned int i =0 ; i <sizeof(int); i++)
	{
		temp[i] = results[i];
	}
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
	//Initialize variables
	listOfRIDs = ridList;
	attributeList = aList;
	tableName = tName;
	counter = cc;
	return 0;
}

