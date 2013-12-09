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
#include "inc.h"
#include "rssobject.h"
#include <string>

class RSSUDPSocket{
private:
	char send_sin_tmp[1024];
	char sin_tmp[1024];
	socklen_t slen_tmp;
	in_addr server_inaddr;
	int udp_socket;
	uint buff_maxlen;
    char* buffer;
    RSSObject* rso_data;
public:
	RSSUDPSocket(const char* localaddr);
	virtual ~RSSUDPSocket();
	bool sendToSockAddr(struct sockaddr* sin,socklen_t slen, rs_cmd_t cmd,RSSObject* obj);
	bool sendToRouteServer(rs_cmd_t cmd,RSSObject* obj);
	bool sendToIPAddress(struct in_addr* addr,rs_cmd_t cmd, RSSObject* obj);
	bool sendToIPAddress(struct in6_addr* addr,rs_cmd_t cmd, RSSObject* obj);
	bool sendToLastAddress(rs_cmd_t cmd,RSSObject* obj);
	bool receivePacket(); 
	bool isLastPackedFromRouteServer();
	bool isLastPacketIPv6();
	bool isLastPacketIPv4();
	rs_cmd_t getLastCmd();
	struct sockaddr_in* getLastPacketSockAddrIn();
	struct sockaddr_in6* getLastPacketSockAddrIn6();
	std::string* getLastPacketData();
	RSSObject* getLastPackedDataAsType(const char* type_name); //return null (if unpack error) or object.
};

