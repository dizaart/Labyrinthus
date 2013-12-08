/*
 * Labyrinthus network protocol. http://labyrinthus.org
 *
 * Copyright (c) 2013 Alchemista IT-Consulting, Corp. <administrator@alchemista.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Licens
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-
 */

#define RSSO_WITHOUT_DEFINE_DEBUG
#include "rssobject.h"

void RSSObject::allocate_by_id(int ident){
	int i=ident;
	void* vd;
			if(*struct_types[i]==*_str_dyn){
				std::string* s = new std::string();
				vd=(void*)s;
			}else if ( (index(struct_types[i]->data(),'[')) ){
				std::vector<void*>* v = new std::vector<void*>();
				vd=(void*)v;
			}else if((*struct_types[i]==*_str_word) || (*struct_types[i]==*_str_int) || (*struct_types[i]==*_str_char)  ) {
				vd=NULL;
			}else {
#ifdef RSSO_DEBUG
				vd=(void*)new RSSObject(structs,struct_types[i]->data(),this->allocator_filename.data(),this->allocator_line);
#else
				vd=(void*)new RSSObject(structs,struct_types[i]->data());
#endif			
			}
			struct_values.at(i)=vd;
}

void RSSObject::free_by_id(int ident){
//	printf("REQ ::: => ");fflush(stdout);
//	printf("FREING %s %s ",
//		struct_types[ident]->data(),
//		struct_names[ident]->data()
//		);fflush(stdout);

	int i=ident;
	if(struct_values[i]){
			if(*struct_types[i]==*_str_dyn){ 
//				printf("MODE 1 ");fflush(stdout);
				delete (std::string*)struct_values[i]; 
			}
			else if(index(struct_types[i]->data(),'[')){ 
//				printf("MODE 2 ");fflush(stdout);
				std::vector<void*>* v = (std::vector<void*>*)struct_values[i];
//				printf(" COUNT %lu ",v->size());fflush(stdout);
						for(int y=0;y<v->size();y++){
							delete (RSSObject*)(*v)[y];
						}
						v->clear();
						delete v;
			}else if(  (*struct_types[i]==*_str_int)||(*struct_types[i]==*_str_word)||(*struct_types[i]==*_str_char) ){
//				printf("MODE 3 ");fflush(stdout);
				struct_values[i]=NULL;
			}else{
//				printf("MODE 4 ");fflush(stdout);
				delete (RSSObject*)struct_values[i];
			}
			struct_values[i]=NULL;
	}
	struct_values[i]=NULL;
//	printf(" OKAY\n");fflush(stdout);
}

void RSSObject::reinit_by_id(int ident){
	this->free_by_id(ident);
	this->allocate_by_id(ident);
}

bool RSSObject::clear(){
		for(int i=0;i<struct_values.size();i++){
			this->reinit_by_id(i);
		}
	return true;
}



#ifdef RSSO_DEBUG
#include <algorithm>
std::vector<RSSObject*> rsso;
#endif
	RSSObject::RSSObject(const char** structs,const char* structname
#ifdef RSSO_DEBUG
,const char* allocator_filename,const int allocator_line
#endif
		){
	bool tf=false;

#ifdef RSSO_DEBUG
this->allocator_filename=allocator_filename;
this->allocator_line=allocator_line;
rsso.push_back(this);
#endif

	this->structname=structname;

	_str_dyn=new std::string("dyn");
	_str_int=new std::string("int");
	_str_word=new std::string("word");
	_str_char=new std::string("char");

	this->structs=structs;
		for(int i=0;structs[i];i+=2){
			if(strcmp(structs[i],structname)==0){
				const char* text=structs[i+1];
				this->struct_description.clear();
				this->struct_description.append(text);
				int sl=strlen(text);
				sl++;
				int fromlen=0;
				for(int a=0;a<sl;a++){
					if(text[a]==' '){
						if((a-fromlen)<0){printf("syntax error 0x1\n");exit(-1);}
						struct_types.push_back(new std::string(&text[fromlen],a-fromlen));
						fromlen=a+1;
					}else if((text[a]==',')||(text[a]==0x00)){
						if((a-fromlen)<0){printf("syntax error 0x2\n");exit(-1);}
						struct_names.push_back(new std::string(&text[fromlen],a-fromlen));
						fromlen=a+1;
					}
				}
				tf=true;
				break;
			}
		}

		if(!tf){
			printf("Cant find type description with name %s\n",structname);exit(-1);
		}

		for(int i=0;i<struct_types.size();i++){
			struct_values.push_back(NULL);
		}

		for(int i=0;i<struct_types.size();i++){
			this->allocate_by_id(i);
		}

}


