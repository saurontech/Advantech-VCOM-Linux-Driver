#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "jsmn.h"
#include "jstree.h"

//dev_config _m_dev_conf;

int main(int argc, char * argv[])
{
	int fd;
	int filelen;
	int ret;
	int i;
	int tokcount;
	char * filedata;
	char content[1024];
	char item[1024];
	char *filepath;

	jsmn_parser p;
	jsmntok_t *tok;
	_tree_node *tmp;
	_tree_node * rnode;

	if(argc < 2){
		printf("%s file\n", argv[0]);
		return 0;
	}
	filepath = argv[1];



	fd = open(filepath, O_RDONLY);
	printf("%s fd = %d\n", filepath, fd);

	filelen = lseek(fd, 0, SEEK_END);
//	printf("filelen = %d\n", filelen);
	
	lseek(fd, 0, SEEK_SET);
	filedata = malloc(filelen);

	ret = read(fd, filedata, filelen);

//	printf("ret = %d\n", ret);

	jsmn_init(&p);

	tokcount = 2;
	tok = malloc(sizeof(*tok) * tokcount);

	ret = 0;

	do{
		ret = jsmn_parse(&p, filedata, filelen, tok, tokcount);
		if(ret == JSMN_ERROR_NOMEM){
			tokcount = tokcount * 2;
			tok = realloc_it(tok, sizeof(*tok) * tokcount);
			if(tok == NULL){
				return -1;
			}
			continue;
		}else if(ret < 0){
			printf("failed ret = %d\n", ret);
		}
//		printf("jsmn_parse %d\n", ret);
		break;
	}while(1);

	dump(filedata, tok, p.toknext, 0);

	jstreeret result;

	result = js2tree(filedata, tok, p.toknext);
	dumptree(result.node, 0);

	close(fd);

	return 0;
}


