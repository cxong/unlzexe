#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unlzexe.h"

#define stricmp strcasecmp /* v0.9 */
char *tmpfname = "$tmpfil$.exe";
char *backup_ext = ".olz";

int fnamechk(char *ipath,char *opath, char *ofname,
			  int argc,char **argv);
int fnamechg(char *ipath,char *opath,char *ofname,int rename_sw);
void parsepath(char *pathname, int *fname, int *ext);

int main(int argc,char **argv){
	char ipath[FILENAME_MAX],
	  opath[FILENAME_MAX],
	  ofname[FILENAME_MAX];
	FILE *ifile,*ofile;
	int  ver,rename_sw=0;

	printf("UNLZEXE Ver. "VERSION"\n");
	if(argc!=3 && argc!=2){
		printf("usage: UNLZEXE packedfile [unpackedfile]\n");
		exit(EXIT_FAILURE);
	}
	if(argc==2)
		rename_sw=1;
	if(fnamechk(ipath,opath,ofname,argc,argv)!=SUCCESS) {
		exit(EXIT_FAILURE);
	}
	if((ifile=fopen(ipath,"rb"))==NULL){
		printf("'%s' :not found\n",ipath);
			exit(EXIT_FAILURE);
	}

	if(rdhead(ifile,&ver)!=SUCCESS){
		printf("'%s' is not LZEXE file.\n",ipath);
		fclose(ifile); exit(EXIT_FAILURE);
	}
	if((ofile=fopen(opath,"w+b"))==NULL){
		printf("can't open '%s'.\n",opath);
		fclose(ifile); exit(EXIT_FAILURE);
	}
	printf("file '%s' is compressed by LZEXE Ver. 0.%d\n",ipath,ver);
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

	if(fnamechg(ipath,opath,ofname,rename_sw)!=SUCCESS){
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

/* file name check */
int fnamechk(char *ipath,char *opath, char *ofname,
			  int argc,char **argv) {
	int idx_name,idx_ext;

	strcpy(ipath,argv[1]);
	parsepath(ipath,&idx_name,&idx_ext);
	if (! ipath[idx_ext]) strcpy(ipath+idx_ext,".exe");
	if(! stricmp(ipath+idx_name,tmpfname)){
		printf("'%s':bad filename.\n",ipath);
		return(FAILURE);
	}
	if(argc==2)
		strcpy(opath,ipath);
	else
		strcpy(opath,argv[2]);
	parsepath(opath,&idx_name,&idx_ext);
	if (! opath[idx_ext]) strcpy(opath+idx_ext,".exe");
	if (!stricmp(opath+idx_ext,backup_ext)){
		printf("'%s':bad filename.\n",opath);
		return(FAILURE);
	}
	strncpy(ofname,opath+idx_name,FILENAME_MAX-1);
	strcpy(opath+idx_name,tmpfname);
	return(SUCCESS);
}

int fnamechg(char *ipath,char *opath,char *ofname,int rename_sw) {
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
	strcpy(tpath,opath);
	parsepath(tpath,&idx_name,&idx_ext);
	strcpy(tpath+idx_name,ofname);
	remove(tpath);
	if(rename(opath,tpath)){
		if(rename_sw) {
			strcpy(tpath,ipath);
			parsepath(tpath,&idx_name,&idx_ext);
			strcpy(tpath+idx_ext,backup_ext);
			rename(tpath,ipath);
		}
		printf("can't make '%s'.  unpacked file '%s' is remained.\n",
				 tpath, tmpfname);

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