RSSObject::~RSSObject(){
		for(int i=0;i<struct_values.size();i++){
			this->free_by_id(i);
		}

		for(int i=0;i<struct_types.size();i++){
			delete struct_types[i]; 
		}

		for(int i=0;i<struct_names.size();i++){
			delete struct_names[i];
		}
 	delete _str_dyn;
 	delete _str_int;
 	delete _str_word;
 	delete _str_char;
#ifdef RSSO_DEBUG
	std::vector<RSSObject*>::iterator i= std::find(rsso.begin(),rsso.end(),this);
		if(i!=rsso.end()){
			rsso.erase(i);
		}else{
			printf("ERROR> RSS OBJECT NOT FOUND!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");fflush(stdout);
		}
#endif
	}

	bool RSSObject::compareType(RSSObject* withobj){
		return (*(withobj->getObjectStructDescription())==*this->getObjectStructDescription());
	}


	bool RSSObject::compare(RSSObject* withobj){
		if(!this->compareType(withobj)){return false;}
		std::string *s=new std::string();
		std::string *s2=new std::string();
		withobj->encode(s);
		this->encode(s2);
		bool ar=(*s==*s2);
		delete s;
		delete s2;
		return ar;
	}

	unsigned int RSSObject::getCrc(){
		std::string *s=new std::string();
		this->encode(s);
		unsigned int result=0;
		int length=s->length();
		result=length;
		unsigned char* d=(unsigned char*)s->data();
		for(int i=0;i<length;i++){
			result+=*d;
			d++;
		}
		result^=length;
		delete s;
		return result;
	}

	bool RSSObject::assign(RSSObject* fromobj){
	if(!this->compareType(fromobj)){
			printf("RSSObject(%s)::assign(). struct description not equal. \n",this->structname.data()); 
			printf("ME: \n");
			this->printDump("");
			printf("ASSIGN WITH:");
			fromobj->printDump("");
			fflush(stdout); exit(-1);
			return false;
		}
		this->clear();
		std::string *s=new std::string();
		fromobj->encode(s);
		int ar=this->parse(s->data(),s->length());
		delete s;
		return (ar!=-1);
	}

	std::string* RSSObject::getObjectStructDescription(){
		return &this->struct_description;
	}


	int RSSObject::getIdentOfObjectByName(const char* name){
		for(int i=0;i<struct_names.size();i++){
			if( struct_names[i]->compare(name)==0){
				return i;
			}
		}
		printf("RSSObject(%s)::objectByName(%s) not found\n",this->structname.data(),name); fflush(stdout); exit(-1);
		return -1;		
	}

	RSSObject* RSSObject::objectByName(const char* name){
		int idnt=this->getIdentOfObjectByName(name);
		std::string* type=struct_types[idnt];
		if((*type==*_str_int)||(*type==*_str_word)||(*type==*_str_char)){
			printf("RSSObject(%s)::objectByName(%s). getting object, but type of object = (%s) \n",this->structname.data(),name,type->data()); fflush(stdout); exit(-1);
			exit(-1); return NULL;
		}else{
			return (RSSObject*)struct_values[this->getIdentOfObjectByName(name)];
		}

	}

	RSSObject* RSSObject::operator[] (const char* name){
		return this->objectByName(name);
	}

	std::vector<RSSObject*>* RSSObject::arrayObjectByName(const char* name){
		return (std::vector<RSSObject*>*)this->objectByName(name);		
	}

	std::string* RSSObject::dynObjectByName(const char* name){
		return ((std::string*)this->objectByName(name));
	}

	unsigned long RSSObject::uintObjectByName(const char* name){
		void* v=struct_values[this->getIdentOfObjectByName(name)];
		return (unsigned long)v;		
	}

	bool RSSObject::setUintObjectByName(const char* name,unsigned long val){
		int idnt=this->getIdentOfObjectByName(name);
		std::string* type=struct_types[idnt];
		if((*type==*_str_int)||(*type==*_str_word)||(*type==*_str_char)){
			struct_values[idnt]=(void*)val;
		}else{
			printf("RSSObject(%s)::setUintObjectByName(%s). setting integer to object (%s) \n",this->structname.data(),name,type->data()); fflush(stdout); exit(-1);
			return false;
		}
		return true;
	}


