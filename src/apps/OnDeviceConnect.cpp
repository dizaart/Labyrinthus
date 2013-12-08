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

#include "rs_defines.h"
#include <stdio.h>
#include "udpsocket.h"
#include "rs_socket.h"
#include "inc.h"
#include "rssobject.h"
#include <pthread.h>
#include "get_crc.h"
#include <limits.h>
#include "tun.h"


bool apply_and_route_rss_packet(RSSObject* sendObject,bool decode);
int dns_socket;
int tun_in;
int tun_out;
RSSUDPSocket* rsock;
pthread_t dns_thread;
pthread_t tunnel_thread;
pthread_t device_thread;
unsigned char binded_ip[4]={0};

int max_clients=250;

enum{CLIENT_STATE_UNDEFINED,CLIENT_STATE_UNUSED,CLIENT_STATE_USED,CLIENT_STATE_WAIT_RESPONSE,CLIENT_STATE_REGISTRED};

typedef struct{
	std::string * local_unique_identifier; 
	std::string * local_domain;
	RSSObject* chains;
//	in_addr local_addr. ident= 10.0.0.ident
	in_addr local_addr;
	std::string * traffic_key;
	int state;
	time_t lastActivityTime; //if in waiting_for_register, then last register request. if in registred_clients then last activity (outgoing)
}ClientObject;

//update chains for domain every y minutes. if domain not updated - server remove this domain. (domain offline)
//notify about domain will be send over chain

typedef struct{
	in_addr local_addr;
	unsigned int domain_address_crc;
	RSSObject* getChainsResponse;
}BindedChain;

typedef struct{
	unsigned int domain_address_crc;
	std::string* sockaddr_for_response;
	int dns_req_unique;
	time_t last_request_time;
	std::string* all_request;
	RSSObject* address;
}dns_requests_t;

RSSObject* my_address;

std::vector<ClientObject*> registred_clients;
std::vector<RSSObject*> devices;
std::vector<BindedChain*> binded_chains; //ipv6 to chain
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


         unsigned short ip_sum(unsigned short* ip, int len){
           long sum = 0;  /* assume 32 bit long, 16 bit short */

           while(len > 1){
             sum += *ip; ip++;
             if(sum & 0x80000000)   /* if high order bit set, fold */
               sum = (sum & 0xFFFF) + (sum >> 16);
             len -= 2;
           }

           if(len) /* take care of left over byte */
             sum += (unsigned short) *(unsigned char *)ip;
          
           while(sum>>16)
             sum = (sum & 0xFFFF) + (sum >> 16);

           return ~sum;
         }



void inc_binded(){
for(int i=3;i>-1;i--){
		if(binded_ip[i]!=255){
			binded_ip[i]++;break;
		}else{
			binded_ip[i]=0;
		}
	}
}

