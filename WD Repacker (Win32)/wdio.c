#include "wdio.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define CHUNK 16384

int _tmp_err = 0;

int startInflate(z_stream* strm){
	strm->zalloc = Z_NULL;
	strm->zfree = Z_NULL;
	strm->opaque = Z_NULL;
	strm->avail_in = 0;
	strm->next_in = Z_NULL;
	return inflateInit(strm);
}
int startDeflate(z_stream* strm){
	strm->zalloc = Z_NULL;
	strm->zfree = Z_NULL;
	strm->opaque = Z_NULL;
	return deflateInit(strm, Z_DEFAULT_COMPRESSION);
}

char* readPstr(char *data, int *index, unsigned max){
	_tmp_err = 0;
	if(*index >= max){
		_tmp_err = WDERR_FORMAT_ERROR;
		return NULL;
	}
	unsigned char slen = data[*index];
	*index+=1;
	if((*index+slen) > max){
		_tmp_err = WDERR_FORMAT_ERROR;
		return NULL;
	}
	_set_errno(0);
	char *fullSTR = malloc(slen+1);
	if(fullSTR==NULL){
		_tmp_err = WDERR_MEMORY_ERROR;
		return NULL;
	}
	memcpy(fullSTR, &data[*index], slen);
	fullSTR[slen]=0;//ensure null terminated
	*index+=slen;
	return fullSTR;
}

int _readHeadFlg(unsigned char* data, char flags, int *coff, WD_Header* header, unsigned max){
	_tmp_err = 0;
	if(flags & WDFLAG_EXT_STR){
		header->ext_str = readPstr(data, coff, max);
		if(header->ext_str==NULL || _tmp_err!=0){
			return 1;//error and pass along error
		}
	}
	if(flags & WDFLAG_EXT_INT){
		if((*coff+4) > max){
			_tmp_err = WDERR_FORMAT_ERROR;
			return 1;
		}
		header->ext_int = *(int*)&data[*coff];
		*coff+=4;
	}
	if(flags & WDFLAG_GUID){
		if((*coff+16) > max){
			_tmp_err = WDERR_FORMAT_ERROR;
			return 1;
		}
		memcpy(&header->guid, &data[*coff], 16);
		*coff+=16;
	}
	return 0;
}

