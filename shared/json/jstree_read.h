#ifndef _JSTREE_READ_H
#define _JSTREE_READ_H

#include <stdarg.h>

#define PP_RSEQ_N() \
         127,126,125,124,123,122,121,120, \
         119,118,117,116,115,114,113,112,111,110, \
         109,108,107,106,105,104,103,102,101,100, \
         99,98,97,96,95,94,93,92,91,90, \
         89,88,87,86,85,84,83,82,81,80, \
         79,78,77,76,75,74,73,72,71,70, \
         69,68,67,66,65,64,63,62,61,60, \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,_64,_65,_66,_67,_68,_69,_70, \
         _71,_72,_73,_74,_75,_76,_77,_78,_79,_80, \
         _81,_82,_83,_84,_85,_86,_87,_88,_89,_90, \
         _91,_92,_93,_94,_95,_96,_97,_98,_99,_100, \
         _101,_102,_103,_104,_105,_106,_107,_108,_109,_110, \
         _111,_112,_113,_114,_115,_116,_117,_118,_119,_120, \
         _121,_122,_123,_124,_125,_126,_127,N,...) N

#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)

#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())

static inline int _do_jstree_read(int argc, _tree_node *input, _tree_node **output, ...)
{
	int loop = argc - 2;
	int iter;
	int i;
	va_list ap;
	_tree_node * rnode = input;

	argc-= 2;

	if(argc < 0){
		return -1;
	}

	i = 0;
	va_start(ap, output);
	while( i < loop){
		char *argv;
		argv = va_arg(ap, char *);
		//printf("argv = %s\n", argv);
		if(rnode == 0){
			printf("cannot step forward\n");
			break;
		}

		if(sscanf(argv, "[%d]", &iter)){
	//		printf("jumping to the %dth node\n", iter);
			rnode = next_node(rnode, iter);
			if(rnode == 0){
				//printf("->[%d]:N/A", iter);
				break;
			}else if(rnode->data.type == JSMN_STRING){
				//printf("->[%d](string): %s", iter, rnode->data.data);
				if(i + 1 < loop){
					rnode = rnode->r;
					if(rnode == 0 ){
						//printf("cannot move forward");
						break;
					}else if(rnode->data.type == JSMN_ARRAY){
						rnode = rnode->r;
					//	printf("->(array)");
					}else if(rnode->data.type == JSMN_OBJECT){
						rnode = rnode->r;
					//	printf("->(object)");
					}
				}
			}else if(rnode->data.type == JSMN_PRIMITIVE){
				//printf("->[%d](primitive): %s", iter, rnode->data.data);
			}else if(rnode->data.type == JSMN_OBJECT){
				//printf("->[%d](object)", iter);
				rnode = rnode->r;
			}else if(rnode->data.type == JSMN_ARRAY){
				//printf("->[%d](array)", iter);
				rnode = rnode->r;
			}else{
				//printf("->[%d]unknonw type %x", iter, rnode->data.type);
				break;
			}
			
		}else{

			//printf("->%s", argv);
			rnode = find_node(rnode, argv);
			
			if(rnode == 0){
				//printf("not found");
				break;
			}else if(rnode->r == 0){
				//printf(":has no child");
				break;
			}else if(rnode->r->data.type == JSMN_STRING){
				//printf("->(string): %s", rnode->r->data.data);
				rnode = rnode->r;
				i++;
				break;
			}else if(rnode->r->data.type == JSMN_PRIMITIVE){
				//printf("->(primitive): %s", rnode->r->data.data);
				rnode = rnode->r;
				i++;
				break;
			}else if(rnode->r->data.type == JSMN_OBJECT){
				//printf("->(object)");
				rnode = rnode->r->r;
			}else if(rnode->r->data.type == JSMN_ARRAY){
				//printf("->(array)");
				rnode = rnode->r->r;
			}else{
				//printf("child unknonw type %x", rnode->r->data.type);
				break;
			}
		}
		i++;
	}
	//printf("\n");
	va_end(ap);

	//printf("ret = %d\n",ret);

	*output = rnode;

	return i;
}

//#define readJSTree(...) _do_jstree_read(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define jstree_read(...) _do_jstree_read(PP_NARG(__VA_ARGS__), __VA_ARGS__)

#endif