void receiver(){
	while(rsock->receivePacket()){
	RSSObject* o=NULL;
	rs_cmd_t cmd_id=rsock->getLastCmd();
		if(rsock->isLastPackedFromRouteServer()){
			if((cmd_id==RS_CMD_GETCHAINS_RESPONSE)&&(o=rsock->getLastPackedDataAsType("getChainsResponse"))){
				printf("getChainsResponse\n");
				RSSObject* addr_o=o->objectByName("address");
				std::string* addr=addr_o->dynObjectByName("address");
				uint addr_crc=get_crc(addr->data(),addr->length());
		//		printf("ADDRESSSS %s LEN %lu\n",addr->data(),addr->length());

				BindedChain* bc=NULL;
				
				if(o->uintObjectByName("status")==RS_GETCHAINS_RESPONSE_OK){
						inc_binded();
						printf("RS_GETCHAINS_RESPONSE_OK. RA=");print_ip4(*(in_addr*)&binded_ip);printf(" \n");
						bc=new BindedChain;
						bc->getChainsResponse=new RSSObject(getDefinedStructs(),"getChainsResponse");
						memcpy(&bc->local_addr,&binded_ip[0],4);
						bc->domain_address_crc=addr_crc;
						bc->getChainsResponse->assign(o);
						binded_chains.push_back(bc);
				}

				for(int i=0;i<dns_requests.size();i++){
					dns_requests_t* rt=dns_requests[i];
					if(rt->domain_address_crc==addr_crc){
						if(rt->address->compare(addr_o)){
							if(bc && o->uintObjectByName("status")==RS_GETCHAINS_RESPONSE_OK){
								sendDNSResponse(rt,&bc->local_addr);
							}else{
								sendDNSResponse(rt,NULL);
							}
							free_dns_requests_t(rt);
							dns_requests.erase(dns_requests.begin()+i);
							break;
						}
					}
				}
			}else if((cmd_id==RS_CMD_SETCHAINS_RESPONSE)&&(o=rsock->getLastPackedDataAsType("setChainsResponse"))){
				printf("setChainsResponse\n");
				if(o->objectByName("address")->uintObjectByName("type")==ADDR_TYPE_DOMAIN){
					std::string *sstr=o->objectByName("address")->dynObjectByName("address");
				for(int i=0;i<registred_clients.size();i++){
					if(*(registred_clients.at(i)->local_domain)==*sstr){
						registred_clients.at(i)->state=CLIENT_STATE_REGISTRED;
						break;
						}
					}
				}
//				printf("REGISRED %lu\nWAITED NONE\n",registred_clients.size());
			}else if((cmd_id==RS_CMD_ONDEVICECONNECT_RESPONSE)&&(o=rsock->getLastPackedDataAsType("onDeviceConnectResponse"))){
				printf("onDeviceConnectResponse\n");				
				if(o->uintObjectByName("status")==RS_ONDEVICECONNECT_RESPONSE_OK){
					printf("RS Register Success\n");
					if(my_address){delete my_address;}
					my_address=o->objectByName("you_address")->copy();
					RSSObject* os = new RSSObject(getDefinedStructs(),"getDevicesRequest");
					os->setUintObjectByName("max_count",10);
					rsock->sendToRouteServer(RS_CMD_GETDEVICES_REQUEST,os);
				}else{
					printf("RS Register Error\n");
				}
			}else if((cmd_id==RS_CMD_GETDEVICES_RESPONSE)&&(o=rsock->getLastPackedDataAsType("getDevicesResponse"))){
				printf("getDevicesResponse\n");
		
				//clearing old device list
				for(int i=0;i<devices.size();i++){
					delete devices[i];
				}
				devices.clear();
				//adding new device list
				std::vector<RSSObject*> * rd;
				rd=o->arrayObjectByName("devices");
				if(rd){
					for(int i=0;i<rd->size();i++){
						RSSObject* no=(*rd)[i]->copy();
						if(no){
							devices.push_back(no);
						}
					}
				}


				//checking clients to register
				//check_and_register_clients();
			} 
		}else{ //end of from RouteServer. 
			   //then from devices
				if((cmd_id==RS_CMD_SEND_PACKET)&&(o=rsock->getLastPackedDataAsType("data_packet_t"))){
						//o->printDump("");
						apply_and_route_rss_packet(o,true);
				}


		}//end of from devices
		done:
		if(o){
//			printf("DUMP Object\n");
//			o->printDump("");
			delete o;
		}
	}//while receive packet
}


void onDeviceConnect(){
	RSSObject* o = new RSSObject(getDefinedStructs(),"onDeviceConnectRequest");
	o->setUintObjectByName("transmission_rate",0xAAEBDA);
	rsock->sendToRouteServer(RS_CMD_ONDEVICECONNECT_REQUEST,o);
	delete o;
}


uint getRandomChar(){
	return rand();
}

void encrypt_data(RSSObject* chain,std::string *data,int encryption_alghoritm){
	if(!chain){return;}
	chain->setUintObjectByName("encryption_method",encryption_alghoritm);
	if(data){
		chain->dynObjectByName("data")->clear();
		chain->dynObjectByName("data")->append(data->data(),data->length());
	}
}

RSSObject* encrypt_chain_element(RSSObject* addr, RSSObject* old_chain_row,int encryption_alghoritm  ){
	RSSObject* r=new RSSObject(getDefinedStructs(),"chain_row");
	if(old_chain_row){
		std::string *cr=new std::string();
		old_chain_row->encode(cr);
		encrypt_data(r->objectByName("matreshka"),cr,encryption_alghoritm);
		delete cr;
	}else{
		encrypt_data(r->objectByName("matreshka"),NULL,encryption_alghoritm);
	}
	r->objectByName("address")->assign(addr);
	return r;
}

