#include "datatypes.h" // estruturas de dados necessárias
#include "stdio.h"
#include "queue.h"
#include "pingpong.h"
#include <signal.h>
#include <sys/time.h>
//#define DEBUG

#define QUANTUM 20

task_t task_main;
task_t task_dispatcher;
task_t *task_atual;
int next_id;

char *stack;

task_t *ready_queue;

int userTasks;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicializaÃ§Ã£o to timer
struct itimerval timer;


int curr_quantum;

// funções gerais ==============================================================
void dispatcher_body();

task_t *scheduler();

void start_timer();

void tratador();

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init()
{
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);

    //atribui id 0 a task main
    task_main.tid = 0;
    next_id = 1;

    //salva o contexto atual em ContextMain
    getcontext(&(task_main.context));

    task_main.prev = NULL;
    task_main.next = NULL;

    task_atual = &task_main;

    //uma sempre esta executando
    userTasks = -1;

    task_create(&task_dispatcher, dispatcher_body, "NULL");


    start_timer();

#ifdef DEBUG
    printf("SO inicializado\n");
#endif
}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create(task_t *task,               // descritor da nova tarefa
                void (*start_func)(void *), // funcao corpo da tarefa
                void *arg)                  // argumentos para a tarefa
                {

    getcontext(&(task->context));

    stack = malloc(STACKSIZE);
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        perror("Erro na criacão da pilha: \n");
        return (-1);
    }

    task->prev = NULL;
    task->next = NULL;
    task->status = Normal;
    task->prio = 0;
    task->prio_d = 0;

    //talvez temos que checar o retorno da make context para retornar se deu erro
    makecontext(&(task->context), (void *)(*start_func), 1, arg);

    //atribui id para a task e atualiza o topo
    task->tid = next_id;
    next_id++;

    userTasks++;

    if (task == &task_dispatcher){
        task->type = System;
    }else
    {
        task->type = User;        
        queue_append((queue_t **)&ready_queue, (queue_t *)task);
    }
    
#ifdef DEBUG
    printf("task_create: criou tarefa %d\n", task->tid);
#endif

    return task->tid;
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    task_t *task_old = task_atual;
    task_atual = task;

#ifdef DEBUG
    printf("task_switch: trocando contexto %d -> %d\n", task_old->tid, task_atual->tid);
#endif

    int rt = swapcontext(&(task_old->context), &(task_atual->context));
    // deu ruim
    if (rt == -1)
        return rt;
    else
        return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit(int exitCode)
{
#ifdef DEBUG
    printf("task_exit: tarefa %d sendo encerrada\n", task_atual->tid);
#endif

    userTasks--;

    if (task_atual == &task_dispatcher){

        task_switch(&task_main);
    }else
    {
        task_atual->status = Finnished;
        task_switch(&task_dispatcher);
        
    }
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id(){
#ifdef DEBUG
    printf("task_id: tarefa corrente %d\n", task_atual->tid);
#endif

    return task_atual->tid;
}

////////////// IMPLEMENTAR NA PROXIMA SEMANA /////////////

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL
void task_suspend(task_t *task, task_t **queue);

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume(task_t *task);

/////////////////////////////////////////////////////////////

void dispatcher_body(void *arg)
{
    task_t *next;
    while (userTasks > 0)
    {
        next = scheduler();
        if (next)
        {
            //... // ações antes de lançar a tarefa "next", se houverem
            queue_remove((queue_t **)&ready_queue, (queue_t *)next);
            curr_quantum = QUANTUM;

            task_switch(next); // transfere controle para a tarefa "next"
            
            //... // ações após retornar da tarefa "next", se houverem
            if(next->status == Finnished)
                free(next->context.uc_stack.ss_sp);

        }
    }
    task_exit(0); // encerra a tarefa dispatcher
}

task_t *scheduler()
{

    //não há tarefas
    if (queue_size((queue_t *)ready_queue) == 0)
    {
        printf("SchedErr: fila de prontas vazias\n"); 
        return NULL;
    }


    //pegamos a primeira tarefa da fila para comecar e comparar prioridade
    task_t *task_retorno = (task_t *) ready_queue;

    //navegar a fila até encontrar a tarefa com maior prio_d
    
    task_t *atual = (task_t*) ready_queue;
       
    do
    {                       
        if(atual->prio_d < task_retorno->prio_d){
            task_retorno = atual;
        }

        atual = atual->next;
        
    } while (atual!= ready_queue);
    
    
    //envelhecer todas as outras tarefas
    atual = task_retorno->next;
    do
    {                       
        if(atual->prio_d > -20)
            atual->prio_d--;

        atual = atual->next;
        
    } while (atual!= task_retorno);

    //rejuvenescer a tarefa retorno
    task_retorno->prio_d = task_retorno->prio;    

    return task_retorno;
}

void task_yield()
{

#ifdef DEBUG
    printf("task_yield: tarefa corrente %d\n", task_atual->tid);
#endif

    if (task_atual != &task_dispatcher && task_atual != &task_main)
    {
        queue_append((queue_t **)&ready_queue, (queue_t *)task_atual);
    }

    task_switch(&task_dispatcher);


}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){
    
    if(prio > 20 || prio < -20){
       printf("Warning: Limite de prioridade excedido.\n") ;
       exit(1);
    }
    
    if(task == NULL){
        task_atual->prio = prio;
        task_atual->prio_d = prio;
    }else
    {
        task->prio = prio;
        task->prio_d = prio;
    }
    
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task){
    if(task == NULL){
        return task_atual->prio;
    }else
    {
        return task->prio;
    }
    
}


// tratador do sinal do timer
void tratador (int signum)
{
    if(task_atual->type == User){
        curr_quantum-- ;
        if(curr_quantum == 0){
            task_yield();
        }    
    }
}


void start_timer(){

  // registra a aï¿½ï¿½o para o sinal de timer SIGALRM
  action.sa_handler = tratador ;
  sigemptyset (&action.sa_mask) ;
  action.sa_flags = 0 ;
  if (sigaction (SIGALRM, &action, 0) < 0)
  {
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }

  // ajusta valores do temporizador
  timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
  timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }

}