#include "overload.h"

typedef struct _t_treenode_data{
	int type;
	char *data;
}_treenode_data;

typedef struct _t_treenode{
	_treenode_data data;
	struct _t_treenode * l;
	struct _t_treenode * r;
}_tree_node;


static inline void *realloc_it(void *ptrmem, size_t size) {
	void *p;
	if(size == 0 ){
		free(ptrmem);
		return 0;
	}
	p = realloc(ptrmem, size);
	if (!p)  {
		free (ptrmem);
		//fprintf(stderr, "realloc(): errno=%d\n", errno);
	}
	return p;
}

int dump(const char *js, jsmntok_t *t, size_t count, int indent);

typedef struct {
	int ret;
	_tree_node * node;
} jstreeret;

jstreeret js2tree(const char *js, jsmntok_t *t, size_t count);
_tree_node * _alloc_tree_node_len(int type, const char * data, int datalen);
_tree_node * _alloc_tree_node_data(int type, const char * data);
_tree_node * _alloc_tree_node_type(int type);

#define alloc_tree_node(args...)	ovrld3(args, _alloc_tree_node_len, \
						_alloc_tree_node_data, \
						_alloc_tree_node_type)(args)

_tree_node * find_node( _tree_node * tree, char * key);
_tree_node * next_node( _tree_node *node, int step);
_tree_node ** end_node(_tree_node ** node);
int get_node_string( _tree_node *node, char *buf, int bufsize);
int dumptree(_tree_node * tree, int indent);
int tree2json(_tree_node * tree, char * buf, int bufsize);
void freejstree(_tree_node * tree);

int jstree_string_decode(char * out, int outlen, char * data);
int jstree_string_encode(char *out, int outlen, char * data);

