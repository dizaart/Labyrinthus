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

#ifndef _INC_INCLUDED
#define _INC_INCLUDED
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include <string>
#include <vector>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#ifdef __APPLE__ 
#define IFF_TUN 0 //fix for debug
#else
#include <linux/if_tun.h>
#endif 

#include <netinet/in.h>
#include <net/if.h>
#include <fcntl.h>
#include <arpa/inet.h>


#endif

