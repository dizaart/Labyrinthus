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


typedef struct{
	unsigned int domain_address_crc;
	std::string* sockaddr_for_response;
	int dns_req_unique;
	time_t last_request_time;
	std::string* all_request;
	RSSObject* address;
}dns_requests_t;

std::vector<dns_requests_t*> dns_requests;

void free_dns_requests_t(dns_requests_t* new_request){
		delete new_request->sockaddr_for_response;
		delete new_request->all_request;
		delete new_request->address;
		delete new_request;
}

void sendDNSResponse(dns_requests_t* req,in_addr *ip6addr){

	if(!req){return;}
	std::string *buffer=new std::string;
	buffer->append(req->all_request->data(),req->all_request->length());
	(*buffer)[2]=0x81;
	(*buffer)[3]=0x80;
	(*buffer)[6]=0;

	if(!ip6addr){
		(*buffer)[7]=0;
	}else{
		(*buffer)[7]=1;
		buffer->append("\xC0\x0c\x00\x01\00\x01",6);//AAAA. IN.
		uint ttl=3600;//TTL
		buffer->append((char*)&ttl,4);
		buffer->append("\x00\x04",2);//data length 16
		buffer->append((char*)&ip6addr->s_addr,4);
	}
	sendto(dns_socket,buffer->data(),buffer->length(),0,(sockaddr*)req->sockaddr_for_response->data(),req->sockaddr_for_response->length());
	delete buffer;
}


void onDNSRequest(std::string *address,dns_type_t type,int request_unique,std::string* sockaddr_string,std::string * all_request){
	const char* type_desc[]={"DNS_TYPE_UNDEFINED","DNS_TYPE_A","DNS_TYPE_AAAA","DNS_TYPE_ANY"};
	printf("DNS Request: ADDRESS %s TYPE %s UNQ:%x\n",address->data(),type_desc[type],request_unique);
	uint domain_crc=get_crc(address->data(),address->length());;
	BindedChain* bc;
	if(type==DNS_TYPE_A || type==DNS_TYPE_ANY){
		dns_requests_t* new_request=new dns_requests_t;
		
		new_request->domain_address_crc=domain_crc;
		new_request->sockaddr_for_response=new std::string();
		new_request->sockaddr_for_response->append(sockaddr_string->data(),sockaddr_string->length());
		new_request->all_request=new std::string();
		new_request->all_request->append(all_request->data(),all_request->length());
		new_request->dns_req_unique=request_unique;
		new_request->last_request_time=time(NULL);
		new_request->address=new RSSObject(getDefinedStructs(),"address_t");
		new_request->address->setUintObjectByName("type",ADDR_TYPE_DOMAIN);
		new_request->address->dynObjectByName("address")->append(address->data(),address->length());
		if((bc=getBindedChainByAddress (new_request->address))) {
				sendDNSResponse(new_request,&bc->local_addr);
				free_dns_requests_t(new_request);
		}else{
			dns_requests.push_back(new_request);
		}
	}
}


void* dns_thread_function(void* arg){
	
	dns_socket=create_udp_socket(NULL,53);
	if(dns_socket==-1){
		perror("Can't create DNS Listen Socket");
		exit(0);
	}
	char buffer[4096];
	char sockaddr_buff[1024];
	socklen_t slen;
	typedef struct{
		char tr_id[2];
		char flags[2];
		char questions[2];
		char unswer_rr[2];
		char auth_rr[2];
		char add_rr[2];
	}__attribute__ ((packed))dns_request_t;//+ while  char (req_len),req. and null_end. . char[2] type, char[2] class
	//class. 00 01 . type 00 ff = any. 00 01 - ipv4. 00 1c - ipv6

	typedef struct{
		unsigned char type[2];
		unsigned char rclass[2];
	}__attribute__ ((packed))dns_request_additional_t;

	while(true){
		slen=1024;
		int rl=recvfrom(dns_socket,&buffer[0],4096,0,(sockaddr*)&sockaddr_buff[0],&slen);
		if(rl==-1){perror("DNS Service destroyed. recvfrom return -1\n");break;}
		if(rl>sizeof(dns_request_t)){
			std::string *address=new std::string();
			dns_request_t* a=(dns_request_t*)buffer;

			char* d=buffer+sizeof(dns_request_t);
			int max=rl-sizeof(dns_request_t);
			while(true){
				if(max<1){break;}
				unsigned char nl=*(unsigned char*)d;
				max-=1;
				d+=1;
				if(!nl){break;}
				if(max-nl<0){break;}
				if(address->length()){address->append(".");}
				address->append(d,nl);
				max-=nl;
				d+=nl;
			}

			if(address->length()&&(max>=sizeof(dns_request_additional_t))){
				dns_request_additional_t* di=(dns_request_additional_t*)d;
				dns_type_t dt=DNS_TYPE_UNDEFINED;
				if((di->type[0]==0x00)&&(di->type[1]==0x01)){
					dt=DNS_TYPE_A;
				}else if((di->type[0]==0x00)&&(di->type[1]==0xff)){
					dt=DNS_TYPE_ANY;
				}else if((di->type[0]==0x00)&&(di->type[1]==0x1c)){
					dt=DNS_TYPE_AAAA;
				} 

				int request_unique;
				memcpy(&request_unique,&a->tr_id,sizeof(a->tr_id));
				std::string* sockaddr_string = new std::string();
				sockaddr_string->append(sockaddr_buff,slen);
				std::string *all_request=new std::string();
				all_request->append(buffer,rl);
				onDNSRequest(address,dt,request_unique,sockaddr_string,all_request);
				delete sockaddr_string;
				delete all_request;
			}
			delete address;
		}
	}
	close_udp_socket(dns_socket);
return NULL;
}



