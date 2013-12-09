/*
 * Labyrinthus. Network protocol. http://labyrinthus.org
 *
 * Copyright (c) 2013 Alchemista IT-Consulting, Corp. 
 *
 * Functional diagram by Alexey Vokhmyanin <alchemista@alchemista.info>
 *
 * Written by Alexey Orlov <development@alchemista.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Licens
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-
 */

#ifndef _RSS_OBJECT_INC
#define _RSS_OBJECT_INC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <vector>
#include <string>
#include <sys/types.h>

//#define RSSO_DEBUG

class RSSObject{
private:
	std::vector<std::string*> struct_types;
	std::vector<std::string*> struct_names;
	std::vector<void*> struct_values;
	std::string* _str_dyn;
	std::string* _str_int;
	std::string* _str_word;
	std::string* _str_char;
	std::string structname;
typedef struct{
	unsigned int datalen;
}dyn_t;

typedef struct{
	unsigned int count;
}arr_t;
const char** structs;
std::string struct_description;
	int getIdentOfObjectByName(const char* name);
	void allocate_by_id(int ident);
	void free_by_id(int ident);
	void reinit_by_id(int ident);
public:

#ifdef RSSO_DEBUG
std::string allocator_filename;
int allocator_line;
#endif

	RSSObject(const char** structs,const char* structname
#ifdef RSSO_DEBUG
,const char* allocator_filename,const int allocator_line
#endif
		);
	virtual ~RSSObject();
	const char* getStructName();
	uint parse(std::string* bindata);
	uint parse(const char* data,uint len);
	bool encode(std::string * enc_to);
	RSSObject* objectByName(const char* name);
	RSSObject* operator[] (const char* name);
	std::vector<RSSObject*> *arrayObjectByName(const char* name);
	unsigned long uintObjectByName(const char* name);
	bool setUintObjectByName(const char* name,unsigned long val);
	std::string* dynObjectByName(const char* name);
	RSSObject* copy();//return RSSObject
	unsigned int getStructRows();
	unsigned int getCrc();
	bool assign(RSSObject* fromobj); 
	bool compare(RSSObject* withobj); 
	bool compareType(RSSObject* withobj); 
	bool clear();
	std::string* getObjectStructDescription();
	std::string* getStructRowNameById(unsigned int ident);
	std::string* getStructRowTypeById(unsigned int ident);
	void* getStructRowValueById(unsigned int ident);
	void printDump(const char* withtext);
};

#ifdef RSSO_DEBUG
void RSSObject_print_allocated(FILE * w);

#ifndef RSSO_WITHOUT_DEFINE_DEBUG
#define RSSObject(a,b) RSSObject(a,b,__FILE__,__LINE__)
#endif


#endif

#endif
