/** @file
   Implementacja wczytywania poleceń

   @date 2017-05-11
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parse.h"
#include "utils.h"

/**
 * Sprawdza czy liczba wielomianów na stosie jest wystarczająca
 * @param[in] n_requires_n_polys : minimalna liczba wielomianów
 */
#define REQUIRES_N_POLYNOMIALS(n_requires_n_polys)\
if (StackSize(poly_stack) < (n_requires_n_polys))\
{\
    fprintf(stderr, "ERROR %u STACK UNDERFLOW\n", stream->line_number);\
    return;\
}

/**
 * Dodaje zerowy wielomian na wierzchołek stosu
 * 
 * Nie wymaga wielomianów na stosie
 * @param[in,out] poly_stack : stos wielomianów
 */
static inline void CommandZero(Stack *poly_stack)
{
    Poly *poly_zero = malloc(sizeof(Poly));
    assert(poly_zero != NULL);
    *poly_zero = PolyZero();

    StackPush(poly_stack, poly_zero);
}

/**
 * Wypisuje (na standardowe wyjście) czy wielomian jest współczynnikiem
 * (1 - tak, 0 - nie)
 *
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in] poly_stack : stos wielomianów
 */
static inline void CommandIsCoefficient(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(1)

    if (PolyIsCoeff(StackTop(poly_stack)))
    {
        printf("1\n");
    }
    else {
        printf("0\n");
    }
}

/**
 * Wypisuje (na standardowe wyjście) czy wielomian jest zerem
 * (1 - tak, 0 - nie)
 * 
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in] poly_stack : stos wielomianów
 */
static inline void CommandIsZero(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(1)

    if (PolyIsZero(StackTop(poly_stack)))
    {
        printf("1\n");
    }
    else {
        printf("0\n");
    }
}

/**
 * Robi głęboką kopię wielomianu na wierzchołku stosu
 * 
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in,out] poly_stack : stos wielomianów
 */
static inline void CommandClone(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(1)

    Poly *result = malloc(sizeof(Poly));
    assert(result != NULL);
    *result = PolyClone(StackTop(poly_stack));

    StackPush(poly_stack, result);
}

/**
 * Zdejmuje dwa wielomiany z wierzchołka stosu, dodaje je do siebie
 * i dodaje wynik na wierzchołek stosu
 * 
 * Wymaga 2 wielomianów na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in,out] poly_stack : stos wielomianów
 */
static inline void CommandAdd(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(2)

    Poly *q = StackTop(poly_stack);
    Poly *p = StackPeek(poly_stack);

    PolyAddInPlace(p, q);

    StackPop(poly_stack);
    free(q);
}

/**
 * Zdejmuje dwa wielomiany z wierzchołka stosu, mnoży je ze sobą
 * i dodaje wynik na wierzchołek stosu
 * 
 * Wymaga 2 wielomianów na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in,out] poly_stack : stos wielomianów
 */
static inline void CommandMul(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(2)

    Poly *q = StackTop(poly_stack);
    StackPop(poly_stack);

    Poly *p = StackTop(poly_stack);
    StackPop(poly_stack);

    Poly *result = malloc(sizeof(Poly));
    assert(result != NULL);
    *result = PolyMul(p, q);

    PolyDestroy(p);
    PolyDestroy(q);
    free(p);
    free(q);

    StackPush(poly_stack, result);
}

/**
 * Zdejmuje wielomian z wierzchołka stosu, dodaje wielomian przeciwny do
 * zdjętego na wierzchołek stosu
 * 
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in,out] poly_stack : stos wielomianów
 */
static inline void CommandNeg(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(1)

    Poly *old_poly = StackTop(poly_stack);
    Poly result = PolyNeg(old_poly);

    PolyDestroy(old_poly);

    *old_poly = result;
}

/**
 * Zdejmuje dwa wielomiany z wierzchołka stosu, odejmuje je od siebie
 * ( 2 od góry - 1 od góry )
 * i dodaje wynik na wierzchołek stosu
 * 
 * Wymaga 2 wielomianów na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in,out] poly_stack : stos wielomianów
 */
