/*
* unlzexe ver 0.5 (PC-VAN UTJ44266 Kou )
*   UNLZEXE converts the compressed file by lzexe(ver.0.90,0.91) to the
*   UNcompressed executable one.
*
*   usage:  UNLZEXE packedfile[.EXE] [unpackedfile.EXE]

v0.6  David Kirschbaum, Toad Hall, kirsch@usasoc.soc.mil, Jul 91
	Problem reported by T.Salmi (ts@uwasa.fi) with UNLZEXE when run
	with TLB-V119 on 386's.
	Stripping out the iskanji and isjapan() stuff (which uses a somewhat
	unusual DOS interrupt) to see if that's what's biting us.

--  Found it, thanks to Dan Lewis (DLEWIS@SCUACC.SCU.EDU).
	Silly us:  didn't notice the "r.h.al=0x3800;" in isjapan().
	Oh, you don't see it either?  INT functions are called with AH
	having the service.  Changing to "r.x.ax=0x3800;".

v0.7  Alan Modra, amodra@sirius.ucs.adelaide.edu.au, Nov 91
    Fixed problem with large files by casting ihead components to long
    in various expressions.
    Fixed MinBSS & MaxBSS calculation (ohead[5], ohead[6]).  Now UNLZEXE
    followed by LZEXE should give the original file.

v0.8  Vesselin Bontchev, bontchev@fbihh.informatik.uni-hamburg.de, Aug 92
    Fixed recognition of EXE files - both 'MZ' and 'ZM' in the header
    are recognized.
    Recognition of compressed files made more robust - now just
    patching the 'LZ90' and 'LZ91' strings will not fool the program.

v0.9  Stian Skjelstad, stian.skjelstad@gmail.com, Aug 2019
    Use memmove when memory-regions overlap
    Do not use putw/getw, since they on modern systems do not read/write 16bit
    getc() return char, which might be signed.
    Include POSIX headers
    span + pointer, was expected to wrap 16bit
*/
#include "unlzexe.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>        /* v0.9 */
typedef uint16_t WORD;     /* v0.9 */
typedef uint8_t BYTE;      /* v0.9 */

int reloc90(FILE *ifile,FILE *ofile,long fpos);
int reloc91(FILE *ifile,FILE *ofile,long fpos);

