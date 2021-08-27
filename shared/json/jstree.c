#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "jsmn.h"
#include "jstree.h"


_tree_node * _alloc_tree_node_type(int type)
{
	_tree_node * node;
	node = malloc(sizeof(_tree_node));
	node->data.type = type;
	node->data.data = 0;
	node->l = 0;
	node->r = 0;

	return node;
}

_tree_node * _alloc_tree_node_data(int type, const char * data)
{
	_tree_node * node;
	node = malloc(sizeof(_tree_node));
	node->data.type = type;
	if(data != 0){
		node->data.data = malloc(strlen(data) + 1);
		memcpy(node->data.data, data, strlen(data));
		node->data.data[strlen(data)] = '\0';
	}else{
		node->data.data = 0;
	}
	node->l = 0;
	node->r = 0;

	return node;
}


_tree_node * _alloc_tree_node_len(int type, const char * data, int datalen)
{
	_tree_node * node;
	node = malloc(sizeof(_tree_node));
	node->data.type = type;
	if(data != 0){
		if(datalen >= 0){
			node->data.data = malloc(datalen + 1);
			memcpy(node->data.data, data, datalen);
			node->data.data[datalen] = '\0';
		}else{
			node->data.data = 0;
		}
	}else{
		node->data.data = 0;
	}
	node->l = 0;
	node->r = 0;

	return node;
}

int dumptree(_tree_node * tree, int indent)
{
	int j, k;

	_treenode_data * t = &(tree->data);
	_tree_node * tree_ptr;
	
	if (t->type == JSMN_PRIMITIVE) {
		printf("%s", t->data);
		return 1;
	} else if (t->type == JSMN_STRING) {
		printf("'%s'", t->data);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		j = 0;
		tree_ptr = tree->r;
		printf("{\n");
		while(tree_ptr > 0){
			for (k = 0; k < indent; k++) printf("  ");
			j += dumptree(tree_ptr, indent+1);
			printf(":");
			j += dumptree(tree_ptr->r, indent+1);
			printf("\n");
			tree_ptr = tree_ptr->l;
		};
		for (k = 0; k < indent; k++) printf("  ");
		printf("}");

		return j + 1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		printf("\n");
		tree_ptr = tree->r;
		while(tree_ptr > 0){
			for (k = 0; k < indent; k++) printf("  ");
			printf("- ");
			j += dumptree(tree_ptr, indent+1);
			printf("\n");
			tree_ptr = tree_ptr->l;
		};
		return j+1;
	}

	printf("unknown type\n");
	return 0;

}

int dump(const char *js, jsmntok_t *t, size_t count, int indent) {
	int i, j, k;
	if (count == 0) {
		return 0;
	}
	if (t->type == JSMN_PRIMITIVE) {
		printf("%.*s", t->end - t->start, js+t->start);
		return 1;
	} else if (t->type == JSMN_STRING) {
		printf("'%.*s'", t->end - t->start, js+t->start);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		j = 0;
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent; k++) printf("  ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf(": ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf("\n");
		}
		return j+1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		printf("\n");
		for (i = 0; i < t->size; i++){
			for (k = 0; k < indent-1; k++) printf("  ");
			printf("   - ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf("\n");
		}
		return j+1;
	}
	return 0;
}

jstreeret js2tree(const char *js, jsmntok_t *t, size_t count) {
	int i, j;
	_tree_node ** node_pp;
	jstreeret ret;
	jstreeret tmp;

	ret.ret = 0;

	if (count == 0) {
		return ret;
	}
	
	if (t->type == JSMN_PRIMITIVE) {
		ret.node = alloc_tree_node(t->type, 
				js + t->start, 
				t->end - t->start);
		ret.ret = 1;
		return ret;
	} else if (t->type == JSMN_STRING) {
		ret.node = alloc_tree_node(t->type, 
				js + t->start, 
				t->end - t->start);
		ret.ret = 1;
		return ret;
	} else if (t->type == JSMN_OBJECT) {
		ret.node = alloc_tree_node(t->type);
		node_pp = &((ret.node)->r);

		j = 0;
		for (i = 0; i < t->size; i++) {
			tmp = js2tree(js, t+1+j, count-j);
			j += tmp.ret;
			*node_pp = tmp.node;
			
			tmp = js2tree(js, t+1+j, count-j);
			j += tmp.ret;
			(*node_pp)->r = tmp.node;
			node_pp = &((*node_pp)->l);
		}
		ret.ret = j + 1;
		return ret;
	} else if (t->type == JSMN_ARRAY) {
		ret.node = alloc_tree_node(t->type);
		node_pp = &((ret.node)->r);
		j = 0;
		for (i = 0; i < t->size; i++){
			tmp = js2tree(js, t+1+j, count-j);
			j += tmp.ret;
			*node_pp = tmp.node;
			node_pp = &((*node_pp)->l);
	//		printf("next attrib %x\n", (*node_pp));
		}
		ret.ret = j + 1;
		return ret;
	}

	return ret;
}

_tree_node * find_node(_tree_node * tree, char * key)
{
	int keylen = strlen(key);
	int datalen;
	_tree_node * tmp = tree;
	_treenode_data * data;
	while(1){
//		printf("search tmp %x\n", tmp);
		if(tmp->data.data != 0){
			data = &tmp->data;
			if(data->type == JSMN_PRIMITIVE ||
				data->type == JSMN_STRING){
				datalen = strlen(data->data);
				if(datalen == keylen && 
						memcmp(data->data, key, keylen) == 0){
//					printf("%.*s is a match\n", data->end - data->start, js+data->start);
					return tmp;
				}else{
//					printf("%.*s not a match\n", data->end - data->start, js+data->start);
				}
			}

		}

		if(tmp->l == 0){
//			printf("reach end of tree\n");
			break;
		}
		tmp = tmp->l;
//		printf("next item\n");
	}

	return NULL;
}

