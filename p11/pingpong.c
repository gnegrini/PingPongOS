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
task_t *sleep_queue;

unsigned int time;

// semaforo para operacoes de barreira
semaphore_t  s_bar ;

// semaforo para suspensao de tarefas
semaphore_t  s_susp ;

int userTasks;

// variavel para ativar/desativar preempcao e permitir op. atomicas
int preempcao;

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

    //continua inicializacao do OS
    task_atual = &task_main;

    //inicializa contador para id da proxima tarefa
    next_id = 1;

    //inicializa o numer de tarefas, uma sempre esta executando => -1
    userTasks = 0;

    //cria semaforo de barreira
    sem_create (&s_bar, 1) ;
    sem_create (&s_susp, 1) ;

    task_create(&task_dispatcher, dispatcher_body, "NULL");

    preempcao = 1;
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
    int count = queue_size((queue_t *)suspended_queue);
    if (count != 0)
    {
        task_t *atual = suspended_queue;
        task_t *q_next;

        for (int i = 0; i < count; i++)
        {
            q_next = atual->next;

            if (atual->waits == task_atual)
            {
                task_resume(atual);
            }

            atual = q_next;
        }
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
void task_suspend(task_t *task, task_t **queue)
{
    sem_down(&s_susp);

    //atribui  tarefa atual se task==NULL
    if (task == NULL)
    {
        task = task_atual;
    }

    //retira da fila atual (no caso, a fila de prontas)
    //se task==task_atual, ela já foi tirada da fila pelo scheduler    
    if (task->queue != NULL)
        queue_remove((queue_t **)task->queue, (queue_t *)task);

    //adiciona à fila **queue
    queue_append((queue_t **)queue, (queue_t *)task);
    task->queue = queue;

    //muda estado para suspensa
    task->status = Suspended;

    sem_up(&s_susp);
    
    //passa o controle para o dispatcher executar a proxima tarefa
    if(task == task_atual)
        task_switch(&task_dispatcher);
}

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume(task_t *task)
{

    sem_down(&s_susp);
    
    //retira da fila atual
    queue_remove((queue_t **)task->queue, (queue_t *)task);

    //adiciona à fila de tarefas prontas
    queue_append((queue_t **)&ready_queue, (queue_t *)task);
    task->queue = &ready_queue;

    //muda seu estado para pronta
    task->status = Normal;

    sem_up(&s_susp);

#ifdef DEBUG
    printf("task_resume: acordou tarefa %d\n", task->tid);
#endif
}

//suspende a tarefa atual até que *task encerre
int task_join(task_t *task)
{

    //checa se *task não existe ou ja foi encerrada
    if (task == NULL || task->status == Finished)
    {
        return -1;
    }

    //marca que a tarefa esta suspensa esperando *task
    //RESTRIÇÃO: É POSSIVEL ESPERAR APENAS POR UMA TAREFA
    task_atual->waits = task;

    //suspende a tarefa atual e retorna o código de encerramento da tarefa *task;
    task_suspend(NULL, &suspended_queue);
    return task->exit_code;
}

void task_sleep(int t)
{

    //calcula o instante em que a tarefa deverá ser acordada
    task_atual->wake_time = systime() + (t * 100);

    #ifdef DEBUG
    printf("task_sleep: dormiu tarefa %d\n", task_atual->tid);
    #endif

    //coloca na fila de tarefas adormecidas
    task_suspend(NULL, &sleep_queue);
}

////////////////////////////////// semáforos

// cria um semáforo com valor inicial "value"
int sem_create(semaphore_t *s, int value)
{
    if(s ==NULL)
        return -1;

    s->status = Normal;
    s->count = value;
    
    return 0;
}

// requisita o semáforo
int sem_down(semaphore_t *s)
{
    //desativa preempcao
    preempcao = 0;

    //semaforo inexistente ou destruído
    if (s == NULL || s->status == Destroyed){
        preempcao =1;
        return -1;
    }

    //se houver, decrementa o contador
    s->count -= 1;

    //se nao houver recurso, suspende a tarefa
    if (s->count < 0)
    {
        task_suspend(NULL, &s->queue);
    }


    //caso o semaforo foi destruido dps da tarefa ja estar fila
    if (s == NULL || s->status == Destroyed){
        preempcao = 1;
        return -1;
    }
    
    return 0;
}

// libera o semáforo
int sem_up(semaphore_t *s)
{

    //semaforo inexistente ou destruído
    if (s == NULL || s->status == Destroyed)
        return -1;

    //incrementa o contador
    s->count += 1;    

    //acorda a primeira tarefa da fila do semaforo (se houver)
    if (s->count <= 0)
    {
        if(s->queue==NULL)
                printf("Tentando resumir tarefa nula no up");
        task_resume(s->queue);
    }

    //reativa preempcao
    preempcao = 1;

    return 0;
}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy(semaphore_t *s)
{

    int count = queue_size((queue_t *)s->queue);
    //acorda todas as tarefas que dependem do semaforo
    if (count != 0)
    {
        task_t *atual = s->queue;
        task_t *q_next;

        for (int i = 0; i < count; i++)
        {
            q_next = atual->next;

            if(atual==NULL)
                printf("Tentando resumir tarefa nula no destroy");
            task_resume(atual);

            atual = q_next;
        }
    }

    //destroi o semaforo (USAR FREE?)    
    s->status=Destroyed;

    return 0;
}


/////////////////////////// BARRIERS////////////////////


// Inicializa uma barreira
int barrier_create (barrier_t *b, int N){

    if(b == NULL || N <=0 )
        return -1;

    b->status = Normal;
    b->max = N-1;
    b->count = N-1;

    return 0;

}

// Chega a uma barreira
int barrier_join (barrier_t *b) {
    
    sem_down(&s_bar);    

    if(b == NULL || b->status == Destroyed){
        return -1;
        sem_up(&s_bar);
    }

    if(b->count > 0){
        b->count -= 1;
        sem_up(&s_bar);
        task_suspend(task_atual, &b->queue);
        return 0;
    }
    //acorda todas as tarefas da barreira
    else
    {
        task_t *atual = b->queue;
        task_t *q_next;

        for (int i = 0; i < b->max; i++)
        {
            q_next = atual->next;

            if(atual==NULL)
                printf("Tentando resumir tarefa nula no destroy");
            task_resume(atual);

            atual = q_next;
        }    

        b->count = b->max;
        sem_up(&s_bar);
        return 0;
    }
    

}

// Destrói uma barreira
int barrier_destroy (barrier_t *b){
    
    if(b == NULL){
        return -1;
    }

    sem_down(&s_bar);
    int count = queue_size((queue_t *)b->queue);
    //acorda todas as tarefas que dependem da barreira
    if (count != 0)
    {
        task_t *atual = b->queue;
        task_t *q_next;

        for (int i = 0; i < count; i++)
        {
            q_next = atual->next;

            if(atual==NULL)
                printf("Tentando resumir tarefa nula no destroy");
            atual->error_code = BarrierDestroyed;
            task_resume(atual);

            atual = q_next;
        }
    }

    //destroi o semaforo
    b->status=Destroyed;

    sem_up(&s_bar);
    return 0;

}



/////////////////////////////////////////////////////////////

void dispatcher_body(void *arg)
{

    task_t *next;
    while (userTasks >= 0)
    {

        int count = queue_size((queue_t *)sleep_queue);
        //acorda todas as tarefas dormentes que já estouraram o tempo
        if (count != 0)
        {
            task_t *atual = sleep_queue;
            task_t *q_next;

            for (int i = 0; i < count; i++)
            {
                q_next = atual->next;

                if (atual->wake_time <= systime())
                {
                    task_resume(atual);
                }

                atual = q_next;
            }
        }

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
            if (next->status == Finished && next->context.uc_stack.ss_sp != NULL){                
                free(next->context.uc_stack.ss_sp);
                next->context.uc_stack.ss_sp = NULL;
            }
        }
    }
    task_exit(0); // encerra a tarefa dispatcher
}

task_t *scheduler()
{

    //não há tarefas
    if (queue_size((queue_t *)ready_queue) == 0)
    {
        //printf("SchedErr: fila de prontas vazias\n");
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

    if (task_atual->type == User && preempcao == 1)
    {
        curr_quantum--;
        if (curr_quantum <= 0 )
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