static inline void CommandSub(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(2)

    Poly *q = StackTop(poly_stack);
    StackPop(poly_stack);

    Poly *p = StackTop(poly_stack);
    StackPop(poly_stack);

    Poly *result = malloc(sizeof(Poly));
    assert(result != NULL);
    *result = PolySub(q, p);

    PolyDestroy(p);
    PolyDestroy(q);
    free(p);
    free(q);

    StackPush(poly_stack, result);
}

/**
 * Sprawdza czy dwa wielomiany (licząc od góry stosu) są równe
 * (wypisuje 1 - tak, 0 - nie)
 * 
 * Wymaga 2 wielomianów na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in] poly_stack : stos wielomianów
 */
static inline void CommandIsEq(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(2)

    if (PolyIsEq(StackTop(poly_stack), StackPeek(poly_stack)))
    {
        printf("1\n");
    }
    else {
        printf("0\n");
    }
}

/**
 * Wypisuje stopień wielomianu z wierzchołka stosu
 * 
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in] poly_stack : stos wielomianów
 */
static inline void CommandDeg(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(1)

    Poly *p = StackTop(poly_stack);
    printf("%d\n", PolyDeg(p));
}

/**
 * Wypisuje stopień wielomianu z wierzchołka stosu wg. zmiennej @p var
 * 
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in] poly_stack : stos wielomianów
 * @param[in] var : zmienna wg. której stopień jest liczony
 */
static inline void CommandDegBy(InputStream *stream,
                         Stack *poly_stack,
                         unsigned var)
{
    REQUIRES_N_POLYNOMIALS(1)

    printf("%d\n", PolyDegBy(StackTop(poly_stack), var));
}

/**
 * Składa wielomian z wierzchołka stosu z @p count wielomianami pod nim
 * 
 * Wymaga count + 1 wielomianów na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in] poly_stack : stos wielomianów
 * @param[in] count : liczba wielomianów do wstawienia
 */
static inline void CommandCompose(InputStream *stream,
                                   Stack *poly_stack,
                                   unsigned count)
{
    // Count może być UINT_MAX
    // Sprawdzamy dwa razy, żeby uniknąć przekręceń
    REQUIRES_N_POLYNOMIALS(count)
    REQUIRES_N_POLYNOMIALS(count+1)

    Poly *tab = calloc(count, sizeof(Poly));
    assert(tab != NULL);

    Poly target_poly = *(Poly*)StackTop(poly_stack);
    free(StackTop(poly_stack));
    StackPop(poly_stack);

    for (unsigned i = 0; i < count; ++i)
    {
        tab[i] = *(Poly*)StackTop(poly_stack);
        free(StackTop(poly_stack));
        StackPop(poly_stack);
    }
    Poly *result = malloc(sizeof(Poly));
    assert(result != NULL);
    *result = PolyCompose(&target_poly, count, tab);

    PolyDestroy(&target_poly);
    for (unsigned i = 0; i < count; ++i)
    {
        PolyDestroy(&tab[i]);
    }
    free(tab);

    StackPush(poly_stack, result);
}

/**
 * Zdejmuje wielomian z wierzchołka stosu, liczy jego wartość w value i
 * dodaje wynik na wierzchołek stosu
 * 
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in,out] poly_stack : stos wielomianów
 * @param[in] value : liczba do podstawienia
 */
static inline void CommandAt(InputStream *stream,
                      Stack *poly_stack,
                      poly_coeff_t value)
{
    REQUIRES_N_POLYNOMIALS(1)

    Poly *last_poly = StackTop(poly_stack);
    Poly *result = malloc(sizeof(Poly));
    assert(result != NULL);
    *result = PolyAt(last_poly, value);

    PolyDestroy(last_poly);
    free(last_poly);
    StackPop(poly_stack);

    StackPush(poly_stack, result);
}

/**
 * Wypisuje wielomian z wierzchołka stosu
 * 
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in] poly_stack : stos wielomianów
 */
static inline void CommandPrint(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(1)

    PolyPrint(StackTop(poly_stack));
    printf("\n");
}

