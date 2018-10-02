/** @file
   Implementacja stosu

   @date 2017-05-19
*/

#ifndef __STACK_H__
#define __STACK_H__

#include <stdbool.h>
#include <assert.h>
#include "utils.h"

#define INITIAL_STACK_CAPACITY 16
///< Początkowy rozmiar tablicy w ktorej przechowywane są elementy stosu

/**
 * Struktura przechowująca stos wskaźników
 */
typedef struct Stack
{
    unsigned size; ///< Ilość elementów obecnie na stosie
    unsigned array_size; ///< Rozmiar tablicy przechowywującej elementy
    void **array; ///< Wskaźnik na tablicę przechowywującą elementy
} Stack;

/**
 * Funkcja tworząca nowy stos
 * @return Pusty Stos
 */
static inline Stack StackInit()
{
    Stack s;
    s.size = 0;
    s.array_size = INITIAL_STACK_CAPACITY;
    s.array = calloc(s.array_size, sizeof(void *));
    assert(s.array != NULL);
    return s;
}

/**
 * Zwraca wartość z wierzchołka stosu
 * @param[in] s : stos
 */
static inline void* StackTop(Stack *s)
{
    assert(s != NULL && s->size != 0);
    return s->array[s->size - 1];
}

/**
 * Zwraca drugi od góry element stosu
 * @param[in] s : stos
 */
static inline void* StackPeek(Stack *s)
{
    assert(s != NULL && s->size > 1);
    return s->array[s->size - 2];
}

/**
 * Usuwa element z wierzchołka stosu
 * @param[in] s : stos
 */
static inline void StackPop(Stack *s)
{
    assert(s != NULL && s->size != 0);
    --s->size;
}

/**
 * Zwraca ilość elementów na stosie
 * @param[in] s : stos
 */
static inline size_t StackSize(Stack *s)
{
    assert(s != NULL);
    return s->size;
}

/**
 * Dodaje element (wskaźnik) na wierzchołek stosu
 * @param[in] s : stos
 * @param[in] v : wskaźnik
 */
static inline void StackPush(Stack *s, void *v)
{
    assert(s != NULL && s->array != NULL);
    if (s->size < s->array_size)
    {
        s->array[s->size] = v;
        ++s->size;
    }
    else {
        s->array_size *= 2;
        s->array = realloc(s->array, s->array_size * sizeof(void*));
        assert(s->array != NULL);
        StackPush(s, v);
    }
}

/**
 * Usuwa stos z pamięci
 * 
 * Jeżeli destructor_ptr nie jest NULL, wywołuje destructor_ptr na każdym
 * elemencie stosu, po czym zwalnia pamięć na którą dany element wskazuje.
 * W przeciwnym wypadku, pamięć na którą wskazują poszczególne elementy
 * nie jest zwalaniana
 * @param[in] s : stos
 * @param[in] destructor_ptr : wskaźnik na funckje przyjmujacą wskaźnik (void*)
 * i nic nie zwracającą
 */
static inline void StackDestroy(Stack *s, void *destructor_ptr)
{
    assert(s != NULL && s->array != NULL);
    if (destructor_ptr != NULL)
    {
        void (*destructor)(void*) = destructor_ptr;
        for (unsigned i = 0; i < s->size; ++i)
        {
            (*destructor)(s->array[i]);
            free(s->array[i]);
        }
    }
    free(s->array);
}

#endif /* __STACK_H__ */
