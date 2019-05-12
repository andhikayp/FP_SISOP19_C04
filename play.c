#include <ao/ao.h>
#include <mpg123.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BITS 8

char directory[] = "/home/andhika/Music/";

int flag_akhir = 0;
char lagu[100];
int pause_status = 0;
char queue[100][100];
int idx, awal = 1;

int interface_status = 1;
char a = '\0';
struct termios old, new;
/* Read 1 character - echo defines echo mode */
char getch() 
{
    char ch;
    tcgetattr(0, &old); /* grab old terminal i/o settings */
    new = old; /* make new settings same as old settings */
    new.c_lflag &= ~ICANON; /* disable buffered i/o */
    // new.c_lflag |= ECHO; /* set echo mode */
    new.c_lflag &= ~ECHO; /* set no echo mode */
    tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
    ch = getchar();
    tcsetattr(0, TCSANOW, &old);
    return ch;
}

void *play(void *ptr)
{
    char nama_lagu[100];
    while(1)
    {
        int a;
        while(wait(a) > 0);
        memset(nama_lagu,0,sizeof(nama_lagu));
        strcpy(nama_lagu,directory);
        strcat(nama_lagu,lagu);
        if(strlen(lagu) > 0)
        {
            flag_akhir = 0;

            mpg123_handle *mh;
            unsigned char *buffer;
            size_t buffer_size;
            size_t done;
            int err;

            int driver;
            ao_device *dev;

            ao_sample_format format;
            int channels, encoding;
            long rate;

            FILE *file = fopen(nama_lagu,"r");
            if(file == NULL) continue;

            /* initializations */
            ao_initialize();
            driver = ao_default_driver_id();
            mpg123_init();
            mh = mpg123_new(NULL, &err);
            buffer_size = mpg123_outblock(mh);
            buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

            /* open the file and get the decoding format */
            mpg123_open(mh, nama_lagu);
            mpg123_getformat(mh, &rate, &channels, &encoding);

            /* set the output format and open the output device */
            format.bits = mpg123_encsize(encoding) * BITS;
            format.rate = rate;
            format.channels = channels;
            format.byte_format = AO_FMT_NATIVE;
            format.matrix = 0;
            dev = ao_open_live(driver, &format, NULL);
            while(1){
                if(pause_status == 0){
                    if(mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK){
                        ao_play(dev, buffer, done);
                    }
                    else break;
                }
                if(flag_akhir) break;
            }
            free(buffer);
            ao_close(dev);
            mpg123_close(mh);
            mpg123_delete(mh);
            mpg123_exit();
            ao_shutdown();
        }
    }
}

void queueLagu(){
    DIR *dp;
    struct dirent *de;
    dp = opendir(directory);
    if (dp == NULL) return;
    else
    {
        int i = 0;
        while ((de = readdir(dp)) != NULL) {
            if(strcmp(de->d_name,".") != 0 && strcmp(de->d_name,"..") != 0 )
            {
                strcpy(queue[i],de->d_name);
                i++;
            }   
        }
        closedir(dp);
    }
}

int index_lagu(char nama_lagu[100]){
    DIR *dp;
    struct dirent *de;

    dp = opendir(directory);
    if (dp != NULL)
    {
        int count=0;
        while ((de = readdir(dp)) != NULL) {
            if(strcmp(de->d_name,".") != 0 && strcmp(de->d_name,"..") != 0){
                if(strcmp(nama_lagu,de->d_name) == 0){
                    return count;
                }
                count++;
            }
        }
    }
    else
        printf("List lagu kosong\n");
    closedir(dp);
    return -1;
}

int jumlah_lagu()
{
    DIR *dp;
    struct dirent *de;
    int count=0;
    dp = opendir(directory);
    if (dp != NULL)
    {
        while ((de = readdir(dp)) != NULL) {
            if(strcmp(de->d_name,"..") != 0 && strcmp(de->d_name,".") != 0){
                count++;
            }
        }
    }   
    else
        printf("List lagu kosong\n");
    closedir(dp);
    return count;
}


void input()
{
    while((a = getch()))
    {
        if(a=='1')
        {
            interface_status=2;
        }
        //next
        else if(a=='2')
        {
            awal = 0;
            flag_akhir = 1;
            memset(lagu,0,sizeof(lagu));
            idx++;
            if(idx > jumlah_lagu() - 1){
                idx = 0;
            }
            strcpy(lagu,queue[idx]);
            pause_status = 0;
        }
        //prev
        else if(a=='3')
        {
            awal = 0;
            flag_akhir = 1;
            memset(lagu,0,sizeof(lagu));
            idx--;
            if(idx < 0){
                idx = jumlah_lagu() - 1;
            }
            strcpy(lagu,queue[idx]);
            pause_status = 0;
        }
        //pause
        else if(a=='4')
        {
            pause_status = 1;
        }
        //resume
        else if(a=='5' && pause_status == 1)
        {
            pause_status = 0;
        }
        //menu utama
        else if(a=='9')
        {
            interface_status = 1;
        }
        //selain itu
        else
        {
            printf("Invalid INPUT WOI!\n");
        }
    }
}
void *interface()
{
    while(1)
    {
        if (interface_status == 1)
        {
            printf("MP3 Musik Gaul\n\n");
            if (awal==1)
            {
                printf("Ready?\n");
            }
            else if(jumlah_lagu()>0)
            {
                int idx_prev = idx-1;
                if(idx_prev < 0){
                    idx_prev = jumlah_lagu() - 1;
                }
                printf("Lagu Sebelumnya     : --- %s ---\n", queue[idx_prev]);
                printf("Sedang Dimainkan    : --- %s ---\n", lagu);
                int idx_next = idx+1;
                if(idx_next > jumlah_lagu()-1){
                    idx_next = 0;
                }
                printf("Lagu Selanjutnya    : --- %s ---\n\n", queue[idx_next]);
            }
            else
                printf("List lagu kosong\n\n");
            
            printf("Tekan 1 untuk show list lagu\n");
            printf("Tekan 2 untuk next\n");
            printf("Tekan 3 untuk prev\n");
            printf("Tekan 4 untuk pause\n");
            printf("Tekan 5 untuk resume\n");
            sleep(1);
            system("clear");
        }
        else if (interface_status == 2)
        {
            printf("Jumlah lagu: %d\n", jumlah_lagu());
            DIR *dp;
            struct dirent *de;
            dp = opendir(directory);
            if (dp != NULL)
            {
                while ((de = readdir(dp)) != NULL) 
                {
                    if(strcmp(de->d_name,"..") != 0 && strcmp(de->d_name,".") != 0)
                        printf("%s\n",de->d_name);
                }
                closedir(dp);
                printf("\n\nTekan 9 untuk kembali\n");
                sleep(1);
                system("clear");
            }        
        }
    }
}

int main(int argc, char *argv[])
{
    system("clear");
    queueLagu();
    pthread_t thread[5];

    pthread_create(&thread[0],NULL,play,NULL);
    pthread_create(&thread[1],NULL,interface,NULL);
    input();
    pthread_join(thread[0],NULL);
    pthread_join(thread[1],NULL);
    return 0;
}
