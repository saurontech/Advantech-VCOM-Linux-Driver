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

int main(int argc, char ** argv)
{
	char buf[4096];
	char name[64], val[64];
	int retlen;
	int fd;
	int ret;
	int i, j, k;
	_tree_node * root;
	_tree_node ** root_ptr;
	_tree_node ** obj_ptr;
	_tree_node ** array_ptr;
	_tree_node ** array_obj_ptr;

	if(argc != 2){
		printf("%s [vcom.json]\n", argv[0]);
		return 0;
	}

	fd = open(argv[1], O_CREAT|O_TRUNC|O_RDWR, S_IRWXU);

	root = alloc_tree_node(JSMN_OBJECT);

	root_ptr = &(root->r);

	for(i = 0; i < 3; i++){
		snprintf(name, 64, "obj%d", i);
		*root_ptr = alloc_tree_node(JSMN_STRING, name);
		(*root_ptr)->r = alloc_tree_node(JSMN_OBJECT);

		obj_ptr = &((*root_ptr)->r->r);
		for(j= 0; j < 2; j++){
			snprintf(name, 64, "attr%d", j);
			snprintf(val, 64, "%d", j);
			*obj_ptr = alloc_tree_node(JSMN_STRING, name);
			(*obj_ptr)->r = alloc_tree_node(JSMN_STRING, val);
			obj_ptr = &((*obj_ptr)->l);
		}

		snprintf(name, 64, "array%d", i);
		*obj_ptr = alloc_tree_node(JSMN_STRING, name);
		(*obj_ptr)->r = alloc_tree_node(JSMN_ARRAY, name);

		array_ptr = &((*obj_ptr)->r->r);
		for(j = 0; j < 2; j++){
			*array_ptr = alloc_tree_node(JSMN_OBJECT);
			array_obj_ptr = &((*array_ptr)->r);
			for(k = 0; k < 3; k++){
				snprintf(name, 64, "array_obj_attr%d", k);
				snprintf(val, 64, "%d", k);
				*array_obj_ptr = alloc_tree_node(JSMN_STRING, name);
				(*array_obj_ptr)->r = alloc_tree_node(JSMN_STRING, val);
				array_obj_ptr = &((*array_obj_ptr)->l);
			}
			array_ptr = &((*array_ptr)->l);

		}

		obj_ptr = &((*obj_ptr)->l);
		*obj_ptr = alloc_tree_node(JSMN_STRING, "pure_array");
		(*obj_ptr)->r = alloc_tree_node(JSMN_ARRAY);
		array_ptr = &((*obj_ptr)->r->r);
		for(j = 0; j < 5; j++){
			snprintf(val, 64, "elem%d", j);
			*array_ptr = alloc_tree_node(JSMN_STRING, val);
			array_ptr = &((*array_ptr)->l);
		}

		


		root_ptr = &((*root_ptr)->l);
	}


/*
	root->r = alloc_tree_node(JSMN_STRING, "obj1");
	ptr = &(root->r->r); 
	*ptr = alloc_tree_node(JSMN_OBJECT);

	ptr = &((*ptr)->r);
	*ptr = alloc_tree_node(JSMN_STRING, "obj1attr1");
	(*ptr)->r = alloc_tree_node(JSMN_STRING, "val1");

	ptr = &((*ptr)->l);
	*ptr = alloc_tree_node(JSMN_STRING, "obj1array1");
	(*ptr)->r = alloc_tree_node(JSMN_ARRAY);
	ptr = &((*ptr)->r);

	(*ptr)->r = alloc_tree_node(JSMN_OBJECT);

	(*ptr)->r->r = alloc_tree_node(JSMN_STRING, "obj1array1elem1_1");
	(*ptr)->r->r->r = alloc_tree_node(JSMN_STRING, "1_1", 0);
	(*ptr)->r->r->l = alloc_tree_node(JSMN_STRING, "obj1array1elem1_2");
	(*ptr)->r->r->l->r = alloc_tree_node(JSMN_STRING, "1_2");

	(*ptr)->r->l = alloc_tree_node(JSMN_OBJECT);
	(*ptr)->r->l->r = alloc_tree_node(JSMN_STRING, "obj1array1elem2_1");
	(*ptr)->r->l->r->r = alloc_tree_node(JSMN_STRING, "2_1");
	(*ptr)->r->l->r->l = alloc_tree_node(JSMN_STRING, "obj1array1elem2_2");
	(*ptr)->r->l->r->l->r = alloc_tree_node(JSMN_STRING, "2_2");
*/
	dumptree(root, 0);


	retlen = tree2json(root, buf, sizeof(buf));
	printf("retlen = %d\n", retlen);

	printf("%s\n", buf);

	ret = write(fd, buf, retlen);

	printf("write ret = %d\n", ret);

	close(fd);
	freewtree(root);

	return 0;
}
