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


#include "inc.h"
#include "tun.h"

#ifdef __APPLE__
int tun_alloc(const char *dev, int flags) {
  return 1;
}
#else


int tun_alloc(const char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
 const char *clonedev = "/dev/net/tun";

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }

   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     close(fd);
     return err;
   }

//  strcpy(dev, ifr.ifr_name);
  return fd;
}


#endif
 //apple