int readHeader(FILE* source, WD_Header* header){
	_tmp_err = 0;
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
	ret = startInflate(&strm);
	if (ret != Z_OK){
		_tmp_err = WDERR_COMPRESSION_ERROR;
		return 1;
	}
	//header should not exceed the size of a single chunk, so should not need to loop read it TODO check and ensure that is the case
	strm.avail_in = fread(in, 1, CHUNK, source);
	if(ferror(source) || strm.avail_in == 0){
		(void)inflateEnd(&strm);
		_tmp_err = WDERR_FILE_ERROR;
		return 1;
	}
	strm.next_in = in;
	
	strm.avail_out = CHUNK;
  strm.next_out = out;
	ret = inflate(&strm, Z_NO_FLUSH);
	if(ret != Z_STREAM_END){//header too big or compression error
		(void)inflateEnd(&strm);
		_tmp_err = WDERR_COMPRESSION_ERROR;
		return 1;
	}
	have = CHUNK - strm.avail_out;
	//seek back to end of compressed section
	if(fseek(source, -strm.avail_in, SEEK_CUR)){
		(void)inflateEnd(&strm);
		_tmp_err = WDERR_FILE_ERROR;
		return 1;
	}
	(void)inflateEnd(&strm);
	if(have < 4
	|| out[0]!=0xFF //static header bytes
	|| out[1]!=0xA1 // FF A1 D0
	|| out[2]!=0xD0){
		_tmp_err = WDERR_FORMAT_ERROR;
		return 1;//Error
	}
	char flags = out[3];//byte 4 = FLAGS
	header->flags = flags;
	int coff = 4;
	return _readHeadFlg(out, flags, &coff, header, have);
}
int writeHeader(FILE* destination, WD_Header* header){
	_tmp_err = 0;
	int fullheadLen = 4;//base+flags
	if(header->flags & WDFLAG_EXT_STR)fullheadLen+=1+strlen(header->ext_str);
	if(header->flags & WDFLAG_EXT_INT)fullheadLen+=4;
	if(header->flags & WDFLAG_GUID)fullheadLen+=16;
	char *fullHeader = malloc(fullheadLen);
	if(fullHeader==NULL){
		_tmp_err = WDERR_MEMORY_ERROR;
		return 1;
	}
	fullHeader[0]=0xFF;
	fullHeader[1]=0xA1;
	fullHeader[2]=0xD0;
	fullHeader[3]=header->flags;
	int coff = 4;
	if(header->flags & WDFLAG_EXT_STR){
		unsigned char tlen = (unsigned char)strlen(header->ext_str);
		fullHeader[coff] = tlen;
		coff+=1;
		memcpy(&fullHeader[coff], header->ext_str, tlen);
		coff+=tlen;
	}
	if(header->flags & WDFLAG_EXT_INT){
		memcpy(&fullHeader[coff], &header->ext_int, 4);
		coff+=4;
	}
	if(header->flags & WDFLAG_GUID){
		memcpy(&fullHeader[coff], &header->guid, 16);
		coff+=16;
	}
	char *comphead = malloc(fullheadLen*2);
	int ret;
	unsigned have;
	z_stream strm;
	ret = startDeflate(&strm);
	if(ret != Z_OK){
		_tmp_err = WDERR_COMPRESSION_ERROR;
		free(comphead);
		free(fullHeader);
		return 1;
	}
	strm.avail_in = fullheadLen;
	strm.next_in = fullHeader;
	strm.avail_out = fullheadLen*2;
  strm.next_out = comphead;
	ret = deflate(&strm, Z_FINISH);
	if(ret < 0){
		_tmp_err = WDERR_COMPRESSION_ERROR;
		(void)deflateEnd(&strm);
		free(comphead);
		free(fullHeader);
		return 1;
	}
	have = fullheadLen*2 - strm.avail_out;
	if(fwrite(comphead, 1, have, destination) == 0 || ferror(destination)){
		_tmp_err = WDERR_FILE_ERROR;
		(void)deflateEnd(&strm);
		free(comphead);
		free(fullHeader);
		return 1;
	}
	(void)deflateEnd(&strm);
	free(comphead);
	free(fullHeader);
	return 0;
}

