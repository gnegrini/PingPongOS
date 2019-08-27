#include "datatypes.h" // estruturas de dados necessárias
#include "stdio.h"

task_t task_main;
task_t *task_atual;
int next_id;

char *stack;
// funções gerais ==============================================================

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
        perror("Erro na cria��o da pilha: ");
        return (-1);
    }

    //talvez temos que checar o retorno da make context para retornar se deu erro
    makecontext(&(task->context), (void *)(*start_func), 1, arg);

    //atribui id para a task e atualiza o topo
    task->tid = next_id;
    next_id++;

    return task->tid;
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    task->prev = task_atual;
    task_atual->next = task;
    task_atual = task;

    int rt = swapcontext(&(task->prev->context), &(task->context));

    if (rt == -1)
        return rt;
    else
        return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit(int exitCode)
{
    task_switch(&task_main);
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id()
{
    return task_atual->tid;
}

////////////// IMPLEMENTAR NA PROXIMA SEMANA /////////////

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL
void task_suspend(task_t *task, task_t **queue);

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume(task_t *task);