/*-------------------------------------------*/
static WORD ihead[0x10],ohead[0x10],inf[8];
static long loadsize;
static BYTE sig90 [] = {			/* v0.8 */
    0x06, 0x0E, 0x1F, 0x8B, 0x0E, 0x0C, 0x00, 0x8B,
    0xF1, 0x4E, 0x89, 0xF7, 0x8C, 0xDB, 0x03, 0x1E,
    0x0A, 0x00, 0x8E, 0xC3, 0xB4, 0x00, 0x31, 0xED,
    0xFD, 0xAC, 0x01, 0xC5, 0xAA, 0xE2, 0xFA, 0x8B,
    0x16, 0x0E, 0x00, 0x8A, 0xC2, 0x29, 0xC5, 0x8A,
    0xC6, 0x29, 0xC5, 0x39, 0xD5, 0x74, 0x0C, 0xBA,
    0x91, 0x01, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0xFF,
    0x4C, 0xCD, 0x21, 0x53, 0xB8, 0x53, 0x00, 0x50,
    0xCB, 0x2E, 0x8B, 0x2E, 0x08, 0x00, 0x8C, 0xDA,
    0x89, 0xE8, 0x3D, 0x00, 0x10, 0x76, 0x03, 0xB8,
    0x00, 0x10, 0x29, 0xC5, 0x29, 0xC2, 0x29, 0xC3,
    0x8E, 0xDA, 0x8E, 0xC3, 0xB1, 0x03, 0xD3, 0xE0,
    0x89, 0xC1, 0xD1, 0xE0, 0x48, 0x48, 0x8B, 0xF0,
    0x8B, 0xF8, 0xF3, 0xA5, 0x09, 0xED, 0x75, 0xD8,
    0xFC, 0x8E, 0xC2, 0x8E, 0xDB, 0x31, 0xF6, 0x31,
    0xFF, 0xBA, 0x10, 0x00, 0xAD, 0x89, 0xC5, 0xD1,
    0xED, 0x4A, 0x75, 0x05, 0xAD, 0x89, 0xC5, 0xB2,
    0x10, 0x73, 0x03, 0xA4, 0xEB, 0xF1, 0x31, 0xC9,
    0xD1, 0xED, 0x4A, 0x75, 0x05, 0xAD, 0x89, 0xC5,
    0xB2, 0x10, 0x72, 0x22, 0xD1, 0xED, 0x4A, 0x75,
    0x05, 0xAD, 0x89, 0xC5, 0xB2, 0x10, 0xD1, 0xD1,
    0xD1, 0xED, 0x4A, 0x75, 0x05, 0xAD, 0x89, 0xC5,
    0xB2, 0x10, 0xD1, 0xD1, 0x41, 0x41, 0xAC, 0xB7,
    0xFF, 0x8A, 0xD8, 0xE9, 0x13, 0x00, 0xAD, 0x8B,
    0xD8, 0xB1, 0x03, 0xD2, 0xEF, 0x80, 0xCF, 0xE0,
    0x80, 0xE4, 0x07, 0x74, 0x0C, 0x88, 0xE1, 0x41,
    0x41, 0x26, 0x8A, 0x01, 0xAA, 0xE2, 0xFA, 0xEB,
    0xA6, 0xAC, 0x08, 0xC0, 0x74, 0x40, 0x3C, 0x01,
    0x74, 0x05, 0x88, 0xC1, 0x41, 0xEB, 0xEA, 0x89
}, sig91 [] = {
    0x06, 0x0E, 0x1F, 0x8B, 0x0E, 0x0C, 0x00, 0x8B,
    0xF1, 0x4E, 0x89, 0xF7, 0x8C, 0xDB, 0x03, 0x1E,
    0x0A, 0x00, 0x8E, 0xC3, 0xFD, 0xF3, 0xA4, 0x53,
    0xB8, 0x2B, 0x00, 0x50, 0xCB, 0x2E, 0x8B, 0x2E,
    0x08, 0x00, 0x8C, 0xDA, 0x89, 0xE8, 0x3D, 0x00,
    0x10, 0x76, 0x03, 0xB8, 0x00, 0x10, 0x29, 0xC5,
    0x29, 0xC2, 0x29, 0xC3, 0x8E, 0xDA, 0x8E, 0xC3,
    0xB1, 0x03, 0xD3, 0xE0, 0x89, 0xC1, 0xD1, 0xE0,
    0x48, 0x48, 0x8B, 0xF0, 0x8B, 0xF8, 0xF3, 0xA5,
    0x09, 0xED, 0x75, 0xD8, 0xFC, 0x8E, 0xC2, 0x8E,
    0xDB, 0x31, 0xF6, 0x31, 0xFF, 0xBA, 0x10, 0x00,
    0xAD, 0x89, 0xC5, 0xD1, 0xED, 0x4A, 0x75, 0x05,
    0xAD, 0x89, 0xC5, 0xB2, 0x10, 0x73, 0x03, 0xA4,
    0xEB, 0xF1, 0x31, 0xC9, 0xD1, 0xED, 0x4A, 0x75,
    0x05, 0xAD, 0x89, 0xC5, 0xB2, 0x10, 0x72, 0x22,
    0xD1, 0xED, 0x4A, 0x75, 0x05, 0xAD, 0x89, 0xC5,
    0xB2, 0x10, 0xD1, 0xD1, 0xD1, 0xED, 0x4A, 0x75,
    0x05, 0xAD, 0x89, 0xC5, 0xB2, 0x10, 0xD1, 0xD1,
    0x41, 0x41, 0xAC, 0xB7, 0xFF, 0x8A, 0xD8, 0xE9,
    0x13, 0x00, 0xAD, 0x8B, 0xD8, 0xB1, 0x03, 0xD2,
    0xEF, 0x80, 0xCF, 0xE0, 0x80, 0xE4, 0x07, 0x74,
    0x0C, 0x88, 0xE1, 0x41, 0x41, 0x26, 0x8A, 0x01,
    0xAA, 0xE2, 0xFA, 0xEB, 0xA6, 0xAC, 0x08, 0xC0,
    0x74, 0x34, 0x3C, 0x01, 0x74, 0x05, 0x88, 0xC1,
    0x41, 0xEB, 0xEA, 0x89, 0xFB, 0x83, 0xE7, 0x0F,
    0x81, 0xC7, 0x00, 0x20, 0xB1, 0x04, 0xD3, 0xEB,
    0x8C, 0xC0, 0x01, 0xD8, 0x2D, 0x00, 0x02, 0x8E,
    0xC0, 0x89, 0xF3, 0x83, 0xE6, 0x0F, 0xD3, 0xEB,
    0x8C, 0xD8, 0x01, 0xD8, 0x8E, 0xD8, 0xE9, 0x72
}, sigbuf [sizeof sig90];

