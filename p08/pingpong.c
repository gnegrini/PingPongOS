// PingPongOS
// Baseado no Projeto do Prof. Maziero
// Sistemas Operacionais - UTFPR - 2019.2
// Alunos: Giovane N. M. Costa
//         Henrique K. Xavier

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
task_t *suspended_queue;

unsigned int time;

int userTasks;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;

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

    
    //Inicializacao da Main

    task_main.tid = 0;
    task_main.prev = NULL;
    task_main.next = NULL;
    task_main.activations = 0;
    task_main.prio = 0;
    task_main.prio_d = 0;
    task_main.processor_time = 0;
    task_main.start_time = 0;
    task_main.status = Normal;
    task_main.type = User;
    task_main.start_time = 0;

    //salva o contexto atual em ContextMain
    getcontext(&(task_main.context));

    //--

    //continua inicializacao do OS
    task_atual = &task_main;

    //inicializa contador para id da proxima tarefa
    next_id = 1;

    //inicializa o numer de tarefas, uma sempre esta executando => -1
    userTasks = 0;

    task_create(&task_dispatcher, dispatcher_body, "NULL");    

    time = 0;
    start_timer();

    //ativa o dispatcher
    task_yield();

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
    task->processor_time = 0;
    task->activations = 0;
    task->waits = NULL;

    //talvez temos que checar o retorno da make context para retornar se deu erro
    makecontext(&(task->context), (void *)(*start_func), 1, arg);

    //atribui id para a task e atualiza o topo
    task->tid = next_id;
    next_id++;

    

    if (task == &task_dispatcher)
    {
        task->type = System;
    }
    else
    {
        task->type = User;
        userTasks++;
        queue_append((queue_t **)&ready_queue, (queue_t *)task);
        task->queue = &ready_queue;
    }

#ifdef DEBUG
    printf("task_create: criou tarefa %d\n", task->tid);
#endif

    task->start_time = time;

    return task->tid;
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    task_t *task_old = task_atual;
    task_atual = task;
    
    task_atual->activations++;

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

    //talvez temos que decrementar apenas se  nao for dispatcher
    userTasks--;

    //atualiza o status da tarefa
    if (task_atual != &task_main)
    {
        task_atual->status = Finished;
    }
    
    task_atual->exit_code = exitCode;

    

    //acorda todas as tarefas que dependem dela encerrar
    if (queue_size((queue_t *)suspended_queue) != 0)
    {
        task_t *atual = (task_t *)suspended_queue;

        do
        {

            atual = atual->next;

            if(atual->prev->waits == task_atual){
                task_resume(atual->prev);
            }

            

        } while (queue_size((queue_t *)suspended_queue) != 0 && atual != suspended_queue);        

    }
    
    

    unsigned int exec_time = time - task_atual->start_time;

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", 
        task_atual->tid, exec_time, task_atual->processor_time, task_atual->activations);

    if (task_atual == &task_dispatcher)
    {

        //talvez de para dar um free no dispatcher??
        task_switch(&task_main);
    }
    else
    {

        task_switch(&task_dispatcher);
    }
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id()
{
#ifdef DEBUG
    printf("task_id: tarefa corrente %d\n", task_atual->tid);
#endif

    return task_atual->tid;
}


// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL
void task_suspend(task_t *task, task_t **queue){

    //atribui  tarefa atual se task==NULL
    if (task == NULL)
    {
        task = task_atual;
    }
    
    //retira da fila atual (no caso, a fila de prontas)
    //se task==task_atual, ela já foi tirada da fila pelo scheduler
    //if(task!=task_atual)
    if(task->queue!= NULL)
        queue_remove((queue_t **)task->queue, (queue_t *)task);

    //adiciona à fila **queue
    queue_append((queue_t **)queue, (queue_t *)task);
    task->queue = queue;

    //muda estado para suspensa
    task->status = Suspended;

    //passa o controle para o dispatcher executar a proxima tarefa
    task_switch(&task_dispatcher);
}

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume(task_t *task){

    //retira da fila atual
    queue_remove((queue_t **)task->queue, (queue_t *)task);
    
    //adiciona à fila de tarefas prontas
    queue_append((queue_t **)&ready_queue, (queue_t *)task);
    task->queue = &ready_queue;

    //muda seu estado para pronta
    task->status = Normal;

}

