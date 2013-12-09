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

RSSObject* my_address;

std::vector<ClientObject*> registred_clients;
std::vector<RSSObject*> devices;
std::vector<BindedChain*> binded_chains; //ipv6 to chain


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
		uint domain_crc=get_crc(address->data(),address->length());
		for(int i=0;i<binded_chains.size();i++){
			if(binded_chains[i]->domain_address_crc==domain_crc){
				if(binded_chains[i]->getChainsResponse->objectByName("address")->compare(addr)){
					return binded_chains[i];
					break;
				}
			}
		}
		RSSObject* getChainsRequest=new RSSObject(getDefinedStructs(),"getChainsRequest");
		getChainsRequest->objectByName("address")->assign(addr);
		rsock->sendToRouteServer(RS_CMD_GETCHAINS_REQUEST,getChainsRequest);
		delete getChainsRequest;
		return NULL;
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
	}
	
	delete rsock;
}
