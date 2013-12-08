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

#ifndef __DNS_DEFINES
#define __DNS_DEFINES
//RS = ROUTESERVER
#include <string>
#include "rssobject.h"
#define RS_COMMUNICATION_PORT 7778 

const char** getDefinedStructs();

#define USERS_SUBNET_MAJOR 192
#define USERS_SUBNET_MINOR 254
#define CHAINS_SUBNET_MAJOR 192

#define DESTINATION_TRAFFIC_KEY_SIZE 256
#define DEVICE_TRAFFIC_PUBLICKEY_SIZE 256
#define DEVICE_ADDRESS_PUBLICKEY_SIZE 256
#define DOMAIN_CHAINS_SIGN_SIZE 256 //WHIRLPOOL

#define ADDR_LOCALHASH_SIZE 16
//#define RS_HOST "10.0.1.11"

#define RS_HOST "199.0.0.1"


#include "inc.h"

//address. can be ADDR_TYPE_IP4, ADDR_TYPE_IP6,ADDR_TYPE_DOMAIN
enum addr_type_t{ADDR_TYPE_UNDEFINED,ADDR_TYPE_IP4,ADDR_TYPE_IP6,ADDR_TYPE_DOMAIN,ADDR_TYPE_LOCALHASH};
enum rs_cmd_t {RS_CMD_UNDEFINED,RS_CMD_GETCHAINS_REQUEST,RS_CMD_GETCHAINS_RESPONSE,RS_CMD_SETCHAINS_REQUEST,RS_CMD_SETCHAINS_RESPONSE,
	RS_CMD_ONDEVICECONNECT_REQUEST,RS_CMD_ONDEVICECONNECT_RESPONSE,RS_CMD_GETDEVICES_REQUEST,RS_CMD_GETDEVICES_RESPONSE,RS_CMD_SEND_PACKET};
enum encryption_method_t {RS_ENCRYPTION_UNDEFINED,RS_ENCRYPTION_NOTENCRYPT,RS_ENCRYPTION_BLOWFISH};
enum dns_type_t{DNS_TYPE_UNDEFINED,DNS_TYPE_A,DNS_TYPE_AAAA,DNS_TYPE_ANY};


RSSObject* createRSSObjectWithType(const char* type_name);

#define RS_ONDEVICECONNECT_RESPONSE_OK 1
#define RS_ONDEVICECONNECT_RESPONSE_ERR 2

#define RS_SETCHAINS_RESPONSE_OK 1
#define RS_SETCHAINS_RESPONSE_ERR 2

#define RS_GETCHAINS_RESPONSE_OK 1
#define RS_GETCHAINS_RESPONSE_ERR 2

#define create_rs_socket create_udp_socket 
#define create_dev_socket create_udp_socket 

#define close_rs_socket close_udp_socket 
#define close_dev_socket close_udp_socket 

void print_ip4(in_addr ip);

void randomize();

#endif
