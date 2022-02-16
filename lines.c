#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdarg.h>

#ifndef COLS
unsigned short COLS;
#endif
#ifndef LINES
unsigned short LINES;
#endif

#define SB_GL 0
#define SB_LL 1
#define SB_GS 2
#define SB_LS 3
#define SB_NS 256

#ifndef u64
#define u64 unsigned long long
#endif
#define ERROR -1

#define min(e1, e2) (e1 < e2 ? e1 : e2)

bool verbose = 0;
bool fosymls = 0;
bool allfiles = 0;
bool zerldis = 0;

void swap(void* p1, void* p2, size_t s){
  for(int i = 0; i < s; i++){
	char t = *((char*)p1+i);								
	*((char*)p1+i) = *((char*)p2+i);
	*((char*)p2+i) = t;
  }
}

#define BS_UP 0
#define BS_DOWN 1

void bubble_sort(void* arr, size_t block,
				 size_t size, bool direct){
  for(size_t i = 0; i < size; i++){
	for(size_t j = 0; j < size-i-1; j++){
	  u64 exp0 = 0, exp1 = 0;
	  for(size_t k = 0; k < block; k++){
		exp0 |= *((unsigned char*)arr+j*block+k) << (k*8);
		exp1 |= *((unsigned char*)arr+(j+1)*block+k) << (k*8);
	  }
	  if(direct ? exp0 < exp1 : exp0 > exp1){
		swap((unsigned char*)arr+j*block,
			 (unsigned char*)arr+(j+1)*block, block);
	  }
	}
	if(verbose) printf("\x1b[1A%ld/%ld elements sorted\n", i, size);
  }
}

int bubble_sort_struct(void* struct_arr, size_t struct_size, size_t offset,
						size_t elemsize, size_t size, int8_t direct){
  if(offset == ERROR){
	fprintf(stderr, "ERROR: Unknown sort element\n");
	return -1;
  }
  if(direct == ERROR){
	fprintf(stderr, "ERROR: Unknown direction for sort\n");
	return -1;
  }
  for(size_t i = 0; i < size; i++){
	for(size_t j = 0; j < size-i-1; j++){
	  u64 exp0 = 0, exp1 = 0;
	  for(size_t k = 0; k < elemsize; k++){
		exp0 |= *((unsigned char*)struct_arr+offset+k+j*struct_size) << (k*8);
		exp1 |= *((unsigned char*)struct_arr+offset+k+(j+1)*struct_size) << (k*8);
	  }
	  if(direct ? exp0 > exp1 : exp0 < exp1){
		swap((unsigned char*)struct_arr+j*struct_size,
			 (unsigned char*)struct_arr+(j+1)*struct_size, struct_size);
	  }
	}
	if(verbose){
	  printf("\x1b[G%ld/%ld elements sorted\x1b[K", i+1, size);
	  fflush(stdout);
	}
  }
  if(verbose) printf("\n");
  return 0;
}

u64 count_lines_from_stream(FILE *f, bool l){
  u64 lines = 0;
  int byte = 0;
  while(!feof(f) && (byte = fgetc(f)) > 0){
	if(l) putc(byte, stdout);
	if(byte == 10){
	  lines++;
	}
  }
  fclose(f);
  return lines;
}

u64 count_lines_from_file(const char* file_path){
  FILE* f = fopen(file_path, "r");
  if(f == NULL){
	fprintf(stderr, "\x1b[GERROR: Could not open the file `%s': %s\x1b[K\n",
			file_path, strerror(errno));
	return 0;
  }else if(feof(f)){
	fprintf(stderr, "\x1b[GERROR: Could not read the file `%s': %s\x1b[K\n",
			file_path, strerror(errno));
	return 0;
  }
  return count_lines_from_stream(f, 0);
}

struct direlem{
  int type;
  off_t size;
  char* fullpath;
  u64 lines;
};

