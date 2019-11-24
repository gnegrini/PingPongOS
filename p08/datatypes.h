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


enum stat{Finished, Normal, New, Ready, Running, Suspended, Terminated}; //status das tarefas
enum typ{System, User};  //tipos de tarefas

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
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