int readDirectory(WD_File_Descriptor* parent){
	FILE* source = parent->handle;
	if(fseek(source, -4, SEEK_END)){
		_tmp_err = WDERR_FILE_ERROR;
		return 1;
	}
	long int dirlen = 0;
	if(fread(&dirlen, 4, 1, source) != 1 || ferror(source)){
		_tmp_err = WDERR_FILE_ERROR;
		return 2;
	}
	if(fseek(source, -dirlen, SEEK_END)){
		_tmp_err = WDERR_FILE_ERROR;
		return 3;
	}
	dirlen-=4;
	if(dirlen<0){
		_tmp_err = WDERR_FORMAT_ERROR;
		return 4;
	}
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
	unsigned char *tmp = malloc(10);
	if(tmp==NULL){
		_tmp_err = WDERR_MEMORY_ERROR;
		return 5;
	}
	unsigned tlen = 0;
	ret = startInflate(&strm);
	if(ret != Z_OK){
		_tmp_err = WDERR_COMPRESSION_ERROR;
		free(tmp);
		return 6;
	}
	do{
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			_tmp_err = WDERR_FILE_ERROR;
			(void)inflateEnd(&strm);
			free(tmp);
			return 7;
		}
		if (strm.avail_in == 0)break;
		strm.next_in = in;
		do{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			switch (ret) {
				case Z_NEED_DICT:
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					_tmp_err = WDERR_COMPRESSION_ERROR;
					(void)inflateEnd(&strm);
					free(tmp);
					return 8;
			}
			have = CHUNK - strm.avail_out;
			int ns = tlen+have;
			void *oldtmp = tmp;//realloc returns null and does not free old on error
			tmp = realloc(tmp, ns);
			if(tmp==NULL){
				_tmp_err = WDERR_MEMORY_ERROR;
				(void)inflateEnd(&strm);
				free(oldtmp);
				return 9;
			}
			memcpy(&tmp[tlen], out, have);
			tlen = ns;
			//current method: decompress whole directory before reading
		}while(strm.avail_out == 0);
	}while(ret != Z_STREAM_END);
	(void)inflateEnd(&strm);
	if(ret != Z_STREAM_END){
		_tmp_err = WDERR_COMPRESSION_ERROR;
		free(tmp);
		return 10;
	}
	int roff = 0;
	if((roff+sizeof(FILETIME))>tlen){
		_tmp_err = WDERR_FORMAT_ERROR;
		free(tmp);
		return 11;
	}
	memcpy(&parent->timestamp, tmp, sizeof(FILETIME));
	roff+=sizeof(FILETIME);
	if((roff+2)>tlen){
		_tmp_err = WDERR_FORMAT_ERROR;
		free(tmp);
		return 12;
	}
	unsigned short filecount = *(unsigned short*)&tmp[roff];
	roff+=2;
	parent->dir = calloc(filecount, sizeof(WD_Directory_Entry));
	if(parent->dir==NULL){
		_tmp_err = WDERR_MEMORY_ERROR;
		free(tmp);
		return 13;
	}
	parent->dirlen = filecount;
	for(int i = 0; i<filecount; i++){
		WD_Directory_Entry *ent = &parent->dir[i];
		ent->source_path = NULL;
		ent->path = readPstr(tmp, &roff, tlen);
		if(ent->path==NULL || _tmp_err!=0){
			free(tmp);
			return 14;
		}
		
		if((roff+1)>tlen){
			_tmp_err = WDERR_FORMAT_ERROR;
			free(tmp);
			return 15;
		}
		char flags = tmp[roff];
		roff+=1;
		ent->header.flags = flags;
		
		if((roff+4)>tlen){
			_tmp_err = WDERR_FORMAT_ERROR;
			free(tmp);
			return 16;
		}
		ent->offset = *(unsigned int*)&tmp[roff];
		roff+=4;
		
		if((roff+4)>tlen){
			_tmp_err = WDERR_FORMAT_ERROR;
			free(tmp);
			return 17;
		}
		ent->compressed_length = *(unsigned int*)&tmp[roff];
		roff+=4;
		
		if((roff+4)>tlen){
			_tmp_err = WDERR_FORMAT_ERROR;
			free(tmp);
			return 18;
		}
		ent->uncompressed_length = *(unsigned int*)&tmp[roff];
		roff+=4;
		if(_readHeadFlg(tmp, flags, &roff, &ent->header, tlen)){
			free(tmp);
			return 19;//_tmp_err already set, pass along
		}
	}
	free(tmp);
	return 0;
}
int writeDirectory(WD_File_Descriptor* parent){
	FILE* dest = parent->handle;
	long dirStart = ftell(dest);
	int reqlen = sizeof(FILETIME)+2;
	unsigned short trueDirlen = (unsigned short)parent->dirlen;
	for(int i = 0; i<parent->dirlen; i++){
		WD_Directory_Entry *ent = &parent->dir[i];
		if(ent->offset==0){//0 is an invalid offset, do not count this entry
			trueDirlen-=1;
			continue;
		}
		reqlen+=strlen(ent->path)+14;
		if(ent->header.flags & WDFLAG_EXT_STR)reqlen+=1+strlen(ent->header.ext_str);
		if(ent->header.flags & WDFLAG_EXT_INT)reqlen+=4;
		if(ent->header.flags & WDFLAG_GUID)reqlen+=16;
	}
	unsigned char* in = malloc(reqlen);
	if(in==NULL){
		_tmp_err = WDERR_MEMORY_ERROR;
		return 1;
	}
	int coff=0;
	memcpy(&in[coff], &parent->timestamp, sizeof(FILETIME));
	coff+=sizeof(FILETIME);
	memcpy(&in[coff], &trueDirlen, 2);
	coff+=2;
	for(int i = 0; i<parent->dirlen; i++){
		WD_Directory_Entry *ent = &parent->dir[i];
		if(ent->offset==0) continue;
		unsigned char plen = (unsigned char)strlen(ent->path);
		in[coff] = plen;
		coff+=1;
		memcpy(&in[coff], ent->path, plen);
		coff+=plen;
		in[coff]=ent->header.flags;
		coff+=1;
		memcpy(&in[coff], &ent->offset, 4);
		coff+=4;
		memcpy(&in[coff], &ent->compressed_length, 4);
		coff+=4;
		memcpy(&in[coff], &ent->uncompressed_length, 4);
		coff+=4;
		if(ent->header.flags & WDFLAG_EXT_STR){
			unsigned char tlen = (unsigned char)strlen(ent->header.ext_str);
			in[coff] = tlen;
			coff+=1;
			memcpy(&in[coff], ent->header.ext_str, tlen);
			coff+=tlen;
		}
		if(ent->header.flags & WDFLAG_EXT_INT){
			memcpy(&in[coff], &ent->header.ext_int, 4);
			coff+=4;
		}
		if(ent->header.flags & WDFLAG_GUID){
			memcpy(&in[coff], &ent->header.guid, 16);
			coff+=16;
		}
	}
	unsigned char* out = malloc(reqlen*2);//just in case it's bigger
	if(out==NULL){
		_tmp_err = WDERR_MEMORY_ERROR;
		free(in);
		return 1;
	}
	int ret;
	unsigned have;
	z_stream strm;
	ret = startDeflate(&strm);
	if(ret != Z_OK){
		_tmp_err = WDERR_COMPRESSION_ERROR;
		free(in);
		free(out);
		return 1;
	}
	strm.avail_in = reqlen;
	strm.next_in = in;
	strm.avail_out = reqlen*2;
  strm.next_out = out;
	ret = deflate(&strm, Z_FINISH);
	if(ret != Z_STREAM_END){
		_tmp_err = WDERR_COMPRESSION_ERROR;
		(void)deflateEnd(&strm);
		free(in);
		free(out);
		return 1;
	}
	have = reqlen*2 - strm.avail_out;
	
	if(fwrite(out, 1, have, dest) != have || ferror(dest)){
		_tmp_err = WDERR_FILE_ERROR;
		(void)deflateEnd(&strm);
		free(in);
		free(out);
		return 1;
	}
	(void)deflateEnd(&strm);
	long dirEnd = ftell(dest);
	unsigned int dirLen = (unsigned int)(dirEnd-dirStart+4);
	if(fwrite(&dirLen, 4, 1, dest) != 4 || ferror(dest)){
		_tmp_err = WDERR_FILE_ERROR;
		free(in);
		free(out);
		return 1;
	}
	free(in);
	free(out);
	return 0;
}