bool make_and_append_chain(ClientObject* client){
std::vector<RSSObject*> route;
RSSObject* client_obj=new RSSObject(getDefinedStructs(),"address_t");
client_obj->setUintObjectByName("type",ADDR_TYPE_LOCALHASH);
client_obj->dynObjectByName("address")->append(client->local_unique_identifier->data(),client->local_unique_identifier->length());

route.push_back(client_obj);
route.push_back(my_address);

for(int i=0;i<2;i++){
	RSSObject* random_address=devices[rand()%(devices.size()-1)];
	route.push_back(random_address);
}

//printf
printf("CHAIN: ");
for(int i=route.size()-1;i>-1;i--){
printf("%s",(i==route.size()-1)?"":" => "); 
if(i==0){
	print_ip4( client->local_addr );
}else{
	print_ip4( *(in_addr*)(route[i]->dynObjectByName("address")->data()  ) );	
}
}
printf("\n");
//end printf

RSSObject* o=NULL;
RSSObject* o2=NULL;

for(int i=0;i<route.size();i++){
	encryption_method_t em;
	if( (!o)||(i==(route.size()-1))){ //first and last dont encrypt (first - on device. crypted by second. )
		em=RS_ENCRYPTION_NOTENCRYPT;
	}else{
		em=RS_ENCRYPTION_BLOWFISH;
	}
o2 = encrypt_chain_element(route[i],o,em);
if(o){delete o;}
o=o2;
}
delete client_obj;
client->chains->arrayObjectByName("chain")->push_back(o2);
	return true;
}


ClientObject* allocateClientObject(){
	ClientObject* co=new ClientObject();
	if(!co){return NULL;}
	co->local_unique_identifier=new std::string();
	co->traffic_key=new std::string();
	co->local_domain=new std::string();
	co->chains=new RSSObject(getDefinedStructs(),"chains_t");
	co->local_addr.s_addr=inet_addr("0.0.0.1");
	co->lastActivityTime=0;
	co->state=CLIENT_STATE_UNUSED;
	((unsigned char*)&co->local_addr.s_addr)[0]=USERS_SUBNET_MAJOR;
	((unsigned char*)&co->local_addr.s_addr)[1]=USERS_SUBNET_MINOR;
	return co;
}



void freeClientObject(ClientObject* co){
	if(!co){return;}
	delete co->local_unique_identifier;
	delete co->local_domain;
	delete co->chains;
	delete co->traffic_key;
	delete co;
}


ClientObject* getClientInfoById(unsigned int ip_ident){
	if(ip_ident>=registred_clients.size()){return NULL;}
	ClientObject* co = registred_clients[ip_ident];
	if(co->state==CLIENT_STATE_REGISTRED){
		co->lastActivityTime=time(NULL);
		return registred_clients[ip_ident];
	}
	if(co->state==CLIENT_STATE_USED){return NULL;}
	if(co->state==CLIENT_STATE_WAIT_RESPONSE){return NULL;}

	if(co->state==CLIENT_STATE_UNUSED){
		if(!devices.size()){return NULL;}
		co->state=CLIENT_STATE_WAIT_RESPONSE;

		randomize();
		int it=18+(getRandomChar()%16);
		char cd[128];
		sprintf(cd,"%x.ll",rand()%255);
		co->local_domain->append(cd);
		for(int i=0;i<it;i++){
			unsigned char c=getRandomChar();
			co->local_unique_identifier->append((char*)&c,1);
		}
		//generating chains
		printf("=================== \nNEW USER %s LOCAL IP: ",cd); print_ip4(co->local_addr); printf("\n");
		for(int i=0;i<5;i++){
			make_and_append_chain(co);
		}
		printf("=================== \n");
		
		RSSObject* setChainRequest=new RSSObject(getDefinedStructs(),"setChainsRequest");
		setChainRequest->objectByName("address")->setUintObjectByName("type",ADDR_TYPE_DOMAIN);
		setChainRequest->setUintObjectByName("encryption_method",RS_ENCRYPTION_BLOWFISH);
		setChainRequest->objectByName("address")->dynObjectByName("address")->append(co->local_domain->data(),co->local_domain->length());
		setChainRequest->dynObjectByName("traffic_key")->append(co->traffic_key->data(),co->traffic_key->length());
		setChainRequest->objectByName("chains")->assign(co->chains);
		rsock->sendToRouteServer(RS_CMD_SETCHAINS_REQUEST,setChainRequest);	
		delete setChainRequest;

	return NULL;
	}	
	return NULL;
}