char* fullpath(const char* dirname, const char filename[256]){
  size_t filename_l = strlen(filename);
  size_t dirname_l = strlen(dirname);
  uint8_t off = 0;
  if(dirname[dirname_l-1] != '/'){
	off++;
  }
  char* path = malloc(dirname_l+filename_l+1+off);
  if(off){
	path[dirname_l] = '/';
  }
  memcpy(path, dirname, dirname_l);
  memcpy(path+dirname_l+off, filename, filename_l);
  path[dirname_l+filename_l+off] = 0;
  return path;
}

struct direlem* recursevly_list_files_in_directory(const char* dirname, size_t* count){
  DIR* dir = opendir(dirname);
  if(dir == NULL){
	fprintf(stderr, "\x1b[GERROR: Could not open directory `%s': %s\x1b[K\n",
			dirname, strerror(errno));
	return NULL;
  }
  struct dirent* d = {0};
  struct stat st = {0};
  struct direlem* rv = NULL;
  size_t cnt = 0;
  while((d = readdir(dir)) != NULL){
	if(strcmp(d->d_name, ".") != 0 &&
	   strcmp(d->d_name, "..") != 0 &&
	   (allfiles ? 1 : d->d_name[0] != '.')
	   ){
	  char* path = fullpath(dirname, d->d_name);
	  if(fosymls) stat(path, &st);
	  else lstat(path, &st);
	  if(S_ISREG(st.st_mode)){
		if(verbose){
		  if(strlen(path) > COLS-16){
			printf("\x1b[GCurrent file: `%.*s...%.*s'\x1b[K", (COLS-16)/2, path, (COLS-16)/2-3, path+strlen(path)-(COLS-16)/2+3);
		  }else{
			printf("\x1b[GCurrent file: `%s'\x1b[K", path);
		  }
		  fflush(stdout);
		}
		u64 lines = count_lines_from_file(path);
		if((lines > 0) || !zerldis){
		  rv = realloc(rv, (cnt+1)*sizeof(struct direlem));
		  rv[cnt] = (struct direlem){
			.type = (st.st_mode & S_IFMT),
			.size = st.st_size,
			.fullpath = path,
			.lines = lines
		  };
		  cnt++;
		}else{
		  free(path);
		}
	  }else if(S_ISDIR(st.st_mode)){
		size_t cnt_ndir = 0;
		struct direlem* nextdir = recursevly_list_files_in_directory(path, &cnt_ndir);
		if(nextdir != NULL){
		  rv = realloc(rv, (cnt+cnt_ndir)*sizeof(struct direlem));
		  memcpy(rv+cnt, nextdir, (cnt_ndir)*sizeof(struct direlem));
		  cnt += cnt_ndir;
		  free(nextdir);
		}
		free(path);
	  }else{
		free(path);
	  }
	}
  }
  closedir(dir);
  if(count != NULL){
	*count = cnt;
  }
  return rv;
}

void parseargs(char* arg, char* fmt, ...){
  if(arg[0] == '-'){
	va_list va;
	va_start(va, fmt);
	bool *vals[strlen(fmt)];
	for(int i = 0; i < strlen(fmt); i++)
	  vals[i] = va_arg(va, bool*);
	for(int i = 0; i < strlen(fmt); i++){
	  for(int j = 0; j < min(strlen(fmt), strlen(arg)); j++){
		if(arg[j+1] == fmt[i]){
		  *vals[i] = 1;
		}
	  }
	}
	va_end(va);
  }
}