//opens existing wd file and reads the directory information.
int wd_openExisting(WD_File_Descriptor* out){
	_tmp_err = 0;
	int ret = 0;
	ret = readHeader(out->handle, &out->head);
	if(ret){
		return WDERR_HEADER_ERROR | _tmp_err;
	}
	//ensure header int is 0x02004457 ("WD",0x200) (version 0x200)
	int head = out->head.ext_int & 0xFFFF;
	int ver = (out->head.ext_int>>16) & 0xFFFF;
	if(head != 0x4457){
		return WDERR_HEADER_ERROR | WDERR_FORMAT_ERROR;
	}
	if(ver != 0x200){
		return WDERR_HEADER_ERROR | WDERR_UNSUPPORTED_VERSION;
	}
	ret = readDirectory(out); 
	if(ret){
		return WDERR_DIRECTORY_ERROR | _tmp_err;
	}
	return WDERR_OK;
}
//pipes the file out, with the compressed header if relevant or uncompressed otherwise.
int wd_pipeFileOut(FILE* destination, WD_File_Descriptor* source, WD_Directory_Entry* file){
	FILE* wdhand = source->handle;
	int exthead = file->header.flags & WDFLAG_ALLEXT;
	int filecomp = file->header.flags & WDFLAG_COMPRESSED;
	fseek(wdhand, file->offset, SEEK_SET);
	if(exthead){
		if(writeHeader(destination, &file->header)){
			return WDERR_HEADER_ERROR | _tmp_err;
		}
		filecomp = 0;//prevent decompression during transfer to copy as is
	}
	if(filecomp){
		unsigned int nbl = file->compressed_length;
		int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
		ret = startInflate(&strm);
		if(ret != Z_OK){
			return WDERR_COMPRESSION_ERROR;
		}
		do{
			unsigned int tr = nbl;
			if(tr>CHUNK)tr = CHUNK;
			strm.avail_in = fread(in, 1, tr, wdhand);
			if(ferror(wdhand)){
				(void)inflateEnd(&strm);
				return WDERR_FILE_ERROR;
			}
			nbl-=strm.avail_in;
			strm.next_in = in;
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				switch (ret) {
					case Z_NEED_DICT:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						return WDERR_COMPRESSION_ERROR;
				}
				have = CHUNK - strm.avail_out;
				if (fwrite(out, 1, have, destination) != have || ferror(destination)) {
					(void)inflateEnd(&strm);
					return WDERR_FILE_ERROR;
				}
			} while (strm.avail_out == 0);
		}while(ret != Z_STREAM_END && nbl>0);
		(void)inflateEnd(&strm);
	}else{//file is uncompressed pass through simple
		unsigned char buf[CHUNK];
		unsigned int nbl = file->compressed_length;
		while(nbl>0){
			unsigned int tr = nbl;
			if(tr>CHUNK)tr = CHUNK;
			int towrt = fread(buf, 1, tr, wdhand);
			if(ferror(wdhand)){
				return WDERR_FILE_ERROR;
			}
			if(fwrite(buf, 1, towrt, destination) !=towrt || ferror(destination)){
				return WDERR_FILE_ERROR;
			}
			nbl-=towrt;
		}
	}
	return WDERR_OK;
}
int _pipeFileInExisting(FILE* source, FILE* destination, WD_Directory_Entry* ent){
	long start = ftell(destination);
	ent->offset = (unsigned int)start;
	unsigned int uncomplen = 0;
	if(ent->header.flags & WDFLAG_COMPRESSED){
		int ret;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
		ret = startInflate(&strm);
		if(ret != Z_OK){
			return WDERR_COMPRESSION_ERROR;
		}
		do {
			strm.avail_in = fread(in, 1, CHUNK, source);
			if (ferror(source)) {
				(void)inflateEnd(&strm);
				return WDERR_FILE_ERROR;
			}
			fwrite(in, 1, strm.avail_in, destination);
			if (ferror(destination)) {
				(void)inflateEnd(&strm);
				return WDERR_FILE_ERROR;
			}
			if (strm.avail_in == 0)break;
			strm.next_in = in;
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				switch (ret) {
					case Z_NEED_DICT:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						return WDERR_COMPRESSION_ERROR;
				}
				uncomplen+= CHUNK - strm.avail_out;
			}while(strm.avail_out == 0);
		}while(ret != Z_STREAM_END);
		(void)inflateEnd(&strm);
	}else{
    unsigned char buf[CHUNK];
		do {
			int bwrt = fread(buf, 1, CHUNK, source);
			if (ferror(source)) {
				return WDERR_FILE_ERROR;
			}
			uncomplen+= fwrite(buf, 1, bwrt, destination);
			if (ferror(destination)) {
				return WDERR_FILE_ERROR;
			}
		}while(!feof(source));
	}
	ent->uncompressed_length = (unsigned int)uncomplen;
	long end = ftell(destination);
	ent->compressed_length = (unsigned int)(end-start);
	return WDERR_OK;
}
int _pipeFileInNew(FILE* source, FILE* destination, WD_Directory_Entry* ent){
	long start = ftell(destination);
	long source_start = ftell(source);//should be 0 in most cases
	ent->offset = (unsigned int)start;
	if(ent->header.flags & WDFLAG_COMPRESSED){
		int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
		ret = startDeflate(&strm);
		if(ret != Z_OK){
			return WDERR_COMPRESSION_ERROR;
		}
		do{
			strm.avail_in = fread(in, 1, CHUNK, source);
			if (ferror(source)) {
				(void)deflateEnd(&strm);
				return WDERR_FILE_ERROR;
			}
			flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
			strm.next_in = in;
			do{
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = deflate(&strm, flush);
				if(ret == Z_STREAM_ERROR){
					(void)deflateEnd(&strm);
					return WDERR_COMPRESSION_ERROR;
				}
				have = CHUNK - strm.avail_out;
				if (fwrite(out, 1, have, destination) != have || ferror(destination)) {
					(void)deflateEnd(&strm);
					return WDERR_FILE_ERROR;
				}
			}while(strm.avail_out == 0);
		}while(flush != Z_FINISH);
		(void)deflateEnd(&strm);
	}else{
		unsigned char buf[CHUNK];
		do {
			int bwrt = fread(buf, 1, CHUNK, source);
			if (ferror(source)) {
				return WDERR_FILE_ERROR;
			}
			fwrite(buf, 1, bwrt, destination);
			if (ferror(destination)) {
				return WDERR_FILE_ERROR;
			}
		}while(!feof(source));
	}
	long source_end = ftell(source);
	ent->uncompressed_length = (unsigned int)(source_end-source_start);
	long end = ftell(destination);
	ent->compressed_length = (unsigned int)(end-start);
	return WDERR_OK;
}
//does the full write operation, including reading in files and archiving them.
int wd_createNew(WD_File_Descriptor* fd, _ProgReport progressCallback){
	int error_count = 0;
	FILE* destination = fd->handle;
	int ret = writeHeader(destination, &fd->head);
	if(ret){
		return WDERR_HEADER_ERROR | ret;
	}
	for(int i = 0; i<fd->dirlen; i++){
		WD_Directory_Entry *ent = &fd->dir[i];
		if(ent->source_path){
			if(progressCallback!=NULL && progressCallback(ent->path))return WDERR_OK;//Cancelled
			FILE* fhand = fopen(ent->source_path, "rb");
			if(fhand==NULL){
				error_count+=1;
				ent->offset=0;//null is an invalid offset due to header
				continue;
			}
			if(wd_parseFileHeader(fhand, &ent->header)){
				if(_pipeFileInNew(fhand, destination, ent)){
					error_count+=1; ent->offset=0;
				}
			}else{
				if(_pipeFileInExisting(fhand, destination, ent)){
					error_count+=1; ent->offset=0;
				}
			}
			fclose(fhand);
		}
	}
	ret = writeDirectory(fd);
	if(ret){
		return WDERR_DIRECTORY_ERROR | ret;
	}
	if(error_count){
		return -error_count;//return negative of files with errors
	}
	return WDERR_OK;
}
int wd_createDefaultWDHeader(WD_Header* header){
	header->flags = WDFLAG_DEFAULT;
	header->ext_int = 0x02004457; //"WD" version 0x200
	for(int i = 0; i<16; i++){//assign random guid
		header->guid[i] = (unsigned char)(rand()&0xFF);
	}
	return WDERR_OK;
}
//parseFileHeader checks if file has header and copies flags and extras from it if present.
int wd_parseFileHeader(FILE* source, WD_Header* header){
	unsigned char testHead[2];
	long int retloc = ftell(source);
	int res = fread(testHead, 1, 2, source);
	fseek(source, retloc, SEEK_SET);
	if(ferror(source) || res != 2){
		return WDERR_FORMAT_ERROR;
	}
	if(testHead[0] == 0x78 && testHead[1] == 0x9c){
		int ret = readHeader(source, header);
		if(ret!=WDERR_OK){
			fseek(source, retloc, SEEK_SET);
			return ret;
		}
		return WDERR_OK;
	}
	return WDERR_FORMAT_ERROR;//does not have header, set yourself (default to compression on, unless specific file extension?)
}

int wd_Free(WD_File_Descriptor* fd){
	if(fd->handle){
		fclose(fd->handle);
		fd->handle = 0;
	}
	if(fd->head.ext_str)free(fd->head.ext_str);
	if(fd->dir){
		for(int i = 0; i<fd->dirlen; i++){
			WD_Directory_Entry *ent = &fd->dir[i];
			if(ent->source_path)free(ent->source_path);
			if(ent->path)free(ent->path);
			if(ent->header.ext_str)free(ent->header.ext_str);
		}
		fd->dirlen = 0;
		free(fd->dir);
		fd->dir = NULL;
	}
	return 0;
}