bool RSSObject::encode(std::string * enc_to){
for(int i=0;i<struct_types.size();i++){
	if(*struct_types[i]==*_str_int){
		enc_to->append((char*)&struct_values[i],sizeof(int));
	}else if(*struct_types[i]==*_str_word){
		enc_to->append((char*)&struct_values[i],sizeof(short int));
	}else if(*struct_types[i]==*_str_char){
		enc_to->append((char*)&struct_values[i],sizeof(char));
	}else if(*struct_types[i]==*_str_dyn){
		dyn_t dt;
		std::string* s=((std::string*)struct_values[i]);
		dt.datalen=s->length();
		enc_to->append((char*)&dt,sizeof(dyn_t));
		enc_to->append(s->data(),s->length());
	}else if(index(struct_types[i]->data(),'[')){
		arr_t arr;
		std::vector<RSSObject*>* v=((std::vector<RSSObject*>*)struct_values[i]);
		arr.count=v->size();
		enc_to->append((char*)&arr,sizeof(arr_t));
		for(int q=0;q<v->size();q++){
			(*v)[q]->encode(enc_to);
		}
	}else{
		((RSSObject*)(struct_values[i]))->encode(enc_to);
	}
}
	return true;
}

RSSObject* RSSObject::copy(){
#ifdef RSSO_DEBUG
	RSSObject* o = new RSSObject(this->structs,structname.data(),this->allocator_filename.data(),this->allocator_line);
#else
	RSSObject* o = new RSSObject(this->structs,structname.data());
#endif			
	std::string * encoded=new std::string();
	if(!this->encode(encoded)){
		delete o;
		delete encoded;
		return NULL;
	}
	if(o->parse(encoded->data(),encoded->length())==-1){
		delete o;
		delete encoded;
		return NULL;
	}
	delete encoded;
	return o;
}


uint RSSObject::parse(std::string* bindata){
	if(!bindata){return -1;}
	return this->parse(bindata->data(),bindata->length());
}


