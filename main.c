#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "unlzexe.h"

#define stricmp strcasecmp /* v0.9 */
char *backup_ext = ".olz";

int fnamechg(char *ipath,char *opath,int rename_sw);
void parsepath(char *pathname, int *fname, int *ext);

int main(int argc,char **argv){
	char ofname[FILENAME_MAX];
	FILE *ifile,*ofile;
	int  ver,rename_sw=0;

	printf("UNLZEXE Ver. "VERSION"\n");
	if(argc!=3 && argc!=2){
		printf("usage: UNLZEXE packedfile [unpackedfile]\n");
		exit(EXIT_FAILURE);
	}
	const char *ipath = argv[1];
	if(argc==2)
		rename_sw=1;
	if((ifile=fopen(ipath,"rb"))==NULL){
		printf("'%s' :not found\n",ipath);
			exit(EXIT_FAILURE);
	}

	if(rdhead(ifile,&ver)!=SUCCESS){
		printf("'%s' is not LZEXE file.\n",ipath);
		fclose(ifile); exit(EXIT_FAILURE);
	}
	printf("file '%s' is compressed by LZEXE Ver. 0.%d\n",ipath,ver);

	char opath[FILENAME_MAX];
	if(argc==2)
	{
		strcpy(opath, "unlzexeXXXXXX");
		int fd = mkstemp(opath);
		if (fd == -1)
		{
			printf("can't create temp file.\n");
			fclose(ifile); exit(EXIT_FAILURE);
		}
		// Reopen in binary mode to be able to fseek
		close(fd);
	}
	else
	{
		strcpy(opath, argv[2]);
	}
	ofile=fopen(opath,"w+b");
	if(ofile==NULL){
		printf("can't open '%s'.\n",opath);
		fclose(ifile); exit(EXIT_FAILURE);
	}

	if(mkreltbl(ifile,ofile,ver)!=SUCCESS) {
		fclose(ifile);
		fclose(ofile);
		remove(opath);
		exit(EXIT_FAILURE);
	}
	if(unpack(ifile,ofile)!=SUCCESS) {
		fclose(ifile);
		fclose(ofile);
		remove(opath);
		exit(EXIT_FAILURE);
	}
	fclose(ifile);
	wrhead(ofile);
	fclose(ofile);

	if(fnamechg(ipath,opath,rename_sw)!=SUCCESS){
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

int fnamechg(char *ipath,char *opath,int rename_sw) {
	int idx_name,idx_ext;
	char tpath[FILENAME_MAX];

	if(rename_sw) {
		strcpy(tpath,ipath);
		parsepath(tpath,&idx_name,&idx_ext);
		strcpy(tpath+idx_ext,backup_ext);
		remove(tpath);
		if(rename(ipath,tpath)){
			printf("can't make '%s'.\n", tpath);
			remove(opath);
			return(FAILURE);
		}
	printf("'%s' is renamed to '%s'.\n",ipath,tpath);
	}
	strcpy(tpath,ipath);
	if(rename(opath,tpath)){
		if(rename_sw) {
			strcpy(tpath,ipath);
			parsepath(tpath,&idx_name,&idx_ext);
			strcpy(tpath+idx_ext,backup_ext);
			rename(tpath,ipath);
		}
		printf("can't make '%s'.\n", tpath);

		return(FAILURE);
	}
	printf("unpacked file '%s' is generated.\n",tpath);
	return(SUCCESS);
}

void parsepath(char *pathname, int *fname, int *ext) {
	char c;
	int i;

	*fname=0; *ext=0;
	for(i=0;(c=pathname[i]);i++) {
		switch(c) {
			case ':' :
			case '\\':  *fname=i+1; break;
			case '.' :  *ext=i; break;
			default  :  ;
		}
	}
	if(*ext<=*fname) *ext=i;
}
