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

void print_ip4(in_addr ip){
  unsigned char* ips=(unsigned char*)&ip;
  printf("%d.%d.%d.%d",ips[0],ips[1],ips[2],ips[3]);
}