uint RSSObject::parse(const char* data,uint len){
		uint skip=0;
		void* v;
		for(int i=0;i<struct_types.size();i++){
			if(*struct_types[i]==*_str_int){
				if(len<sizeof(int)){return -1;}
				long d=(long)*((int*)data);
				struct_values[i]=(void*)d;
				data+=sizeof(int);
				skip+=sizeof(int);
			}else if(*struct_types[i]==*_str_char){
				if(len<sizeof(char)){return -1;}
				long d=(long)*((unsigned char*)data);
				struct_values[i]=(void*)d;
				data+=sizeof(char);len-=sizeof(char);
				skip+=sizeof(char);
			}else if(*struct_types[i]==*_str_word){
				if(len<sizeof(short int)){return -1;}
				long d=(long)*((unsigned short int*)data);
				struct_values[i]=(void*)d;
				data+=sizeof(unsigned short int);len-=sizeof(unsigned short int);
				skip+=sizeof(unsigned short int);
			}else if(*struct_types[i]==*_str_dyn){
				if(len<sizeof(dyn_t)){return -1;}
				dyn_t* dt=(dyn_t*)data;
				len-=sizeof(dyn_t);data+=sizeof(dyn_t);
				if(len<dt->datalen){return -1;}
				std::string* s = (std::string*)struct_values[i];
				s->clear();
				s->append(data,dt->datalen);
				data+=dt->datalen;len-=dt->datalen;
				skip+=sizeof(dyn_t)+dt->datalen;
			}else if(index(struct_types[i]->data(),'[')){
				if(len<sizeof(arr_t)){return -1;}
				arr_t* arr=(arr_t*)data;
				len-=sizeof(arr_t);data+=sizeof(arr_t);
				skip+=sizeof(arr_t);
				std::vector<void*> *v = (std::vector<void*>*)struct_values[i];
				std::string* s=new std::string();
				s->append(struct_types[i]->data(),struct_types[i]->length()-2);
				for(int q=0;q<arr->count;q++){
					if (!len){delete s; return -1;}
					#ifdef RSSO_DEBUG
						RSSObject* rso = new RSSObject(this->structs,s->data(),this->allocator_filename.data(),this->allocator_line);
					#else
						RSSObject* rso = new RSSObject(this->structs,s->data());
					#endif			
					int ll=rso->parse(data,len);
					if(ll==-1){delete rso; delete s; return -1;}
					v->push_back(rso);
					len-=ll;data+=ll; skip+=ll;
				}
				delete s;
			}else{
					RSSObject* rso=(RSSObject*)struct_values[i];
					int ll=rso->parse(data,len);
					if(ll==-1){return -1;}
					len-=ll;data+=ll; skip+=ll;				
			}
		}
		return skip;
	}

	unsigned int RSSObject::getStructRows(){
		return this->struct_types.size();
	}
	std::string* RSSObject::getStructRowNameById(unsigned int ident){
		return this->struct_names[ident];
	}
	std::string* RSSObject::getStructRowTypeById(unsigned int ident){
		return this->struct_types[ident];
	}
	void* RSSObject::getStructRowValueById(unsigned int ident){
		return this->struct_values[ident];
	}

	void RSSObject::printDump(const char* withtext){
		std::string* ps=new std::string();
		ps->append("\t");
		ps->append(withtext);
		int sr=this->getStructRows();
		for(int i=0;i<sr;i++){
			std::string* type=this->getStructRowTypeById(i);
			printf("%s%s %s = ",ps->data(),type->data(),this->getStructRowNameById(i)->data());
			if(*type==*_str_int){
				printf("%lu",(unsigned long)this->getStructRowValueById(i) );
			}else if(*type==*_str_char){
				printf("%lu",(unsigned long)this->getStructRowValueById(i) );
			}else if(*type==*_str_word){
				printf("%lu",(unsigned long)this->getStructRowValueById(i) );
			}else if(*type==*_str_dyn){
				printf("DATA[%lu]",((std::string*)(this->getStructRowValueById(i)))->length() );
			}else if(index(type->data(),'[')){
				printf("Array {\n");
				std::string* ps2=new std::string();
				ps2->append("\t\t");
				ps2->append(withtext);
				std::vector<RSSObject*>* rso=(std::vector<RSSObject*>*)(this->getStructRowValueById(i));
				for(int q=0;q<rso->size();q++){
					printf("\t\t%s[%d] = {\n",withtext,q);
					(*rso)[q]->printDump(ps2->data());
					printf("\t\t%s}\n\n",withtext);
				}
				printf("%s }\n",ps->data());
				delete ps2;
			}else{
				RSSObject* rso=(RSSObject*)(this->getStructRowValueById(i));
				printf("Object {\n");
				std::string* ps2=new std::string();
				ps2->append("\t");
				ps2->append(withtext);
				rso->printDump(ps2->data());
				printf("%s }\n",ps->data());
				delete ps2;
			}
			printf("\n");
		}
	
	}

	const char* RSSObject::getStructName(){
		return this->structname.data();
	}


#ifdef RSSO_DEBUG
	void RSSObject_print_allocated(FILE * w){
		printf("ALLOCATED OBJECTS COUNT: %lu\n",rsso.size());
		for(int i=0;i<rsso.size();i++){
			RSSObject* t=rsso.at(i);
			fprintf(w,"OBJECT %s FILE %s LINE %d\n",t->getStructName(),t->allocator_filename.data(),t->allocator_line);
		}
		fflush(w);
	}
#endif