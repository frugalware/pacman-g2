/* sha.c - Functions to compute SHA1 message digest of files or
   memory blocks according to the NIST specification FIPS-180-1.

   Copyright (C) 2000, 2001, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Scott G. Miller
   Credits:
      Robert Klep <robert@ilse.nl>  -- Expansion function fix
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "util.h"
#include "fsha1.h"

/* Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
rights reserved.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

char* _pacman_SHAFile(char *filename) {
    FILE *file;
    FSHA1 *sha1;
    int len = 0, i, x;
    unsigned char buffer[1024], digest[20];
    char *ret;

    if ((sha1 = f_sha1_new ()) == NULL) {
        return NULL;
    }

    if((file = fopen(filename, "rb")) == NULL) {
	fprintf(stderr, _("%s can't be opened\n"), filename);
    } else {
	f_sha1_init (sha1);
	while((len = fread(buffer, 1, 1024, file))) {
	    f_sha1_update (sha1, buffer, len);
	}
	f_sha1_fini (sha1, digest);
	fclose(file);
#ifdef DEBUG
	SHAPrint(digest);
#endif
	ret = (char*)malloc(41);
	ret[0] = '\0';
	for(i = 0; i < 20; i++) {
	    x = sprintf(ret + len, "%02x", digest[i]);
	    if (x >= 0) { len += x; }
	}
	ret[40] = '\0';
	return(ret);
    }

    f_sha1_delete (sha1);

    return(NULL);
}