/* EXE header test (is it LZEXE file?) */
int rdhead(FILE *ifile ,int *ver){
    long entry; 	/* v0.8 */
/* v0.7 old code */
/*  if(fread(ihead,sizeof ihead[0],0x10,ifile)!=0x10)
 *      return FAILURE;
 *  memcpy(ohead,ihead,sizeof ihead[0] * 0x10);
 *  if(ihead[0]!=0x5a4d || ihead[4]!=2 || ihead[0x0d]!=0)
 *      return FAILURE;
 *  if(ihead[0x0c]==0x1c && memcmp(&ihead[0x0e],"LZ09",4)==0){
 *      *ver=90; return SUCCESS ;
 *  }
 *  if(ihead[0x0c]==0x1c && memcmp(&ihead[0x0e],"LZ91",4)==0){
 *      *ver=91; return SUCCESS ;
 *  }
 */
    if (fread (ihead, 1, sizeof ihead, ifile) != sizeof ihead)	     /* v0.8 */
	return FAILURE; 					     /* v0.8 */
    memcpy (ohead, ihead, sizeof ohead);			     /* v0.8 */
    if((ihead [0] != 0x5a4d && ihead [0] != 0x4d5a) ||		     /* v0.8 */
       ihead [0x0d] != 0 || ihead [0x0c] != 0x1c)		     /* v0.8 */
	return FAILURE; 					     /* v0.8 */
    entry = ((long) (ihead [4] + ihead[0x0b]) << 4) + ihead[0x0a];   /* v0.8 */
    if (fseek (ifile, entry, SEEK_SET) != 0)			     /* v0.8 */
	return FAILURE; 					     /* v0.8 */
    if (fread (sigbuf, 1, sizeof sigbuf, ifile) != sizeof sigbuf)    /* v0.8 */
	return FAILURE; 					     /* v0.8 */
    if (memcmp (sigbuf, sig90, sizeof sigbuf) == 0) {		     /* v0.8 */
	*ver = 90;						     /* v0.8 */
	return SUCCESS; 					     /* v0.8 */
    }								     /* v0.8 */
    if (memcmp (sigbuf, sig91, sizeof sigbuf) == 0) {		     /* v0.8 */
	*ver = 91;						     /* v0.8 */
	return SUCCESS; 					     /* v0.8 */
    }								     /* v0.8 */
    return FAILURE;
}

/* make relocation table */
int mkreltbl(FILE *ifile,FILE *ofile,int ver) {
    long fpos;
    int i;

/* v0.7 old code
 *  allocsize=((ihead[1]+16-1)>>4) + ((ihead[2]-1)<<5) - ihead[4] + ihead[5];
 */
    fpos=(long)(ihead[0x0b]+ihead[4])<<4;		/* goto CS:0000 */
    fseek(ifile,fpos,SEEK_SET);
    fread(inf, sizeof inf[0], 0x08, ifile);
    ohead[0x0a]=inf[0]; 	/* IP */
    ohead[0x0b]=inf[1]; 	/* CS */
    ohead[0x08]=inf[2]; 	/* SP */
    ohead[0x07]=inf[3]; 	/* SS */
    /* inf[4]:size of compressed load module (PARAGRAPH)*/
    /* inf[5]:increase of load module size (PARAGRAPH)*/
    /* inf[6]:size of decompressor with  compressed relocation table (BYTE) */
    /* inf[7]:check sum of decompresser with compressd relocation table(Ver.0.90) */
    ohead[0x0c]=0x1c;		/* start position of relocation table */
    fseek(ofile,0x1cL,SEEK_SET);
    switch(ver){
    case 90: i=reloc90(ifile,ofile,fpos);
             break;
    case 91: i=reloc91(ifile,ofile,fpos);
             break;
    default: i=FAILURE; break;
    }
    if(i!=SUCCESS){
        printf("error at relocation table.\n");
        return (FAILURE);
    }
    fpos=ftell(ofile);
/* v0.7 old code
 *  i= (int) fpos & 0x1ff;
 *  if(i) i=0x200-i;
 *  ohead[4]= (int) (fpos+i)>>4;
 */
    i= (0x200 - (int) fpos) & 0x1ff;	/* v0.7 */
    ohead[4]= (WORD) ((fpos+i)>>4);	/* v0.7 */

    for( ; i>0; i--)
        putc(0, ofile);
    return(SUCCESS);
}
/* for LZEXE ver 0.90 */
int reloc90(FILE *ifile,FILE *ofile,long fpos) {
    unsigned int c;
    WORD rel_count=0;
    WORD rel_seg,rel_off;

    fseek(ifile,fpos+0x19d,SEEK_SET);
    				/* 0x19d=compressed relocation table address */
    rel_seg=0;
    do{
        if(feof(ifile) || ferror(ifile) || ferror(ofile)) return(FAILURE);
        fread (&c, 2, 1, ifile);            /* v0.9 */
        for(;c>0;c--) {
            fread (&rel_off, 2, 1, ifile);  /* v0.9 */
            fwrite (&rel_off, 2, 1, ofile); /* v0.9 */
            fwrite (&rel_seg, 2, 1, ofile); /* v0.9 */
            rel_count++;
        }
        rel_seg += 0x1000;
    } while(rel_seg!=(0xf000+0x1000));
    ohead[3]=rel_count;
    return(SUCCESS);
}
/* for LZEXE ver 0.91*/
int reloc91(FILE *ifile,FILE *ofile,long fpos) {
    WORD span;
    WORD rel_count=0;
    WORD rel_seg,rel_off;

    fseek(ifile,fpos+0x158,SEEK_SET);
                                /* 0x158=compressed relocation table address */
    rel_off=0; rel_seg=0;
    for(;;) {
        if (feof(ifile) || ferror(ifile) || ferror(ofile)) return(FAILURE);
        if((span=(BYTE)getc(ifile))==0) { /* v0.9 */
            fread(&span, 2, 1, ifile);    /* v0.9 */
            if(span==0){
                rel_seg += 0x0fff;
                continue;
            } else if(span==1){
                break;
            }
        }
        rel_off += span;
        rel_seg += (rel_off & ~0x0f)>>4;
        rel_off &= 0x0f;
        fwrite(&rel_off, 2, 1, ofile);   /* v0.9 */
        fwrite(&rel_seg, 2, 1, ofile);   /* v0.9 */
        rel_count++;
    }
    ohead[3]=rel_count;
    return(SUCCESS);
}

