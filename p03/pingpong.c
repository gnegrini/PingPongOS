#include "datatypes.h" // estruturas de dados necessárias
#include "stdio.h"
#define DEBUG
#include "queue.h"
#include "pingpong.h"


task_t task_main;
task_t task_dispatcher;
task_t *task_atual;
int next_id;

char *stack;

queue_task *ready_queue;

int userTasks;

// funções gerais ==============================================================
void dispatcher_body();

task_t* scheduler();


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

    userTasks = -1;

    task_create(&task_dispatcher, dispatcher_body, "NULL");



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

    //talvez temos que checar o retorno da make context para retornar se deu erro
    makecontext(&(task->context), (void *)(*start_func), 1, arg);

    //atribui id para a task e atualiza o topo
    task->tid = next_id;
    next_id++;

    userTasks++;

    //adiciona nova tarefa no final da fila
    queue_append((queue_t**) &ready_queue, (queue_t*) task);

#ifdef DEBUG
    printf("task_create: criou tarefa %d\n", task->tid);
#endif

    return task->tid;
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
   
    task->prev = task_atual;
    task_atual->next = task;
    task_atual = task;

    #ifdef DEBUG
        printf("task_switch: trocando contexto %d -> %d\n",task->prev->tid, task->tid);
    #endif   
  
    int rt = swapcontext(&(task->prev->context), &(task->context)); 
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
        printf ("task_exit: tarefa %d sendo encerrada\n", task_atual->tid) ;
    #endif

    userTasks--;

    free(task_atual->context.uc_stack.ss_sp);
    task_yield();    
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id()
{
     #ifdef DEBUG
        printf ("task_id: tarefa corrente %d\n", task_atual->tid) ;
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

void dispatcher_body(void * arg)
{
    task_t *next;
    while ( userTasks > 0 )
    {
    next = scheduler(); 
    if (next)
        {
        //... // ações antes de lançar a tarefa "next", se houverem
        task_switch (next) ; // transfere controle para a tarefa "next"
        //... // ações após retornar da tarefa "next", se houverem
        }
    }
    task_exit(0) ; // encerra a tarefa dispatcher

}

task_t* scheduler(){

    if(queue_size((queue_t*) ready_queue)==0){
        return NULL;
    }

    task_t* task_retorno = (task_t*) ready_queue->prev;
    queue_remove ((queue_t**) &ready_queue, (queue_t*) ready_queue->prev); 


    return task_retorno;
}

void task_yield()
{

    queue_append((queue_t**) &ready_queue, (queue_t*) task_atual);


    #ifdef DEBUG
        printf ("task_yield: tarefa corrente %d\n", task_atual->tid) ;
    #endif

    task_switch(&task_dispatcher);
}