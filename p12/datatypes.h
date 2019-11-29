// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

#define STACKSIZE 32768		/* tamanho de pilha das threads */
#define _XOPEN_SOURCE 600	/* para compilar no MacOS */

#include <stdlib.h>
#include <ucontext.h>


enum stat{Finished, Normal, New, Ready, Running, Suspended, Destroyed}; //status das tarefas
enum typ{System, User};  //tipos de tarefas
enum err{BarrierDestroyed, MQueueDestroyed};

typedef struct queue_task 
{
   struct queue_task *next ;  // ptr para usar cast com queue_t
   struct queue_task *prev ;  // ptr para usar cast com queue_t
   int id ;
   // outros campos podem ser acrescidos aqui
} queue_task ;

// Estrutura que define uma tarefa
typedef struct task_t
{
    struct task_t *prev, *next;
    int tid;
    ucontext_t context;
    int status;
    int prio; //prioridade estatica
    int prio_d;  //prioridade dinamica
    int type;
    unsigned int start_time;
    unsigned int processor_time;
    int activations;
    int exit_code;
    struct task_t *waits;
    struct task_t **queue;
    unsigned int wake_time;
    int error_code;
    
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  
  int count;
  task_t *queue;
  int status;

} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  int lock;
  task_t *queue;
  int status;
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  int count;
  int max;
  task_t *queue;
  int status;
  semaphore_t sem;
} barrier_t ;

// estrutura de uma msg para funcionar com fila
typedef struct
{  
 
  struct msg_t *prev, *next;
  void *msg_data;

}msg_t;


// estrutura que define uma fila de mensagens
typedef struct
{
  
  msg_t *msg_queue;  //ponteiro da fila de msgs
  int max;
  int size;
  int n_msgs;
  task_t *task_full_mqueue;  //fila de tarefas que aguardam espaço na fila de mensagens
  task_t *task_empty_mqueue;  //fila de tarefas que aguardam novas mensagens
  int status;
  semaphore_t sem;

} mqueue_t ;





#endif
