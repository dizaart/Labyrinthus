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

const char* structs[]={
"address_t","char type,dyn address",
"getChainsRequest","address_t address",
"getChainsResponse","char status,dyn traffic_key,char encryption_method,address_t address,chain_row chain",
"setChainsRequest","address_t address,dyn traffic_key,char encryption_method,chains_t chains,dyn signed_chains_data", //sign need for domains not in <.hashdomain.lab>
"setChainsResponse","char status,address_t address",
"onDeviceConnectRequest","int transmission_rate",
"onDeviceConnectResponse","char status,address_t you_address",
"getDevicesRequest","word min_transmission_rate,word max_transmission_rate,int max_count",
"getDevicesResponse","int device_count,address_t[] devices",
"packet_t","char cmd,dyn data",
"getPublicKeysOfDevice","dyn traffic_key,dyn address_key",
"chains_t","chain_row[] chain",
"chain_row","address_t address,encrypted_packet_t matreshka",
"encrypted_packet_t","char encryption_method,dyn data",
"data_packet_t","chain_row chain,dyn encrypted_data",
"data_packet_encrypted_data","address_t return_address,dyn data,char ip_protocol",   //data = DATA WITH IP HEADER. BUT IP ADDRESSES IS NULLED.
NULL,NULL
};

const char** getDefinedStructs(){
return (const char**)structs;

}

RSSObject* createRSSObjectWithType(const char* name){
	return new RSSObject((const char**)&structs,name);
}


void randomize(){
	struct timeval tv;
	struct timezone tz;
	memset(&tz,0x00,sizeof(tz));
	gettimeofday(&tv,&tz);
	srand(tv.tv_usec);
}


