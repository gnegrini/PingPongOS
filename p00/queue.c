//------------------------------------------------------------------------------
// Definição e operações em uma fila genérica.
// Implementação por Giovane Negrini e Henrique Kreuzner
// Sistemas Operacionais - 2019.1
//------------------------------------------------------------------------------

#include "queue.h"
#include <stdio.h>


//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem)
{
    if (queue == NULL)
    {
        printf("E: Fila nao existente\n");
        return;
    }
    
    
    if (elem == NULL)
    {
        printf("E: Elemento nao existe\n");
        return;
    }

    if (elem->prev != NULL || elem->next != NULL)
    {
        printf("E: Elemento esta em outra fila\n");
        return;
    }
    
    queue_t *atual = *queue;

    if (*queue == NULL)
    {
        printf("G: A fila esta vazia\n");
        
        // First aponta para o elemento
        *queue = elem;

        // Elemento aponta para ele mesmo
        elem->next = elem;
        elem->prev = elem;
    }

    // Percorre o elemento ate o ultimo elemento
    else
    {        
        do    
        {
            atual = atual->next;
        } while (atual->next != *queue);

        // Faz o novo apontar para o anterior
        elem->prev = atual;

        // Faz o novo apontar para o primeiro
        elem->next = *queue;

        // Faz o anterior apontar para o novo
        atual->next = elem;

        // Faz o inicio apontar para o final
        (*queue)->prev = elem;
    }    
    
}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila
int queue_size (queue_t *queue)
{
    // Se a fila estiver vazia
    if(queue == NULL)
    {
        return 0;
    }

        
    int counter = 0;
    queue_t *atual = queue;    
    
    // Incrementa o contador e passa para o proximo elemento
    do
    {        
        counter++;
        atual = atual->next;
    } while (atual != queue);    
    
    return counter;

}


//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
    if (queue == NULL)
    {
        printf("E: Fila nao existe\n");
        return NULL;
    }
    
    if(*queue == NULL)
    {
        printf("E: Fila vazia\n");
        return NULL;
    }

    if(elem == NULL)
    {
        printf("E: Elemento nao existente\n");
        return NULL;
    }
    
    // Fila com mais elementos
            
    queue_t *atual = *queue;
    
    // Procura o elemento
    while (atual!= elem)
    {
        atual = atual->next;
        
        // Se fez uma volta completa
        if (atual == *queue) {
            printf("E: Elemento nao esta na fila\n");
            return NULL;
        }
        
    }

    //           Realiza a remocao
    // Faz anterior apontar pro proximo
    (atual->prev)->next = atual->next;

    // Faz proximo apontar pro anterior
    (atual->next)->prev = atual->prev;

    // Caso seja o primeiro da fila, corrige o ponteiro de inicio
    if (*queue == elem)
    {
        // Fila com somente um elemento
        if (elem->next == elem)
        {
            *queue = NULL;
        }
        else
        {
            *queue = elem->next;
        }
        
    }

    // Anula os next e prox do elemento
    elem->next = NULL;
    elem->prev = NULL;        

    return atual;    

}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
    

    if (queue == NULL)
    {
        printf("%s[]\n", name);        
        return;
    }
    

    queue_t *atual = queue;
    
    printf("%s[", name);
    
    // Procura o elemento
    do
    {               
        print_elem(atual);
        printf(" ");
        atual = atual->next;
        
    } while (atual!= queue);

    printf("]\n");
    
    
}