/**
 * Zdejmuje wielomian z wierzchołka stosu
 * 
 * Wymaga 1 wielomianu na stosie
 * @param[in,out] stream : wskaźnik na InputStream z którego polecenie zostało
 *                         wczytane
 * @param[in,out] poly_stack : stos wielomianów
 */
static inline void CommandPop(InputStream *stream, Stack *poly_stack)
{
    REQUIRES_N_POLYNOMIALS(1)

    PolyDestroy(StackTop(poly_stack));
    free(StackTop(poly_stack));
    StackPop(poly_stack);
}

/**
 * Wczytuje i wykonuje polecenia
 * @param[in,out] stream : wskaźnik na InputStream do czytania poleceń
 * @param[in,out] poly_stack : stos wielomianów
 */
void ReadAndExecuteCommand(InputStream *stream, Stack *poly_stack)
{
    char *command = calloc(MAX_COMMAND_LENGTH + 1, sizeof(char));
    assert(command != NULL);

    unsigned command_length = 0;
    char c;

    while (EOF != (c = ReadCharacter(stream)))
    {
        if (c == ' ' || c == '\n')
        {
            break;
        }

        if (command_length >= MAX_COMMAND_LENGTH ||
            IsValidCommandCharacter(c) == false)
        {
            fprintf(stderr, "ERROR %u WRONG COMMAND\n",
                    stream->line_number + 1);
            SkipLine(stream);

            free(command);
            return;
        }
        else {
            command[command_length] = c;
            ++command_length;
        }
    }

    if (strcmp(command, COMMAND_ZERO) == 0 && c == '\n')
    {
        CommandZero(poly_stack);
    }
    else if (strcmp(command, COMMAND_IS_COEFF) == 0 && c == '\n')
    {
        CommandIsCoefficient(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_IS_ZERO) == 0 && c == '\n')
    {
        CommandIsZero(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_CLONE) == 0 && c == '\n')
    {
        CommandClone(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_ADD) == 0 && c == '\n')
    {
        CommandAdd(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_MUL) == 0 && c == '\n')
    {
        CommandMul(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_NEG) == 0 && c == '\n')
    {
        CommandNeg(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_SUB) == 0 && c == '\n')
    {
        CommandSub(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_IS_EQ) == 0 && c == '\n')
    {
        CommandIsEq(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_DEG) == 0 && c == '\n')
    {
        CommandDeg(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_DEG_BY) == 0)
    {
        if (c == ' ')
        {
            unsigned var = ReadDegByCommandArgument(stream);
            if (!stream->parse_error)
            {
                CommandDegBy(stream, poly_stack, var);
            }
        }
        else {
            if (c != '\n')
            {
                SkipLine(stream);
            }
            fprintf(stderr, "ERROR %u WRONG VARIABLE\n", stream->line_number);
        }
    }
    else if (strcmp(command, COMMAND_COMPOSE) == 0)
    {
        if (c == ' ')
        {
            unsigned var = ReadComposeCommandArgument(stream);
            if (!stream->parse_error)
            {
                CommandCompose(stream, poly_stack, var);
            }
        }
        else {
            if (c != '\n')
            {
                SkipLine(stream);
            }
            fprintf(stderr, "ERROR %u WRONG COUNT\n", stream->line_number);
        }
    }
    else if (strcmp(command, COMMAND_AT) == 0)
    {
        if (c == ' ')
        {
            poly_coeff_t value = ReadAtCommandArgument(stream);
            if (!stream->parse_error)
            {
                ReadCharacter(stream);
                CommandAt(stream, poly_stack, value);
            }
        }
        else {
            if (c != '\n')
            {
                SkipLine(stream);
            }
            stream->parse_error = true;
            fprintf(stderr, "ERROR %u WRONG VALUE\n", stream->line_number);
        }
    }
    else if (strcmp(command, COMMAND_PRINT) == 0 && c == '\n')
    {
        CommandPrint(stream, poly_stack);
    }
    else if (strcmp(command, COMMAND_POP) == 0 && c == '\n')
    {
        CommandPop(stream, poly_stack);
    }
    else {
        if(c != '\n')
        {
            SkipLine(stream);
        }
        fprintf(stderr, "ERROR %u WRONG COMMAND\n", stream->line_number);
    }

    free(command);
}