//suspende a tarefa atual até que *task encerre
int task_join (task_t *task){
    
    //checa se *task não existe ou ja foi encerrada
    if(task == NULL || task->status == Finished){
        return -1;
    } 
    
    
    //marca que a tarefa esta suspensa esperando *task
    //RESTRIÇÃO: É POSSIVEL ESPERAR APENAS POR UMA TAREFA
    task_atual->waits = task;
    
    //suspende a tarefa atual e retorna o código de encerramento da tarefa *task;
    task_suspend(NULL, &suspended_queue);
    return task->exit_code;
    
}

/////////////////////////////////////////////////////////////

void dispatcher_body(void *arg)
{    

    task_t *next;
    while (userTasks >= 0)
    {
        next = scheduler();
        if (next)
        {
            //... // ações antes de lançar a tarefa "next", se houverem
            queue_remove((queue_t **)&ready_queue, (queue_t *)next);
            next->queue = NULL;
            
            curr_quantum = QUANTUM;

            //for para testar o tempo do dispatcher
            // for (int i=0; i<4000; i++)
            //     for (int j=0; j<4000; j++) ;

            task_switch(next); // transfere controle para a tarefa "next"

            //... // ações após retornar da tarefa "next", se houverem
            if (next->status == Finished)
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
    task_t *task_retorno = (task_t *)ready_queue;

    //navegar a fila até encontrar a tarefa com maior prio_d

    task_t *atual = (task_t *)ready_queue;

    do
    {
        if (atual->prio_d < task_retorno->prio_d)
        {
            task_retorno = atual;
        }

        atual = atual->next;

    } while (atual != ready_queue);

    //envelhecer todas as outras tarefas
    atual = task_retorno->next;
    do
    {
        if (atual->prio_d > -20)
            atual->prio_d--;

        atual = atual->next;

    } while (atual != task_retorno);

    //rejuvenescer a tarefa retorno
    task_retorno->prio_d = task_retorno->prio;

    return task_retorno;
}

void task_yield()
{

#ifdef DEBUG
    printf("task_yield: tarefa corrente %d\n", task_atual->tid);
#endif

    if (task_atual != &task_dispatcher)
    {
        queue_append((queue_t **)&ready_queue, (queue_t *)task_atual);
        task_atual->queue = &ready_queue;
    }


    task_switch(&task_dispatcher);
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio(task_t *task, int prio)
{

    if (prio > 20 || prio < -20)
    {
        printf("Warning: Limite de prioridade excedido.\n");
        exit(1);
    }

    if (task == NULL)
    {
        task_atual->prio = prio;
        task_atual->prio_d = prio;
    }
    else
    {
        task->prio = prio;
        task->prio_d = prio;
    }
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio(task_t *task)
{
    if (task == NULL)
    {
        return task_atual->prio;
    }
    else
    {
        return task->prio;
    }
}

// tratador do sinal do timer
void tratador(int signum)
{
    time++;

    task_atual->processor_time++;

    if (task_atual->type == User)
    {
        curr_quantum--;
        if (curr_quantum == 0)
        {
            task_yield();
        }
    }
}

void start_timer()
{

    // registra a aï¿½ï¿½o para o sinal de timer SIGALRM
    action.sa_handler = tratador;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(1);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;    // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec = 0;        // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000; // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec = 0;     // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer(ITIMER_REAL, &timer, 0) < 0)
    {
        perror("Erro em setitimer: ");
        exit(1);
    }
}

unsigned int systime()
{
    return time;
}