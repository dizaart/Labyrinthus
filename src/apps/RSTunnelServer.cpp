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
