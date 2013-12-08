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

#include "inc.h"
#include "udpsocket.h"



int create_udp_socket(const char* localhost,int port){

 int udp_fd;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   udp_fd=socket(AF_INET,SOCK_DGRAM,0);
if(udp_fd==-1){return -1;}
   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=localhost?inet_addr(localhost): htonl(INADDR_ANY);
   servaddr.sin_port=htons(port);
   int br=bind(udp_fd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if(br){close(udp_fd);return -1;}
	return udp_fd;
}

bool close_udp_socket(int sock){
if(sock==-1){return true;}
	return (close(sock)==0);
}
