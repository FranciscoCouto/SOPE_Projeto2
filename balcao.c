#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h> // For O_* constants
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "log.h"

#define MAX_BALCOES 20


typedef struct {
 int numbalcao; //número do balcão
 long tempoIn; //tempo em que iniciou o funcionamento (epoch)
 int durAb; //duração da abertura em segundos 
 int fifoid; //id do fifo em que o balcão atende clientes
 int cliemAt; //clientes em atendimento
 int cliAt; //clientes atendidos
 float tempomAt; //tempo médio de atendimento
} tabela_balcao;

typedef struct {
 pthread_mutex_t tabela_lock;
 long tempoab;  //tempo de abertura da loja
 int nbalcoes; // numero de balcoes registados
 tabela_balcao tabela[MAX_BALCOES];
} Shared_memory;

Shared_memory *shmem; 
int numerobal;

Shared_memory * create_shared_memory(char* shm_name, int shm_size)
{

 int shmfd;
 Shared_memory *shm;
 //create the shared memory region
 shmfd = shm_open(shm_name,O_CREAT|O_RDWR|O_EXCL,0660);

 if(shmfd<0)
 {  
   printf("\nShared Memory Already Existed : Attaching!\n");
   shmfd = shm_open(shm_name,O_RDWR|O_EXCL,0660);
   shm = mmap(0,shm_size,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
   return (Shared_memory *) shm;
 }
 //specify the size of the shared memory region
 if (ftruncate(shmfd,shm_size) < 0)
 {
   perror("Failure in ftruncate()");
   return NULL;
 } 
 //attach this region to virtual memory
 shm = mmap(0,shm_size,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
 if(shm == MAP_FAILED)
 {
   perror("Failure in mmap()");
   return NULL;
 }

 shm->tempoab = 0;
 shm->nbalcoes = 0;
 pthread_mutexattr_t mattr;
 pthread_mutexattr_init(&mattr);
 pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

 pthread_mutex_init(&shm->tabela_lock, &mattr);
 myLog(2, 1, "inicia_mempart", "-");
 myLog(0, 1, "inicia_mempart", "-");

 return (Shared_memory *) shm;

}

void destroy_shared_memory(Shared_memory *shm, char* shm_name, int shm_size)
{
 if (munmap(shm,shm_size) < 0)
 {
   perror("Failure in munmap()");
   exit(EXIT_FAILURE);
 }
 if (shm_unlink(shm_name) < 0)
 {
   perror("Failure in shm_unlink()");
   exit(EXIT_FAILURE);
 }
} 

void destroyFIFO(char *name)
{
    if (unlink(name)<0)
      printf("Error when destroying FIFO %s \n" , name);
  else
      printf("FIFO %s has been destroyed\n", name);
}

void *atendimento(void * arg)
{

 int tempo_atend;
 char *cli = (char *) arg;

//myLog(0, numerobal, "inicia_atend_cli", cli);

int in_fd=open(cli, O_RDWR);

if (in_fd==-1) {
    perror("Open error in client!");
    exit( EXIT_FAILURE );
  }

pthread_mutex_lock(&shmem->tabela_lock);
tempo_atend = shmem->tabela[numerobal-1].cliemAt +1;
pthread_mutex_unlock(&shmem->tabela_lock);

if(tempo_atend > 10){

  tempo_atend = 10;
}

printf("\nClient '%s' will take %d seconds to finish\n", cli, tempo_atend);

pthread_mutex_lock(&shmem->tabela_lock);
shmem->tabela[numerobal-1].cliemAt += 1;
pthread_mutex_unlock(&shmem->tabela_lock);
sleep(tempo_atend);

write(in_fd,"fim_atendimento", 20);
//myLog(0, numerobal, "fim_atend_cli", cli); 

close(in_fd);

pthread_mutex_lock(&shmem->tabela_lock);
shmem->tabela[numerobal-1].cliAt += 1;
shmem->tabela[numerobal-1].cliemAt -= 1;
pthread_mutex_unlock(&shmem->tabela_lock);
  
return NULL;

} 

int main (int argc, char *argv[])
{

  if(argc != 3) //primeiro teste de erro do programa para seguir a tipologia fornecida
  {
    printf("\n O programa deve fornecer executável, <nome_mempartilhada> e <temp_abertura>! \n");
    return 1;
  }

  setbuf(stdout, NULL);

  int tempo_abertura = atoi(argv[2]);
  int startTime = time(NULL);
  int endTime = startTime+tempo_abertura;

  printf("\nSeconds since the Epoch: %d\n", startTime);

  //CRIAçÂO DO FIFO DE ATENDIMENTO/BALCÃO COM A TERMINOLOGIA CORRECTA
  int pid = getpid();
  char fifo_balcao[256];
  sprintf(fifo_balcao, "tmp/fb%d", pid);

  tabela_balcao line;

  if( mkfifo(fifo_balcao, 0666 ) < 0 ) { //Criação do fifo relativo ao balcão criado

    if( errno == EEXIST ) {
      printf("\nCounter's FIFO '%s' already exists\n", fifo_balcao);
    }

    else
      printf("\nError creating counter's FIFO\n");
  }

  if ((shmem = create_shared_memory(argv[1], sizeof(Shared_memory))) == NULL)
    {
      perror("\nCounter: could not create shared memory\n");
      exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&shmem->tabela_lock);
    shmem->nbalcoes += 1;
    numerobal = shmem->nbalcoes; 
    pthread_mutex_unlock(&shmem->tabela_lock);

    line.numbalcao = numerobal;
    line.tempoIn = startTime;
    line.durAb = -1;
    line.fifoid = pid;
    line.cliemAt = 0;
    line.cliAt = 0;
    line.tempomAt = 0.0;
    pthread_mutex_lock(&shmem->tabela_lock);
    shmem->tabela[numerobal-1] = line;
    pthread_mutex_unlock(&shmem->tabela_lock);

    myLog(0, numerobal, "cria_linh_mempart", fifo_balcao);

    printf("\nCreated counter %d: %s  -  %ds\n", numerobal, fifo_balcao, tempo_abertura);

    pthread_t threads[2048];
    char cli_fifo_buffer[2048];
    int thread_i = 0;
    int fif;

    int in_fd=open(fifo_balcao, O_RDONLY);

    if (in_fd==-1) {
        perror("Open error in balcao!");
        destroyFIFO(fifo_balcao);
        exit( EXIT_FAILURE );
    }

    do
    {
     
      fif = read(in_fd,&cli_fifo_buffer,2048*sizeof(char));

      if (fif>0){

        printf("Client %s has arrived\n",cli_fifo_buffer);
  
        pthread_create(&threads[thread_i], NULL, atendimento, (void*)cli_fifo_buffer);
        thread_i++;

      }

    } while(time(NULL) <= endTime);
    
    int k;
    for(k = 0; k < thread_i; k++)
        pthread_join(threads[k], NULL);


    //float serv = ((float)shmem->tabela[numerobal-1].cliAt / (float)tempo_abertura);
    
    printf("Closing counter: %s\n", fifo_balcao);
    printf("Clients served: %d\n", shmem->tabela[numerobal-1].cliAt);
    

    close(in_fd);
    destroyFIFO(fifo_balcao);

    pthread_mutex_lock(&shmem->tabela_lock);
    shmem->nbalcoes -= 1;
    shmem->tabela[numerobal-1].durAb = tempo_abertura;
    //shmem->tabela[numerobal-1].tempomAt = serv;
    pthread_mutex_unlock(&shmem->tabela_lock);


    myLog(0, numerobal, "fecha_balcao", fifo_balcao);

    if(shmem->nbalcoes == 0){

    destroy_shared_memory(shmem, argv[1], sizeof(Shared_memory));

    printf("\nShared Memory %s has been destroyed!\n", argv[1]);

    myLog(0, numerobal, "fecha_loja", fifo_balcao);
    myLog(3, 1, "inicia_mempart", "-");

    exit(0);

    } 

    exit(0);

}