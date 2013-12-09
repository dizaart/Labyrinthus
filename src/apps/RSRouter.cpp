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
