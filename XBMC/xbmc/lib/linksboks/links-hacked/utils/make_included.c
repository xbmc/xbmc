/* Generate C-style string with file content */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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

int main(int argc, char **argv)
{
        int i;

        printf("struct included_files included[] = {\n"
              );
        for(i=1;i<argc;i++){
                unsigned char *filename = argv[i];
                FILE *file = fopen(filename,"r");
                int c;
                int int_data = 0;
                int length = 0;

                if(!file)
                        continue;

                fprintf(stderr,"Processing file %s\n",filename);

                printf("{\"%s\", \"",filename);
                while( (c = fgetc(file)) != EOF ){
                        int_data=print_char(stdout,c,int_data);
                        length++;
                }
                printf("\", %d },\n",length);

                fclose(file);
        }
        printf("{ NULL, NULL, 0}"
               "};\n");

        return 0;
}
