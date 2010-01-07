
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// THIS IS BAAAAAD CODING AND CRASHES TOWARDS THE END BUT IT WRITES THE FILE!

int main(int argc, char *argv[])
{
    char s[1024];
    unsigned char header[10], s2[3], val;
    unsigned int type, entries;
    int i, add_entry = 1;

    printf("Generate libbluray config file builder tool\n\n");

    if (argc == 2) {
        FILE *fp = fopen(argv[1], "w");

        while (add_entry) {

            printf("Record type in hex (e.g. 0x01): ");
            scanf("%x", &type);

            printf("Number of entries in dec: ");
            scanf("%d", &entries);

            printf("Record data (all entries): ");
            memset(s, 0, sizeof(s));
            scanf("%s", s);
            memset(header, 0, 256);
            header[0] = type;
            header[1] = (strlen(s)/2 + 10) >> 16;
            header[2] = (strlen(s)/2 + 10) >> 8;
            header[3] = (strlen(s)/2 + 10) & 0xff;
            header[4] = entries >> 8;
            header[5] = entries & 0xff;
            header[6] = strlen(s)/2/entries >> 24;
            header[7] = strlen(s)/2/entries >> 16;
            header[8] = strlen(s)/2/entries >> 8;
            header[9] = strlen(s)/2/entries & 0xff;

            fwrite(header, 10, 1, fp);

            for (i = 0; i < strlen(s); i += 2) {
                s2[0] = s[i];
                s2[1] = s[i + 1];
                s2[2] = 0;
                val = strtol((char*)s2, NULL, 16);
                fputc(val, fp);
            }

            printf("Add another record [y/n]? ");
            scanf("%s", s);
            if ((s[0] != 'Y') && (s[0] != 'y'))
                add_entry = 0;
        }

        fclose(fp);
    } else {
        printf("Usage: %s /path/to/file.bin\n", argv[0]);
    }

    return 0;
}
