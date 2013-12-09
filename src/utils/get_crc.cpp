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


unsigned int get_crc(const char* data,int length){
		unsigned int result=0;
		result=length;
		unsigned char* d=(unsigned char*)data;
		for(int i=0;i<length;i++){
			result+=*d;
			d++;
		}
		return result^length;
}