/*---------------------*/
typedef struct {
        FILE  *fp;
        WORD  buf;
        BYTE  count;
    } bitstream;

void initbits(bitstream *,FILE *);
int getbit(bitstream *);

/*---------------------*/
/* decompressor routine */
int unpack(FILE *ifile,FILE *ofile){
    int len;
    WORD span;
    long fpos;
    bitstream bits;
    static BYTE data[0x4500], *p=data;

    fpos=((long)ihead[0x0b]-(long)inf[4]+(long)ihead[4])<<4;
    fseek(ifile,fpos,SEEK_SET);
    fpos=(long)ohead[4]<<4;
    fseek(ofile,fpos,SEEK_SET);
    initbits(&bits,ifile);
    printf(" unpacking. ");
    for(;;){
        if(ferror(ifile)) {printf("\nread error\n"); return(FAILURE); }
        if(ferror(ofile)) {printf("\nwrite error\n"); return(FAILURE); }
        if(p-data>0x4000){
            fwrite(data,sizeof data[0],0x2000,ofile);
            p-=0x2000;
            memmove(data,data+0x2000,p-data);  /* v0.9 */
            putchar('.');
        }
        if(getbit(&bits)) {
            *p++=(BYTE)getc(ifile);            /* v0.9 */
            continue;
        }
        if(!getbit(&bits)) {
            len=getbit(&bits)<<1;
            len |= getbit(&bits);
            len += 2;
            span=(BYTE)getc(ifile) | 0xff00;   /* v0.9 */
        } else {
            span=(BYTE)getc(ifile);
            len=(BYTE)getc(ifile);             /* v0.9 */
            span |= ((len & ~0x07)<<5) | 0xe000;
            len = (len & 0x07)+2;
            if (len==2) {
                len=(BYTE)getc(ifile);         /* v0.9 */

                if(len==0)
                    break;    /* end mark of compreesed load module */

                if(len==1)
                    continue; /* segment change */
                else
                    len++;
            }
        }
        for( ;len>0;len--,p++){
            *p=*(p+(int16_t)span);             /* v0.9 */
        }
    }
    if(p!=data)
        fwrite(data,sizeof data[0],p-data,ofile);
    loadsize=ftell(ofile)-fpos;
    printf("end\n");
    return(SUCCESS);
}

/* write EXE header*/
void wrhead(FILE *ofile) {
    if(ihead[6]!=0) {
        ohead[5]-= inf[5] + ((inf[6]+16-1)>>4) + 9;     /* v0.7 */
        if(ihead[6]!=0xffff)
            ohead[6]-=(ihead[5]-ohead[5]);
    }
    ohead[1]=((WORD)loadsize+(ohead[4]<<4)) & 0x1ff;    /* v0.7 */
    ohead[2]=(WORD)((loadsize+((long)ohead[4]<<4)+0x1ff) >> 9); /* v0.7 */
    fseek(ofile,0L,SEEK_SET);
    fwrite(ohead,sizeof ohead[0],0x0e,ofile);
}


/*-------------------------------------------*/

/* get compress information bit by bit */
void initbits(bitstream *p,FILE *filep){
    p->fp=filep;
    p->count=0x10;
    fread(&p->buf, 2, 1, p->fp);     /* v0.9 */
    /* printf("%04x ",p->buf); */
}

int getbit(bitstream *p) {
    int b;
    b = p->buf & 1;
    if(--p->count == 0){
        fread(&p->buf, 2, 1, p->fp); /* v0.9 */
        /* printf("%04x ",p->buf); */
        p->count= 0x10;
    }else
        p->buf >>= 1;

    return b;
}
