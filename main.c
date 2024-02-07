#include <stdio.h>
#include <stdlib.h>

#include "unlzexe.h"

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
	printf("file '%s' is compressed by LZEXE Ver. 0.%d\n",ipath,ver); /* v0.8 */
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
