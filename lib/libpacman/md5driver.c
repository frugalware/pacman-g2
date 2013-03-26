/* MD5DRIVER.C - taken and modified from MDDRIVER.C (license below)  */
/*               for use in pacman-g2.                               */
/*********************************************************************/

/* Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
rights reserved.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

/* The following makes MD default to MD5 if it has not already been
  defined with C compiler flags.
 */
#define MD MD5

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "md5.h"

#include "util.h"

/* Length of test block, number of test blocks.
 */
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 1000

#define MD_CTX MD5_CTX
#define MDInit _pacman_MD5Init
#define MDUpdate _pacman_MD5Update
#define MDFinal _pacman_MD5Final

char* _pacman_MDFile(char *filename)
{
	FILE *file;
	MD_CTX context;
	int len = 0;
	unsigned char buffer[1024], digest[16];

	if((file = fopen(filename, "rb")) == NULL) {
		printf (_("%s can't be opened\n"), filename);
	} else {
		char *ret;
		int i, x;

		MDInit(&context);
		while((len = fread(buffer, 1, 1024, file))) {
			MDUpdate(&context, buffer, len);
		}
		MDFinal(digest, &context);
		fclose(file);
		/*printf("MD5 (%s) = ", filename);
		MDPrint(digest);
		printf("\n");*/

		ret = (char*)malloc(33);
		ret[0] = '\0';
		for(i = 0; i < 16; i++) {
			x = sprintf(ret + len, "%02x", digest[i]);
			if (x >= 0) { len+= x; }
		}
		ret[32] = '\0';
		return(ret);
	}
	return(NULL);
}

/* Prints a message digest in hexadecimal.
 */
void _pacman_MDPrint(unsigned char digest[16])
{
	unsigned int i;
	for (i = 0; i < 16; i++)
	printf ("%02x", digest[i]);
}

/* vim: set ts=2 sw=2 noet: */