void usage(FILE* output_stream, const char* progname,
		   int eval){
  fprintf(output_stream, "Usage: %s [flags] [filenames] [dirnames]\n",
		  progname);
  fprintf(output_stream, "      or <cmd> | %s\n",
		  progname);
  fprintf(output_stream, "Flags:\n");
  fprintf(output_stream, "    -v     Verbose mode\n");
  fprintf(output_stream, "    -L     Follow all symlinks\n");
  fprintf(output_stream, "    -a     Print all files\n");
  fprintf(output_stream, "    -N     Don't print 0 lines files\n");
  fprintf(output_stream, "    -gl    Sort by more lines(default) in file\n");
  fprintf(output_stream, "    -ll    Sort by less lines in file\n");
  fprintf(output_stream, "    -gs    Sort by more size of file\n");
  fprintf(output_stream, "    -ls    Sort by less size of file\n");
  fprintf(output_stream, "    -ns    Print without sort\n");
  fprintf(output_stream, "    -h     Shows this message\n");
  exit(eval);
}

int main(int argc, char* argv[]){
  struct winsize winsz;
  ioctl(0, TIOCGWINSZ, &winsz);
  COLS = winsz.ws_col;
  LINES = winsz.ws_row;
  
  int sortby = SB_GL;
  
  u64 total_lines = 0;
  size_t i = 0;
  struct stat st = {0};
  struct direlem* d = {0};
  if(argc > 1){
	for(int j = 1; j < argc; j++){
	  parseargs(argv[j], "vLaN", &verbose, &fosymls, &allfiles,
				&zerldis);
	  if(strcmp(argv[j], "-gl") == 0) sortby = SB_GL;
	  else if(strcmp(argv[j], "-ll") == 0) sortby = SB_LL;
	  else if(strcmp(argv[j], "-gs") == 0) sortby = SB_GS;
	  else if(strcmp(argv[j], "-ls") == 0) sortby = SB_LS;
	  else if(strcmp(argv[j], "-ns") == 0) sortby = SB_NS;
	  else if(strcmp(argv[j], "-h") == 0) usage(stdout, argv[0], 0);
	  else{
		stat(argv[j], &st);
		if(S_ISDIR(st.st_mode)){
		  size_t _i = 0;
		  struct direlem* nd = recursevly_list_files_in_directory(argv[j], &_i);
		  d = realloc(d, (_i+i)*sizeof(struct direlem));
		  memcpy(d+i, nd, _i*sizeof(struct direlem));
		  printf("\x1b[G%ld elements in directory `%s'\x1b[K\n", _i, argv[j]);
		  free(nd);
		  for(size_t k = _i+i; k-- > i;){
			total_lines += d[k].lines;
		  }
		  i += _i;
		}else if(S_ISREG(st.st_mode)){
		  if(verbose){
			printf("\x1b[GCurrent file: `%s'\x1b[K", argv[j]);
			fflush(stdout);
		  }
		  d = realloc(d, (i+1)*sizeof(struct direlem));
		  d[i].lines = count_lines_from_file(argv[j]);
		  d[i].fullpath = malloc(strlen(argv[j])+1);
		  d[i].size = st.st_size;
		  memcpy(d[i].fullpath, argv[j], strlen(argv[j])+1);
		  total_lines += d[i].lines;
		  i++;
		}
	  }
	}
	if(sortby != SB_NS){
	  printf("\x1b[GSorting %ld elements..\x1b[K\n", i);
	  if(bubble_sort_struct(d, sizeof(struct direlem),
							(sortby == SB_LL || sortby == SB_GL ? sizeof(struct direlem)-8 :
							 sortby == SB_GS || sortby == SB_LS ? sizeof(struct direlem)-24 :
							 ERROR), 8, i,
							(sortby == SB_GS || sortby == SB_GL ? BS_UP :
							 (sortby == SB_LS || sortby == SB_LL ? BS_DOWN :
							  ERROR))) != 0
		 ){
		fprintf(stderr, "ERROR: Failed to sort\n");
	  }
	}
	while(i--){
	  printf("%lld lines in file `%s'\n",
			 d[i].lines, d[i].fullpath);
	  free(d[i].fullpath);
	}
	free(d);
  }else{
	total_lines = count_lines_from_stream(stdin, 1);
  }
  printf("%lld lines total\n", total_lines);
  return 0;
}