_tree_node * next_node( _tree_node *node, int step)
{
	_tree_node * retnode;
	retnode = node;
	while( step-- > 0){
		retnode = retnode->l;
		if(retnode == 0){
			break;
		}
	}

	return retnode;
}

int get_node_string(_tree_node *node, char *buf, int bufsize)
{
	int retlen = 0;
	if(node->data.data > 0){
		retlen = snprintf(buf, bufsize, "%s", node->data.data);
	}else{
		printf("no data\n");
	}

	return retlen;
}


int tree2js(_tree_node * tree, char * out, int outlen, int indent);

static int _tree2js_insert_tree(_tree_node * _jstree, char *out, int outlen, int indent)
{
	int slen; 
	slen = tree2js(_jstree, out, outlen, indent);
	if(slen <= 0){ 
		return 0;
	}

	return slen;
}

static int _tree2js_insert_string(char * out, int outlen, char * _str)
{
	int slen;

	slen = snprintf(out, outlen, "%s", _str);
	if(slen <= 0){ 
		return 0; 
	}

	return slen;
}

static int _tree2js_walk_object(_tree_node * _js_obj, char * out, int outlen, int indent)
{
	int retlen = 0;
	int i;
	_tree_node * node_tmp = _js_obj;

	retlen += _tree2js_insert_string(&out[retlen], outlen - retlen, "{\n");

	do{
		if(_js_obj == 0){
			break;
		}

		for(i = 0; i < indent; i++){
			retlen += _tree2js_insert_string(&out[retlen],
				outlen - retlen, "\t");
		}

		retlen += _tree2js_insert_tree(node_tmp, 
					&out[retlen], outlen -retlen, indent);
		
		retlen += _tree2js_insert_string(&out[retlen], outlen -retlen, ":");
		
		retlen += _tree2js_insert_tree(node_tmp->r, 
					&out[retlen], outlen - retlen, indent);
		if(_js_obj->l != 0){			
			retlen += _tree2js_insert_string(&out[retlen], outlen -retlen, ",");
		}

		retlen += _tree2js_insert_string(&out[retlen], outlen -retlen, "\n");

		if(node_tmp->l != 0){
			node_tmp = node_tmp->l;
		}else{
			break;
		}
	}while(1);

	for(i = 0; i < indent - 1; i++){
		retlen += _tree2js_insert_string(&out[retlen], outlen - retlen, "\t");
	}

	retlen += _tree2js_insert_string(&out[retlen], outlen - retlen, "}");
	return retlen;
}


static int _tree2js_walk_array(_tree_node * js_array, char * out, int outlen, int indent)
{
	int retlen = 0;
	int i;
	_tree_node * node_tmp = js_array;

	retlen += _tree2js_insert_string(&out[retlen], outlen -retlen, "[\n");

	do{
		if(js_array == 0){
			break;
		}

		for(i = 0; i < indent; i++){
			retlen += _tree2js_insert_string(&out[retlen], outlen - retlen, "\t");
		}


		retlen += _tree2js_insert_tree(node_tmp, 
					&out[retlen], outlen -retlen, 
					indent);

		if(node_tmp->l != 0 ){
			retlen += _tree2js_insert_string(&out[retlen], outlen - retlen, ",");
		}


		retlen += _tree2js_insert_string(&out[retlen], outlen - retlen, "\n");

		if(node_tmp->l != 0){
			node_tmp = node_tmp->l;

		}else{
			break;
		}
	}while(1);

	for(i = 0; i < indent - 1; i++){
		retlen += _tree2js_insert_string(&out[retlen], outlen - retlen, "\t");
	}

	retlen += _tree2js_insert_string(&out[retlen], outlen - retlen, "]");

	return retlen;
}

int tree2js(_tree_node * tree, char * out, int outlen, int indent)
{
	int retlen = 0;
	int slen;

	_treenode_data * node = &tree->data;

	if(node->type == JSMN_STRING){
		slen = snprintf(out, outlen, "\"%s\"", node->data);
		return slen;
	}else if(node->type == JSMN_PRIMITIVE){

		slen = snprintf(out, outlen, "%s", node->data);
		return slen;
	}else if(node->type == JSMN_OBJECT){

		retlen = _tree2js_walk_object(tree->r, &out[retlen], outlen, indent + 1);
	
		return retlen;

	}else if(node->type == JSMN_ARRAY){
				
		retlen = _tree2js_walk_array(tree->r, &out[retlen], outlen, indent + 1);
		
		return retlen;
	}

	printf("%s: unknown type should never go here!\n", __func__);
	return 0;
}

int tree2json(_tree_node * tree, char * buf, int bufsize)
{
	return tree2js(tree, buf, bufsize, 0);
}


static _tree_node * _freewtree(_tree_node * node)
{
	_tree_node * ret;
	do{
		if(node->l > 0){
			ret = _freewtree(node->l);
			node->l = 0;
		}else if(node->r > 0){
			ret = _freewtree(node->r);
			node->r = 0;
		}else{
			return node;
		}

		if(ret->data.data > 0){
		//	printf("free %x: %s\n", ret, ret->data.data);
			free(ret->data.data);
		}else{
		//	printf("free object %x\n", ret);
		}
		free(ret);
	}while(1);
}

void freejstree(_tree_node * tree)
{
	_tree_node * ret;

	ret = _freewtree(tree);
	free(ret);
}

