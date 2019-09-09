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
	int loop = argc - 2;

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
//	printf("%s fd = %d\n", filepath, fd);

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

	jstreeret result;

	result = js2tree(filedata, tok, p.toknext);
//	dumptree(result.node, 0);

//	printf("loop = %d\n", loop);

	rnode = result.node->r;
	i = 0;
	int iter;

	while(i < loop){
		ret = sscanf(argv[2 + i], "[%d]", &iter);
		if(rnode <= 0){
			printf("cannot step forward\n");
			break;
		}

		if(ret > 0){
	//		printf("jumping to the %dth node\n", iter);
			rnode = next_node(rnode, iter);
			if(rnode <= 0){
				printf("->[%d]:N/A", iter);
				break;
			}else if(rnode->data.type == JSMN_STRING){
				printf("->[%d](string): %s", iter, rnode->data.data);
				if(i + 1 < loop){
					rnode = rnode->r;
					if(rnode == 0 ){
						printf("cannot move forward");
						break;
					}else if(rnode->data.type == JSMN_ARRAY){
						rnode = rnode->r;
						printf("->(array)");
					}else if(rnode->data.type == JSMN_OBJECT){
						rnode = rnode->r;
						printf("->(object)");
					}
				}
			}else if(rnode->data.type == JSMN_PRIMITIVE){
				printf("->[%d](primitive): %s", iter, rnode->data.data);
			}else if(rnode->data.type == JSMN_OBJECT){
				printf("->[%d](object)", iter);
				rnode = rnode->r;
			}else if(rnode->data.type == JSMN_ARRAY){
				printf("->[%d](array)", iter);
				rnode = rnode->r;
			}else{
				printf("->[%d]unknonw type %x", iter, rnode->data.type);
				break;
			}
			
		}else{

			printf("->%s", argv[2 + i]);
			rnode = find_node(rnode, argv[2 + i]);
			
			if(rnode == 0){
				printf("not found");
				break;
			}else if(rnode->r == 0){
				printf(":has no child");
			}else if(rnode->r->data.type == JSMN_STRING){
				printf("->(string): %s", rnode->r->data.data);
				break;
			}else if(rnode->r->data.type == JSMN_PRIMITIVE){
				printf("->(primitive): %s", rnode->r->data.data);
				break;
			}else if(rnode->r->data.type == JSMN_OBJECT){
				printf("->(object)");
				rnode = rnode->r->r;
			}else if(rnode->r->data.type == JSMN_ARRAY){
				printf("->(array)");
				rnode = rnode->r->r;
			}else{
				printf("child unknonw type %x", rnode->r->data.type);
				break;
			}
		}
		i++;
	}
	printf("\n");

	close(fd);

	return 0;
}
