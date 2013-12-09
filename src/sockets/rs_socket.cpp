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

#include "rs_socket.h"
#include "udpsocket.h"

RSSUDPSocket::RSSUDPSocket(const char* localaddr){
   this->udp_socket=create_udp_socket(localaddr,RS_COMMUNICATION_PORT);
   if(this->udp_socket==-1){perror("Cant create socket");exit(-1);}
   this->server_inaddr.s_addr=inet_addr(RS_HOST);
   this->buff_maxlen=65535;
   this->buffer=(char*)malloc(this->buff_maxlen);
   this->rso_data=NULL;
}

RSSUDPSocket::~RSSUDPSocket(){
   if(buffer){free(buffer);}
   close_udp_socket(this->udp_socket);
   if(rso_data){delete rso_data;}
}

bool RSSUDPSocket::sendToRouteServer(rs_cmd_t cmd,RSSObject* obj){
   sockaddr_in* sia=(sockaddr_in*)send_sin_tmp;
   memset(sia,0x00,sizeof(sockaddr_in));
   sia->sin_addr=this->server_inaddr;
   sia->sin_family=AF_INET;
   sia->sin_port=htons(RS_COMMUNICATION_PORT);
   return this->sendToSockAddr((sockaddr*)sia,sizeof(sockaddr_in),cmd,obj);
}

bool RSSUDPSocket::sendToSockAddr(struct sockaddr* sin,socklen_t slen, rs_cmd_t cmd,RSSObject* obj){
   if(udp_socket==-1){return false;}
   std::string * sbuffer=new std::string();
   RSSObject* cmd_obj=new RSSObject(getDefinedStructs(),"packet_t");
   cmd_obj->setUintObjectByName("cmd",cmd);
   obj->encode(cmd_obj->dynObjectByName("data"));
   cmd_obj->encode(sbuffer);
   sendto(udp_socket,sbuffer->data(),sbuffer->length(),MSG_DONTWAIT,sin,slen);
   delete cmd_obj;
   delete sbuffer;
   return true;
}

 bool RSSUDPSocket::sendToLastAddress(rs_cmd_t cmd,RSSObject* obj){
   sockaddr_in* sa;
   sockaddr_in6* sa6;
   if((sa=this->getLastPacketSockAddrIn())){
      in_addr ia=sa->sin_addr;
      return this->sendToIPAddress(&ia,cmd,obj);
   }else if((sa6=this->getLastPacketSockAddrIn6())){
      in6_addr ia6=sa6->sin6_addr;
      return this->sendToIPAddress(&ia6,cmd,obj);
   }
   return false;
 }


rs_cmd_t RSSUDPSocket::getLastCmd(){
   if(!rso_data){return RS_CMD_UNDEFINED;}
   return (rs_cmd_t)this->rso_data->uintObjectByName("cmd");
}

bool RSSUDPSocket::sendToIPAddress(struct in_addr* addr, rs_cmd_t cmd,RSSObject* obj){
   memset(send_sin_tmp,0x00,sizeof(sockaddr_in));
   sockaddr_in* sia=(sockaddr_in*)sin_tmp;
   sia->sin_addr=*addr;
   sia->sin_family=AF_INET;
   sia->sin_port=htons(RS_COMMUNICATION_PORT);
   return this->sendToSockAddr((sockaddr*)sia,sizeof(sockaddr_in),cmd,obj);
}

bool RSSUDPSocket::sendToIPAddress(struct in6_addr* addr,rs_cmd_t cmd, RSSObject* obj){
   memset(send_sin_tmp,0x00,sizeof(sockaddr_in6));
   sockaddr_in6* sia=(sockaddr_in6*)sin_tmp;
   sia->sin6_addr=*addr;
   sia->sin6_family=AF_INET6;
   sia->sin6_port=htons(RS_COMMUNICATION_PORT);
   return this->sendToSockAddr((sockaddr*)sia,sizeof(sockaddr_in6),cmd,obj);
}

bool  RSSUDPSocket::receivePacket(){
   if(this->rso_data){delete this->rso_data;}
   this->rso_data=NULL;
   slen_tmp=sizeof(sin_tmp);
   if(udp_socket==-1){return false;}
   while(true){
      int rr=recvfrom(udp_socket,buffer,buff_maxlen,0,(sockaddr*)&sin_tmp[0],&slen_tmp);
      if(rr==-1){return false;}
      if(rr>0){
      rso_data=new RSSObject(getDefinedStructs(),"packet_t");
         if(rso_data->parse(buffer,rr)==-1){
            delete rso_data;
            this->rso_data=NULL;
         }else{
            break;
         }
      }

   }
   return true;
}

bool RSSUDPSocket::isLastPacketIPv6(){
   if( (slen_tmp==sizeof(sockaddr_in6))&&( ((sockaddr_in6*)sin_tmp)->sin6_family=AF_INET6) ){
      return true;
   }else{
      return false;
   }

}
bool RSSUDPSocket::isLastPacketIPv4(){
   if( (slen_tmp==sizeof(sockaddr_in))&&( ((sockaddr_in*)sin_tmp)->sin_family=AF_INET) ){
      return true;
   }else{
      return false;
   }
}
struct sockaddr_in* RSSUDPSocket::getLastPacketSockAddrIn(){
   if(this->isLastPacketIPv4()){
      return (sockaddr_in*)sin_tmp;
   }else{
      return NULL;
   }
}
struct sockaddr_in6* RSSUDPSocket::getLastPacketSockAddrIn6(){
   if(this->isLastPacketIPv6()){
      return (sockaddr_in6*)sin_tmp;
   }else{
      return NULL;
   }
}

bool RSSUDPSocket::isLastPackedFromRouteServer(){
   if(this->isLastPacketIPv4()){
      sockaddr_in* sia=(sockaddr_in*)sin_tmp;
      if(memcmp(&sia->sin_addr,&server_inaddr,sizeof(in_addr))==0){
         return true;
      }
   }
   return false;
}

std::string* RSSUDPSocket::getLastPacketData(){
   return this->rso_data->dynObjectByName("data");
}

RSSObject* RSSUDPSocket::getLastPackedDataAsType(const char* type_name){
//return null (if unpack error) or object
  std::string *s=this->getLastPacketData();
  if(!s){return NULL;}
  RSSObject* out=new RSSObject(getDefinedStructs(),type_name);
  if(out->parse(s->data(),s->length())==-1){
   delete out;
   return NULL;
  }
  return out;
}