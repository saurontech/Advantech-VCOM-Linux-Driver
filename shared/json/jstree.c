#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "jsmn.h"
#include "jstree.h"


int jstree_string_encode(char *out, int outlen, char * data)
{
	int ptr;
	int memlen = strlen(data) + 1;
	int i;
	ptr = 0;

	for(i = 0; i < memlen; i++)
	{
		if(data[i] == '\\' ||
			data[i] == '\"'){
			if(out && outlen){
				out[ptr] = '\\';
			}
			ptr++;

			if(out && 
				outlen && 
				ptr >= outlen){
				return -1;
			}
			
		}

		if(out && outlen){
			out[ptr] = data[i];
		}

		ptr++;
		if( out && 
			outlen && 
			ptr >= outlen){

			return -1;
		}
	}

	return ptr - 1;
}

static int _c_hex(char in)
{
	if(in >= '0' && in <= '9'){
		return in - '0';
	}else if(in >= 'a' && in <= 'f'){
		return 10 + in - 'a';
	}else if(in >= 'A' && in <= 'F'){
		return 10 + in - 'A';
	}else{
		return 0;
	}
}

int jstree_string_decode(char * out, int outlen, char * data)
{
	int i;
	int ptr;
	int memlen = strlen(data) + 1;
	
	ptr = i = 0;
	while(i < memlen){
		if(data[i] != '\\'){
			if(out && outlen){
				out[ptr] = data[i];
			}
			ptr++;
			i++;
			continue;
		}

		if( i + 1 >= memlen){
			return -1;
		}
		
		switch(data[i + 1]){
				case '\\':
				case '\"':
					if(out && outlen && ptr < outlen){
						out[ptr] = data[i + 1];
					}
					i+=2;
					ptr++;
					break;
				case 't':
					if(out && outlen && ptr < outlen){
						out[ptr] = '\t';
					}
					i+=2;
					ptr++;
					break;
				case 'r':
					if(out && outlen && ptr < outlen){
						out[ptr] = '\r';
					}
					i+=2;
					ptr++;
					break;
				case 'n':
					if(out && outlen && ptr < outlen){
						out[ptr] = '\n';
					}
					i+=2;
					ptr++;
					break;
				case 'b':
					if(out && outlen && ptr < outlen){
						out[ptr] = '\b';
					}
					i+=2;
					ptr++;
					break;
				case 'f':
					if(out && outlen && ptr < outlen){
						out[ptr] = '\f';
					}
					i+=2;
					ptr++;
					break;
				case 'u':{
					unsigned short _tmp = 0;
					if(i + 5 >= memlen){
						return -1;
					}
					_tmp += ((unsigned short)_c_hex(data[i + 2]))<< 12;
					_tmp += ((unsigned short)_c_hex(data[i + 3])) << 8;
					_tmp += ((unsigned short)_c_hex(data[i + 4])) << 4;
					_tmp += (unsigned short)_c_hex(data[i + 5]);

					if(_tmp < 0x80){
						if(out && outlen && ptr < outlen){
							out[ptr] = (char)_tmp;
						}
						ptr++;
					}else if(_tmp < 0x800){
						if(out && outlen && ptr + 1 < outlen){
							out[ptr] = (unsigned char)(0xc0 | ((_tmp & 0x7c0) >> 6));
							out[ptr + 1] = (unsigned char)(0x80 | (_tmp & 0x3f));
						}
						ptr += 2;
					}else if(_tmp < 0x10000){
						if(out && outlen && ptr + 2 < outlen){
							out[ptr] = (unsigned char)(0xe0 | ((_tmp & 0xf000) >> 12));
							out[ptr + 1] = (unsigned char)(0x80 | ((_tmp & 0xfc0) >> 6));
							out[ptr + 2] = (unsigned char)(0x80 | (_tmp & 0x3f));
						}
						ptr += 3;
					}else if(_tmp < 0x200000){
						out[ptr] = (unsigned char)(0xf0 | ((_tmp & 0x1b0000) >> 18));
						out[ptr + 1] = (unsigned char)(0xe0 | ((_tmp & 0xf000) >> 12));
						out[ptr + 2] = (unsigned char)(0x80 | ((_tmp & 0xfc0) >> 6));
						out[ptr + 3] = (unsigned char)(0x80 | (_tmp & 0x3f));
						ptr += 4;
					}
						
					i+= 6;
				}
				break;
				default:
					return -1;
					break;
			}
	}
	return ptr - 1;
}



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
	if(data != 0 && datalen >= 0){
		node->data.data = malloc(datalen + 1);
		memcpy(node->data.data, data, datalen);
		node->data.data[datalen] = '\0';
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

	if(tree == 0){
		return 0;
	}

	_treenode_data * t = &(tree->data);
	_tree_node * tree_ptr;
	
	if (t->type == JSMN_PRIMITIVE) {
		printf("%s", t->data);
		return 1;
	} else if (t->type == JSMN_STRING) {
		printf("\"%s\"", t->data);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		j = 0;
		tree_ptr = tree->r;
		printf("{\n");
		while(tree_ptr > 0){
			for (k = 0; k < indent; k++) printf(" ");
			j += dumptree(tree_ptr, indent+1);
			printf(":");
			j += dumptree(tree_ptr->r, indent+1);
			if(tree_ptr->l != 0){
				printf(",");
			}
			printf("\n");
			tree_ptr = tree_ptr->l;
		};
		for (k = 0; k < indent; k++) printf(" ");
		printf("}");

		return j + 1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		printf("[\n");
		tree_ptr = tree->r;

		while(tree_ptr > 0){
			for (k = 0; k < indent; k++) printf(" ");
			//printf("- ");
			j += dumptree(tree_ptr, indent+1);
			if(tree_ptr->l != 0){
				printf(",");
			}
			printf("\n");
			tree_ptr = tree_ptr->l;
		};

		for (k = 0; k < indent; k++){
			printf(" ");
		}
		printf("]");

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
			for (k = 0; k < indent; k++) printf(" ");
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
			for (k = 0; k < indent-1; k++) printf(" ");
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

_tree_node **  end_node(_tree_node ** node)
{
	_tree_node ** tmp = node;
	
	while((*tmp) != 0 ){
		tmp = &((*tmp)->l);
	}

	return tmp;
}


int tree2js(_tree_node * tree, char * out, int outlen, int indent);

static int _tree2js_insert_tree(_tree_node * _jstree, char *out, int outlen, int indent)
{
	int slen; 

	if(_jstree == 0){
		return 0;
	}
	
	slen = tree2js(_jstree, out, outlen, indent);
	if(slen <= 0){ 
		return 0;
	}

	return slen;
}

static int _tree2js_insert_string(char * out, int outlen, char * _str)
{
	int slen;

	if(out ==  0 ){
		outlen = 0;
	}
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

	retlen += _tree2js_insert_string( out?(&out[retlen]):0, outlen - retlen, "{\n");

	do{
		if(_js_obj == 0){
			break;
		}

		for(i = 0; i < indent; i++){
			retlen += _tree2js_insert_string(out?(&out[retlen]):0,
				outlen - retlen, "\t");
		}

		retlen += _tree2js_insert_tree(node_tmp, 
					out?(&out[retlen]):0, outlen -retlen, indent);
		
		retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen -retlen, ":");
		
		retlen += _tree2js_insert_tree(node_tmp->r, 
					out?(&out[retlen]):0, outlen - retlen, indent);
		if(node_tmp->l != 0){
			retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen -retlen, ",");
		}

		retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen -retlen, "\n");

		if(node_tmp->l != 0){
			node_tmp = node_tmp->l;
		}else{
			break;
		}
	}while(1);

	for(i = 0; i < indent - 1; i++){
		retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen - retlen, "\t");
	}

	retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen - retlen, "}");
	return retlen;
}


