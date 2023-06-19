static char * nopass = "";

typedef struct {
	char *password;
	char *rootca;
	char *keyfile;
	int accept_expired_key;
}vc_ssl_cfg;

static char * create_cfg_cwd(char * config_file)
{
	char *wd;
	char *wd_end;

	if((wd_end = memrchr(config_file, '/', strlen(config_file)))){
		int wdlen = wd_end - config_file + 1;
		

		wd = malloc(wdlen + 1);
		if(wd == 0){
			printf("can't malloc for chdir\n");
			return 0;
		}
		wd[wdlen] = '\0';
		memcpy(wd, config_file , wdlen);
	}else{
		return 0;
	}
	
	return wd;
}

static int loadconfig(char * filepath, vc_ssl_cfg * cfg)
{
	int fd;
	int filelen;
	int ret;
	int tokcount;
	char * filedata;

	jsmn_parser p;
	jsmntok_t *tok;
	_tree_node * rnode;


	fd = open(filepath, O_RDONLY);
//	printf("%s fd = %d\n", filepath, fd);
	if(fd <= 0){
		//can't open file
		return -1;
	}

	filelen = lseek(fd, 0, SEEK_END);
//	printf("filelen = %d\n", filelen);
	
	lseek(fd, 0, SEEK_SET);
	filedata = malloc(filelen);

	ret = read(fd, filedata, filelen);

//	printf("ret = %d\n", ret);

	jsmn_init(&p);

	tokcount = 2;
	tok = malloc(sizeof(*tok) * tokcount);

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
		printf("jsmn_parse %d\n", ret);
		break;
	}while(1);

//	dump(filedata, tok, p.toknext, 0);

	jstreeret result;

	result = js2tree(filedata, tok, p.toknext);
	//dumptree(result.node, 0);

	close(fd);
	free(filedata);
	free(tok);

	if(jstree_read(result.node->r, &rnode, "ssl", "keyfile")!= 2){
		printf("didn't find keyfile\n");
		return -1;
	}
	printf("found keyfile = %s\n", rnode->data.data);

	cfg->keyfile = rnode->data.data;

	if(jstree_read(result.node->r, &rnode, "ssl", "rootca")!= 2){
		printf("didn't find rootCA\n");
		return -1;
	}
	printf("found rootca = %s\n", rnode->data.data);

	cfg->rootca = rnode->data.data;

	if(jstree_read(result.node->r, &rnode, "ssl", "password")!= 2){
		printf("didn't find password\n");
		cfg->password = nopass;
	}else{
		int _len = jstree_string_decode(0, 0, rnode->data.data);
		cfg->password = malloc(_len + 1);
		jstree_string_decode(cfg->password, _len + 1, rnode->data.data);
	}

	printf("found password = %s\n", cfg->password);

	if(jstree_read(result.node->r, &rnode, "ssl", "accept_expired_key")== 2 &&
	strcmp(rnode->data.data, "enable") == 0){
		printf("accept expired CA\n");
		cfg->accept_expired_key = 1;
	}else {
		printf("reject expired CA\n");
		cfg->accept_expired_key = 0;
	}

	close(fd);

	return 0;
}
