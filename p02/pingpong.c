#include "datatypes.h"		// estruturas de dados necessárias
#include "stdio.h"

// funções gerais ==============================================================

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init ()
{
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0) ;
}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg)			// argumentos para a tarefa
{

}
// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{

}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{

}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id ()
{

}


////////////// IMPLEMENTAR NA PROXIMA SEMANA /////////////

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL
void task_suspend (task_t *task, task_t **queue) ;

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume (task_t *task) ;