BindedChain* getBindedChainByAddress(RSSObject* addr){
	std::string* address=addr->dynObjectByName("address");
//	printf("BINDED CHAIN BY ADDRESS %s ",address->data());
		uint domain_crc=get_crc(address->data(),address->length());
		for(int i=0;i<binded_chains.size();i++){
			if(binded_chains[i]->domain_address_crc==domain_crc){
				if(binded_chains[i]->getChainsResponse->objectByName("address")->compare(addr)){
//					printf("OKAY\n");
					return binded_chains[i];
					break;
				}
			}
		}
//		printf("REQUESTING\n");
		RSSObject* getChainsRequest=new RSSObject(getDefinedStructs(),"getChainsRequest");
		getChainsRequest->objectByName("address")->assign(addr);
		rsock->sendToRouteServer(RS_CMD_GETCHAINS_REQUEST,getChainsRequest);
		delete getChainsRequest;
		return NULL;
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

bool encrypt_data(std::string* data,std::string* public_key){
	return true;
}

bool decrypt_data(std::string* data,std::string* private_key){
	return true;
}


bool apply_and_route_rss_packet(RSSObject* sendObject,bool decode){  // object = data_packet_t . . not decode. decoding make in recv function.
bool r=false;
if(!sendObject){return false;}
RSSObject* chain_from_param=sendObject->objectByName("chain");
RSSObject* chain=chain_from_param->copy();

std::string* encrypted_chains=chain->objectByName("matreshka")->dynObjectByName("data"); //and encryption method exists
std::string* encrypted_data=sendObject->dynObjectByName("encrypted_data");
if(decrypt_data(encrypted_chains,NULL)){
	RSSObject* chain_addr=chain->objectByName("address");
	uint type_of_address=chain_addr->uintObjectByName("type");
	if(type_of_address==ADDR_TYPE_IP4){
		if(chain_from_param->parse(encrypted_chains)!=-1){
				unsigned char * s=(unsigned char* )chain_addr->dynObjectByName("address")->data();
				r=rsock->sendToIPAddress((struct in_addr*)chain_addr->dynObjectByName("address")->data(),RS_CMD_SEND_PACKET,sendObject);
		}
	}else if(type_of_address==ADDR_TYPE_LOCALHASH){
		std::string* localhash=chain_addr->dynObjectByName("address");
		ClientObject* co;
		for(int i=0;i<registred_clients.size();i++){
			if((co=registred_clients[i]) && (*co->local_unique_identifier==*localhash)){
				RSSObject* decode_me=new RSSObject(getDefinedStructs(),"data_packet_encrypted_data");
				if(decrypt_data(encrypted_data,NULL)){//my pubkey
					if(decode_me->parse(encrypted_data)!=-1){
						std::string* data=decode_me->dynObjectByName("data");
						std::string *iface_data=new std::string();
						ip4_t ip;
						memset(&ip,0x00,sizeof(ip4_t));

						RSSObject* return_addr=decode_me->objectByName("return_address");
						BindedChain* bc;
						if((bc=getBindedChainByAddress (return_addr) )) {
							ip.ip_v=4;
							ip.ip_hl=5;
							ip.ip_tos=0;
							ip.ip_len=htons(data->length()+sizeof(ip4_t));
							ip.ip_id=0;
							ip.ip_off=0x0040;
							ip.ip_ttl=64;
							ip.ip_p=(u_int8_t)decode_me->uintObjectByName("ip_protocol");
							ip.ip_sum=0;
							ip.ip_src=bc->local_addr;
							ip.ip_dst=co->local_addr;
							ip.ip_sum=ip_sum((unsigned short*)&ip,sizeof(ip4_t));
							iface_data->append("\x00\x00\x08\x00",4);
							iface_data->append((const char*)&ip,sizeof(ip4_t));
							iface_data->append(data->data(),data->length());
							r=(write(tun_in, iface_data->data(),iface_data->length() )!=-1);
						}else{
						}
						delete iface_data;
					}else{
						printf("DECODE ME ERROR\n");
					}
				}
				delete decode_me;
				break;
			}
		}
	}
}else{
	r=false;;
}
delete chain;
return r;
}

void* tunnel_thread_function(void* p){
	char tun_name[IFNAMSIZ];
  	fd_set fds_r;
  	fd_set fds_e;
   ip4_t* ip_head;

  /* Connect to the device */
  tun_in=tun_alloc("lab0", IFF_TUN/*|IFF_NO_PI*/ );  /* tun interface */

  int nread;
  unsigned char *buffer=(unsigned char*)malloc(65540);

  if((tun_in < 0)){
    perror("Allocating interface");
    exit(1);
  }

  int fd_max_p1=(tun_in>tun_out)?(tun_in+1):(tun_out+1);
  struct timeval tv;
	      while(true){
	      nread = read(tun_in,buffer,65540);
 		  if(nread>=4){
//			printf("FLAGS %02x %02x PROTO %02x %02x\n",buffer[0],buffer[1],buffer[2],buffer[3]);
				if(buffer[2]==0x08 && buffer[3]==0x00 && buffer[4]==0x45 && (nread-4>sizeof(ip4_t))){  //ipV4	
					//going to internet. generating chains. binding to ipv4Chains, etc.
					ip_head=(ip4_t*)(buffer+4);
					
					if(  ( ((const unsigned char*)&ip_head->ip_src.s_addr) [0]==USERS_SUBNET_MAJOR)&&( ((const unsigned char*)&ip_head->ip_src.s_addr) [1]==USERS_SUBNET_MINOR)            ){
					if(  (((const unsigned char*)&ip_head->ip_dst.s_addr)[0]==CHAINS_SUBNET_MAJOR)&&( nread-4>= ntohs(ip_head->ip_len))){
						((unsigned char*)&ip_head->ip_src.s_addr)[0]=((unsigned char*)&ip_head->ip_src.s_addr)[1]=((unsigned  char*)&ip_head->ip_dst.s_addr)[0]=0;

						(( char*)&ip_head->ip_src.s_addr)[3]--;
						(( char*)&ip_head->ip_dst.s_addr)[3]--;

						unsigned int chain_id= ntohl( *(unsigned int*)&ip_head->ip_dst.s_addr );
						unsigned int client_id= ntohl( *(unsigned int*)&ip_head->ip_src.s_addr );

						ip_head->ip_dst.s_addr=ip_head->ip_src.s_addr=0;

						ClientObject* ca=NULL;
//						printf("CHAINS: %lu CHAINREQ: %d CLIENTS %d CLIENTID %d\n",binded_chains.size(),chain_id,-1,client_id); fflush(stdout);
						if(  (ca=getClientInfoById(client_id)) && (chain_id<binded_chains.size()) ){

							RSSObject* sendObject=new RSSObject(getDefinedStructs(),"data_packet_t");
							RSSObject* encdata=new RSSObject(getDefinedStructs(),"data_packet_encrypted_data");
							RSSObject* ra=encdata->objectByName("return_address");
							std::string* li=ca->local_domain;
							ra->dynObjectByName("address")->append(li->data(),li->length());
							ra->setUintObjectByName("type",ADDR_TYPE_DOMAIN);
							unsigned char ip_proto=*(unsigned char*)&ip_head->ip_p;
							encdata->setUintObjectByName("ip_protocol",ip_proto);
							encdata->dynObjectByName("data")->append((const char*)(buffer+4+sizeof(ip4_t)),(size_t)ntohs(ip_head->ip_len)-sizeof(ip4_t) );

							std::string *ss=new std::string;
							encdata->encode(ss);
							delete encdata;
							sendObject->dynObjectByName("encrypted_data")->append(ss->data(),ss->length());
							delete ss;
							encrypt_data(sendObject->dynObjectByName("encrypted_data"),NULL /* DESTINATION KEY */ );
							sendObject->objectByName("chain")->assign(binded_chains.at(chain_id)->getChainsResponse->objectByName("chain"));
							apply_and_route_rss_packet(sendObject,false);
							delete sendObject;
						}
					}
					}

				}else if(buffer[2]==0x86 && buffer[3]==0xdd && buffer[4]==0x60 && (nread-4>sizeof(ip6_t)) ){  //ipV6

				}
			}
		}
	free(buffer);
}



void* device_thread_function(void* p){
	onDeviceConnect();
	receiver();
	return NULL;
}



int main(int argc,char** argv){
	memset(binded_ip,0x00,4);
	binded_ip[0]=CHAINS_SUBNET_MAJOR;
	for(int i=0;i<max_clients;i++){
		ClientObject* co=allocateClientObject();
		((unsigned char*)&co->local_addr.s_addr)[3]=i+1;
		registred_clients.push_back(co);
	}

	rsock = new RSSUDPSocket(NULL);
	pthread_create(&dns_thread,NULL,&dns_thread_function,NULL);
	pthread_create(&device_thread,NULL,&device_thread_function,NULL);
	pthread_create(&tunnel_thread,NULL,&tunnel_thread_function,NULL);

	while(true){
		sleep(10);
		FILE * f;
		if((f=fopen("ODC.exit","r"))){
			fclose(f);
			unlink("ODC.exit");
			exit(0);
		}
	}
	
	delete rsock;
}
