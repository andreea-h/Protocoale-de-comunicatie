#include <stdlib.h>
#include <unistd.h> /* pentru open(), exit() */
#include <fcntl.h> /* O_RDWR */
#include <errno.h> /* perror() */
#include <stdio.h>
#include <string.h>


void fatal(char * mesaj_eroare)
{
    perror(mesaj_eroare);
    exit(1);
}

int main(void)
{
    int miner_destinatie;
    int copiat;
    char *buf=(char *)malloc(1024*sizeof(char));
    char **lines = (char **)malloc(200*sizeof(char *));
    size_t dim=1024;

    FILE* miner_sursa = fopen("sursa", "r");
    miner_destinatie = open("destinatie", O_WRONLY | O_CREAT, 0644);
 
    if (miner_sursa < 0 || miner_destinatie < 0)
        fatal("Nu pot deschide un fisier");
 
    int i=0;
    while (getline(&buf, &dim, miner_sursa)>0){
        //copiaza buf in matricea de linii
        lines[i]=(char *)malloc(1024*sizeof(char));
        strcpy(lines[i],buf);
        i++;
    }

    int nr_lines = i-1;
    int j;
    for(j=nr_lines;j>=0;j--) {
        write(1, lines[j], 1024);
    }
 
    fclose(miner_sursa);
    close(miner_destinatie);

    return 0;
}