static int _tree2js_walk_array(_tree_node * js_array, char * out, int outlen, int indent)
{
	int retlen = 0;
	int i;
	_tree_node * node_tmp = js_array;

	retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen -retlen, "[\n");

	do{
		if(js_array == 0){
			break;
		}

		for(i = 0; i < indent; i++){

			retlen += _tree2js_insert_string(out?(&out[retlen]):0, 
					outlen - retlen, "\t");
		}

		retlen += _tree2js_insert_tree(node_tmp, 
					out?(&out[retlen]):0, outlen -retlen, 
					indent);

		if(node_tmp->l != 0 ){
			retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen - retlen, ",");
		}

		retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen - retlen, "\n");

		if(node_tmp->l != 0){
			node_tmp = node_tmp->l;

		}else{
			break;
		}
	}while(1);

	for(i = 0; i < indent - 1; i++){
		retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen - retlen, "\t");
	}

	retlen += _tree2js_insert_string(out?(&out[retlen]):0, outlen - retlen, "]");

	return retlen;
}

int tree2js(_tree_node * tree, char * out, int outlen, int indent)
{
	int retlen = 0;
	int slen;

	_treenode_data * node = &tree->data;
	
	if(out == 0){
		outlen = 0;
	}

	if(node->type == JSMN_STRING){
		slen = snprintf(out, outlen, "\"%s\"", node->data);
		return slen;
	}else if(node->type == JSMN_PRIMITIVE){
		slen = snprintf(out, outlen, "%s", node->data);
		return slen;
	}else if(node->type == JSMN_OBJECT){

		retlen = _tree2js_walk_object(tree->r, out?(&out[retlen]):0, outlen, indent + 1);
	
		return retlen;

	}else if(node->type == JSMN_ARRAY){
				
		retlen = _tree2js_walk_array(tree->r, out?(&out[retlen]):0, outlen, indent + 1);
		
		return retlen;
	}

	printf("%s: unknown type should never go here!\n", __func__);
	return 0;
}

int tree2json(_tree_node * tree, char * buf, int bufsize)
{
	int ret;

	ret = tree2js(tree, buf, bufsize, 0);

	if(buf == 0){
		ret += 1;
	}

	return ret ;
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

