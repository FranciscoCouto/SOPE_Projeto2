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
#include <sys/wait.h>
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


Shared_memory * attach_shared_memory(char* shm_name, int shm_size)
{
    int shmfd;
    Shared_memory *shm;
    shmfd = shm_open(shm_name,O_RDWR|O_EXCL,0660);
 
  if(shmfd<0)
  {
    perror("Failure in shm_open()");
    return NULL;
  }
  //attach this region to virtual memory
  shm = mmap(0,shm_size,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
  if(shm == MAP_FAILED)
  {
    perror("Failure in mmap()"); 
    return NULL;
  }
  
  return (Shared_memory *) shm;
} 

int smallestLineID(Shared_memory *shm)
{
    int i;
    int lowest = 100;
    int id = 0; 

    for(i = 0; i < MAX_BALCOES; i++){
      if((shm->tabela[i].fifoid != 0) && (shm->tabela[i].durAb == -1)){
        if(shm->tabela[i].cliemAt < lowest){
          lowest = shm->tabela[i].cliemAt;
          id = shm->tabela[i].fifoid;

        }
      }
    }

    return id;
} 

void destroyFIFO(char *name)
{
    if (unlink(name)<0)
      printf("Error when destroying FIFO %s \n" , name);
  else
      printf("FIFO %s has been destroyed\n", name);
}

int main (int argc, char *argv[])
{

  Shared_memory *shmem;
  int i;
  pid_t pid;

  int numForks = atoi(argv[2]);
  
  if(argc != 3) //primeiro teste de erro do programa para seguir a tipologia fornecida
  {
        printf("\n O programa deve fornecer executável, <nome_mempartilhada> e <num_clientes>! \n");
        return 1;
  }

  setbuf(stdout, NULL);

 if ((shmem = attach_shared_memory(argv[1], sizeof(Shared_memory))) == NULL)
 {
    perror("Cliente: could not attach shared memory");
    exit(EXIT_FAILURE);
 } 

  printf("\nGerando %d novos clientes!\n", numForks);

    for( i=0; i < numForks; i++ )
    {
     pid = fork();

    if(pid < 0) {

        printf("Error Forking a Client!");
        exit(EXIT_FAILURE);
    } 

    else if (pid == 0) {
      
      printf("Client (%d): %d\n", i + 1, getpid());
     
      char fifo_cliente[256];
      char buf[2048];
      int inte;
      int c_id = getpid();
      sprintf(fifo_cliente, "tmp/fc%d", c_id);

      if( mkfifo(fifo_cliente, 0666 ) < 0 ) { //Criação do fifo relativo ao cliente criado
        if( errno == EEXIST ) {
            printf("Client Fifo '%s' already exists!\n", fifo_cliente);
            exit( EXIT_FAILURE);
          }
        else
            printf("Can't create client FIFO\n"); 
            exit( EXIT_FAILURE); 
        }
       
      pthread_mutex_lock(&shmem->tabela_lock);
      int b_id = smallestLineID(shmem);
      pthread_mutex_unlock(&shmem->tabela_lock);
      
      char fifo_balcao[256];
      sprintf(fifo_balcao, "tmp/fb%d", b_id);

      printf("Sending to: %s\n", fifo_balcao);

      int in_fd=open(fifo_balcao, O_WRONLY);

      if (in_fd==-1) {
        printf("O balcão %s já fechou!\n", fifo_balcao);
        exit(1);
    }

      //myLog("teste", 1, 1, "pede_atendimento", fifo_cliente);
      myLog(1, i+1, "pede_atendimento", fifo_cliente);

      write(in_fd, fifo_cliente, 256*sizeof(char));

      int in_fde=open(fifo_cliente, O_RDONLY);

        if (in_fde == -1) {
        perror("Open error in Cliente (pai)!");
        exit( EXIT_FAILURE );
      }

        inte = read(in_fde,&buf,2048*sizeof(char));
        if (inte>0){
        if(strcmp("fim_atendimento", buf) == 0){
        printf("%s is finished (%s)\n", fifo_cliente, buf);
        myLog(1, i+1, "fim_atendimento", fifo_cliente);
      }
      }

    close(in_fd);
    close(in_fde);
    destroyFIFO(fifo_cliente); 

    exit(0);

    } 
  }

  exit(0);

}
