#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <png.h>

#ifndef PROGNAME
#define PROGNAME "generate_font"
#endif /* #ifndef PROGNAME */

struct letter{
	int begin; /* Begin in the byte stream */
	int length; /* Length (in bytes) of the PNG data in the byte stream */
	int code; /* Unicode code of the character */
	int xsize;
	int ysize;
	void *foo;
	void *bar;
};

struct font{
	unsigned char *family;
	unsigned char *weight;
	unsigned char *slant;
	unsigned char *adstyl;
	unsigned char *spacing;
	int n_letters; /* Number of letters in this font */
	struct letter *letters; 
};

struct font *fonts;
int n_fonts;
int n_letters;
int stamp;

int file_select(const struct dirent * entry)
{

	char *s=entry->d_name;
	FILE *f;

	if (strlen(s)!=8) return 0;
	if (!((s[0]>='0'&&s[0]<='9')||(s[0]>='a'&&s[0]<='f'))) return 0;
	if (!((s[1]>='0'&&s[1]<='9')||(s[1]>='a'&&s[1]<='f'))) return 0;
	if (!((s[2]>='0'&&s[2]<='9')||(s[2]>='a'&&s[2]<='f'))) return 0;
	if (!((s[3]>='0'&&s[3]<='9')||(s[3]>='a'&&s[3]<='f'))) return 0;
	if (s[4]!='.'||s[5]!='p'||s[6]!='n'||s[7]!='g') return 0;
	f=fopen(s,"r");
	if (!f) return 0; /* If it can't be open as a file then it's probably
		           * a directory
			   */ 
	fclose(f);
	return 1;
}

/* If the directory name has bad structure (must have 4 dashes in the name),
 * then nothing is done
 * Fills weight, slant, adstyl, spacing with appropriate strings
 * Returns 1 if the input should be skipped.
 */
int parse_font_name(unsigned char *input, unsigned char **family,
	unsigned char **weight, unsigned char ** slant, unsigned char ** adstyl,
	unsigned char ** spacing)
{
	int dashes=0;
	unsigned char *p, *r;
	FILE *f;
	unsigned char alias[256];

	p=input;
	while(*p){
		if (*p=='-') dashes++;
		p++;
	}
	if (dashes!=4) return 1; /* Invalid name structure -- ignore this entry */
	if (chdir(input)){
		/* Is a directory and has appropriate name, but can't
		 * change into.
		 */
		fprintf(stderr,"%s: can't change into directory %s.\n",
				PROGNAME, input);
		perror(PROGNAME);
		exit(1);
	}
	p=input;
	r=p;
	while (*r!='-') r++;
	*family=malloc(r-p+1);
	if (!*family){
		fprintf(stderr,"Out of memory.\n");
		exit(1);
	}
	memcpy(*family,p,r-p);
	(*family)[r-p]=0;
	r++;
	p=r;
	while (*r!='-') r++;
	*weight=malloc(r-p+1);
	if (!*weight){
		fprintf(stderr,"Out of memory.\n");
		exit(1);
	}
	memcpy(*weight,p,r-p);
	(*weight)[r-p]=0;
	r++;
	p=r;
	while (*r!='-') r++;
	*slant=malloc(r-p+1);
	if (!*slant){
		fprintf(stderr,"Out of memory.\n");
		exit(1);
	}
	memcpy(*slant,p,r-p);
	(*slant)[r-p]=0;
	r++;
	p=r;
	while (*r!='-') r++;
	*adstyl=malloc(r-p+1);
	if (!*adstyl){
		fprintf(stderr,"Out of memory.\n");
		exit(1);
	}
	memcpy(*adstyl,p,r-p);
	(*adstyl)[r-p]=0;
	r++;
	p=r;
	while (*r) r++;
	*spacing=malloc(r-p+1);
	if (!*spacing){
		fprintf(stderr,"Out of memory.\n");
		exit(1);
	}
	memcpy(*spacing,p,r-p);
	(*spacing)[r-p]=0;
	/* Now let's append the aliases file contents */
	f=fopen("aliases","r");
	if (f){
		while(fgets(alias,sizeof(alias),f)){
			int length=strlen(alias);

			while(length>0&&alias[length-1]=='\n')length--;
			while(length>0&&alias[length-1]==' ')length--;
			while(length>0&&alias[length-1]=='\t')length--;
			alias[length]=0;
			if (!length) continue;
			*family=realloc(*family,strlen(*family)+2+strlen(alias));
			strcpy(*family+strlen(*family),"-");
			strcpy(*family+strlen(*family),alias);
		}
		fclose(f);
	}
	return 0;
}

