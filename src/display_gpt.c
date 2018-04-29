/* **********************************************************************
* Copyright (C) 2016 Elliott Mitchell <ehem+android@m5p.com>		*
*									*
*	This program is free software: you can redistribute it and/or	*
*	modify it under the terms of the GNU General Public License as	*
*	published by the Free Software Foundation, either version 3 of	*
*	the License, or (at your option) any later version.		*
*									*
*	This program is distributed in the hope that it will be useful,	*
*	but WITHOUT ANY WARRANTY; without even the implied warranty of	*
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
*	GNU General Public License for more details.			*
*									*
*	You should have received a copy of the GNU General Public	*
*	License along with this program.  If not, see			*
*	<http://www.gnu.org/licenses/>.					*
************************************************************************/


#define _LARGEFILE64_SOURCE

#include <iconv.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "gpt.h"


static const char *progname;

static bool display_gpt(const char *);


int main(int argc, char **argv)
{
	int ret=0;

	progname=argv[0];

	if(argc<2) {
		fprintf(stderr, "%s <GPT file(s)>\n", progname);
		exit(1);
	}

	while(--argc) if(!display_gpt(*++argv)) ret=1;

	return ret;
}


static bool display_gpt(const char *filename)
{
	struct gpt_data *data, *alt;
	int f;
	char buf0[37], buf1[37];
	int i;

	if((f=open(filename, O_RDONLY|O_LARGEFILE))<0) {
		fprintf(stderr, "%s: open() failed: %s\n", progname, strerror(errno));
		return false;
	}
	if(!(data=readgpt(f, GPT_ANY))) {
		printf("No GPT found in \"%s\"\n", filename);
		return false;
	}

	if((data->head.myLBA==1)&&(alt=readgpt(f, GPT_BACKUP))) {
		if(comparegpt(data, alt)) printf("Found identical primary and backup GPTs\n");
		else printf("Primary and backup GPTs differ!\n");
		free(alt);
	} else printf("Second GPT is absent!\n");

	printf("Found v%hu.%hu %s GPT in \"%s\" (%jd sector size)\n",
data->head.major, data->head.minor,
data->head.myLBA==1?"primary":"backup", filename, data->blocksz);
	uuid_unparse(data->head.diskUuid, buf0);
	printf("device=%s\nmyLBA=%llu altLBA=%llu dataStart=%llu "
"dataEnd=%llu\n\n", buf0, (unsigned long long)data->head.myLBA,
(unsigned long long)data->head.altLBA,
(unsigned long long)data->head.dataStartLBA,
(unsigned long long)data->head.dataEndLBA);

	for(i=0; i<data->head.entryCount; ++i) {
		if(uuid_is_null(data->entry[i].id)) {
			printf("Name(%d): <empty entry>\n", i+1);
		} else {
			uuid_unparse(data->entry[i].type, buf0);
			uuid_unparse(data->entry[i].id, buf1);
			printf("Name(%d): \"%s\" start=%llu end=%llu count=%llu\n"
"typ=%s id=%s\n", i+1, data->entry[i].name,
(unsigned long long)data->entry[i].startLBA,
(unsigned long long)data->entry[i].endLBA,
(unsigned long long)(data->entry[i].endLBA-data->entry[i].startLBA+1), buf0,
buf1);
		}
	}

	free(data);
	return true;
}