/* Doesn't free the strings, just pushes them into the global structure */
void add_font_name(unsigned char *family, unsigned char *weight,unsigned char
*slant, unsigned char *adstyl, unsigned char *spacing)
{
	struct font *f;

	n_fonts++;
	fonts=realloc(fonts,n_fonts*sizeof(*fonts));
	if (!fonts){
		fprintf(stderr,"%s: Out of memory\n",PROGNAME);
		exit(1);
	}
	f=fonts+n_fonts-1;
	f->family=family;
	f->weight=weight;
	f->slant=slant;
	f->adstyl=adstyl;
	f->spacing=spacing;
}

void get_png_dimensions(int *x, int *y, FILE * stream)
{
	png_structp png_ptr;
	png_infop info_ptr;

	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
	NULL);
	info_ptr=png_create_info_struct(png_ptr);
	png_init_io(png_ptr,stream);
	png_read_info(png_ptr, info_ptr);
	*x=png_get_image_width(png_ptr,info_ptr);
	*y=png_get_image_height(png_ptr,info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

/* Returns forbidden_0_to_7 for next char. */
int print_char(FILE *output, int c, int forbidden_0_to_7)
{
	switch(c){
		case '\n':
			fputs("\\n",output);
			forbidden_0_to_7=0;
			break;

		case '\t':
			fputs("\\t",output);
			forbidden_0_to_7=0;
			break;

		case '\b':
			fputs("\\b",output);
			forbidden_0_to_7=0;
			break;

		case '\r':
			fputs("\\r",output);
			forbidden_0_to_7=0;
			break;

		case '\f':
			fputs("\\f",output);
			forbidden_0_to_7=0;
			break;

		case '\\':
			fputs("\\\\",output);
			forbidden_0_to_7=0;
			break;

		case '\'':
			fputs("\\\'",output);
			forbidden_0_to_7=0;
			break;

		default:
			if (c<' '||c=='"'||c=='?'||c>126
				||(c>='0'&&c<='7'&&forbidden_0_to_7)){
				fprintf(output,"\\%o",c);
				forbidden_0_to_7=(c<0100);
			}else{
				fprintf(output,"%c",c);
				forbidden_0_to_7=0;
			
			}
			break;
	}
	return forbidden_0_to_7;
}


/* name:        path to file to process
 * output:      where to put the C code
 * char_number: number of the char. -1 means do not write anything into the tables.
*/
static inline void process_file(unsigned char *name, FILE*output, int char_number)
{
	FILE *f;
	int c;
	int letter_code=letter_code;
	int count=0;
	unsigned char btr[5];
	int forbidden_0_to_7;

	f=fopen(name,"r");
	if (!f){
		fprintf(stderr,"%s can't open file %s.\n",PROGNAME,
				name);
		perror(PROGNAME);
		exit(1);
	}
	if (char_number>=0){
		memcpy(btr,name,4);
		btr[4]=0;
		letter_code=strtoul(btr,NULL,16);
		fonts[n_fonts-1].letters[char_number].begin=stamp;
	}
	fprintf(output,"\"");
	forbidden_0_to_7=0;

	while(EOF!=(c=fgetc(f))){
		forbidden_0_to_7=print_char(output,c,forbidden_0_to_7);
		stamp++;
		count++;
	}
	fprintf(output,"\"\n");
	rewind(f);
	fonts[n_fonts-1].letters[char_number].length=count;
	fonts[n_fonts-1].letters[char_number].code=letter_code;
	get_png_dimensions(&(fonts[n_fonts-1].letters[char_number].xsize),
		&(fonts[n_fonts-1].letters[char_number].ysize),f);
	fclose(f);
}

void process_letters(FILE *output)
{
	struct dirent **namelist;
	struct dirent **ptr;
	int nr,a;
	
	nr=scandir(".",&namelist,file_select,alphasort);
	if (nr<0){
		perror(PROGNAME);
		exit(1);
	}
	fonts[n_fonts-1].n_letters=nr;
	if (!nr) return;
	if (nr<0){
		perror(PROGNAME);
		exit(1);
	}
	n_letters+=nr;
	fonts[n_fonts-1].letters=malloc(sizeof(struct letter)*nr);
	if (!(fonts[n_fonts-1].letters)){
		fprintf(stderr,"%s: Out of memory", PROGNAME);
		exit(1);
	}
	ptr=namelist;                                    
	for(a=0;a<nr;a++){
		process_file((*ptr)->d_name,output,a);
		free(*ptr);
		ptr++;
	}	
	free(namelist);
}

/* build_font_table:
 * scans the font directory
 * for each directory:
 *  * renews font description structures
 *  * scans the images
 *  * builds a binary search array
 *  * writes the font structure into the font_include.c
 *  * releases font description structures
 *  Also prints the font_data byte table.
 */
void build_font_table(FILE *output)
{
	struct dirent *directory_entry;
	unsigned char *directory_name;
	DIR *dir;
	struct stat stat0;
	unsigned char *family, *weight, *slant, *adstyl, *spacing;
        /*
	directory_name="system_font";
	if (0>chdir(directory_name)){
		fprintf(stderr,"%s: can't change into directory %s.\n",
		PROGNAME,directory_name);
		perror(PROGNAME);
		exit(1);
	}
	add_font_name("","","","","");
	process_letters(output);
	if (0>chdir("..")){
		perror(PROGNAME);
		exit(1);
	}
	*/
	directory_name="font";
	dir=opendir(directory_name);
	if (0>chdir(directory_name)){
		fprintf(stderr,"%s can't change into directory %s.\n",
				PROGNAME,directory_name);
		perror(PROGNAME);
		exit(1);
	}
	if (!dir){
		fprintf(stderr,"%s can't opendir() directory %s.\n",PROGNAME,
				directory_name);
		perror(PROGNAME);
		exit(1);
	}
	while((directory_entry=readdir(dir))){
		if (0>stat(directory_entry->d_name,&stat0)) continue; 
			/* Can't be stated
			 */
		if (!S_ISDIR(stat0.st_mode)) continue; /* Is no directory */
		/* Process the directory */
		if (parse_font_name(directory_entry->d_name,&family,&weight,&slant,
			&adstyl,&spacing))
		{
			continue; /* Inappropriate name */
		}
		/* If the directory is name a-... and in a-../aliases there
		 * is
		 * b
		 * c
		 * d
		 *, then family is a-b-c-d.
		 */
		add_font_name(family,weight,slant,adstyl,spacing);
		process_letters(output);
		if (0>chdir("..")){
			perror(PROGNAME);
			exit(1);
		}
	}
	fprintf(output,"\n};\n");
}

void print_string(unsigned char * ptr, FILE * output)
{
	int forbidden_0_to_7;

	fprintf(output,"\"");
	forbidden_0_to_7=0;
	while(*ptr) forbidden_0_to_7=print_char(output,*ptr++,forbidden_0_to_7);
	fprintf(output,"\"");
}

void print_letter(struct letter *p, FILE * output)
{
	fprintf(output,"{0x%08x,0x%08x,0x%08x,% 5d,% 5d, 0, 0}",p->begin,p->length,
		p->code, p->xsize, p->ysize);
}


void print_font(int a, FILE * output)
{
	struct font *f=fonts+a;

	fprintf(output,"{\n");
	print_string(f->family,output);
	fprintf(output,",\n");
	print_string(f->weight,output);
	fprintf(output,",\n");
	print_string(f->slant,output);
	fprintf(output,",\n");
	print_string(f->adstyl,output);
	fprintf(output,",\n");
	print_string(f->spacing,output);
	fprintf(output,",\n");
	fprintf(output,"%d,\n",n_letters);
	fprintf(output,"%d,\n",f->n_letters);
	n_letters+=f->n_letters;
	fprintf(output,"},\n");
}

void print_font_table(FILE *output)
{
	int a;
	int index;

	fprintf(output,"/* Font data %d bytes */\n\n",stamp);
	fprintf(output,"struct builtin_letter letter_data[%d]={\n",n_letters);
	for (a=0;a<n_fonts;a++){
		struct letter *ptr=fonts[a].letters;
		int count=fonts[a].n_letters;

		for(;count;count--){
			print_letter(ptr,output);
			fprintf(output,",\n");
			ptr++;
		}
	}
	fprintf(output,"};\n\n");
	fprintf(output,"struct builtin_font builtin_font_table[%d]={\n",n_fonts);
	n_letters=0;
	for (a=0,index=0;a<n_fonts;a++){
		print_font(a,output);
	}
	fprintf(output,"};\n");
	fprintf(output,"\nint n_builtin_fonts=%d;\n",n_fonts);
}

int main(int argc, char **argv)
{
	FILE *output;
	int retval;

	while(!(output=fopen("font_include.c","w"))
		&&(errno==EAGAIN||errno==EINTR));
	if (!output){
		fprintf(stderr,PROGNAME ": ");
		perror("font_include.c");
		exit(1);
	}
	
	fprintf(output,"#include \"cfg.h\"\n\n");
	fprintf(output,"#ifdef G\n\n");
	fprintf(output,"#include \"links.h\"\n\n");
	fprintf(output,"unsigned char builtin_font_data[]={\n");
	build_font_table(output);
	print_font_table(output);
	fprintf(output,"\n\n#endif\n\n");
	while ((retval=fclose(output)&&(errno==EAGAIN||errno==EINTR)));
	if (retval){
		perror(PROGNAME);
		exit(1);
	}
	return 0;
}
