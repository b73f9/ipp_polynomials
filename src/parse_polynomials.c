/** @file
   Implementacja wczytywania wielomianu

   @date 2017-05-11
*/
#include <stdlib.h>
#include <stdio.h>
#include "parse.h"
#include "utils.h"

/**
 * Wychodzi z funkcji wczytującej wielomian jeżeli
 * predykat jest spełniony.
 * 
 * Zwalnia zaalokowany stos
 * @param[in] condition_parse_poly_exit_if : predykat
 */
#define PARSE_POLY_EXIT_IF(condition_parse_poly_exit_if)\
if (condition_parse_poly_exit_if)\
{\
    StackDestroy(&parse_stack, &StackOfMonosDestructor);\
    return PolyZero();\
}\

/**
 * Wychodzi z funkcji wczytującej wielomian i wypisuje błąd
 * jeżeli predykat nie jest spełniony.
 * 
 * Zwalnia zaalokowany stos i ustawia flagę parse_error
 * @param[in] condition_parse_poly_expect : predykat
 */
#define PARSE_POLY_EXPECT(condition_parse_poly_expect) \
if ((condition_parse_poly_expect) == false)\
{\
    fprintf(stderr, "ERROR %u %u\n", stream->line_number + 1,\
            stream->column_number + 1);\
    SkipLine(stream);\
    stream->parse_error = true;\
\
    PARSE_POLY_EXIT_IF(true)\
}\

/**
 * Alokuje i tworzy nowy stos.
 * @return wskażnik na wynik
 */
static inline Stack* NewStack()
{
    Stack *s = malloc(sizeof(Stack));
    assert(s != NULL);
    *s = StackInit();
    return s;
}

/**
 * Alokuje nowy jednomian i przypisuje mu wartość @p v
 * @param[in] v : wartość nowego jednomianu
 * @return wskaźnik na wynik
 */
static inline Mono* NewMono(Mono v)
{
    Mono *m = calloc(1, sizeof(Mono));
    assert(m != NULL);
    *m = v;
    return m;
}

/**
 * Usuwa stos @p s jednomianów z pamięci
 * @param[in] s : stos
 */
static inline void StackOfMonosDestructor(Stack *s)
{
    StackDestroy(s, &MonoDestroy);
}

/**
 * Zmienia stos jednomianów w wielomian będący ich sumą
 * @param[in] s : stos
 * @return wynikowy wielomian
 */
static inline Poly CollapseMonoStackIntoAPoly(Stack *s)
{
    unsigned mono_count = StackSize(s);
    if (mono_count == 0)
    {
        StackDestroy(s, &MonoDestroy);
        return PolyZero();
    }

    Mono *arr = calloc(mono_count, sizeof(Mono));
    assert(arr != NULL);

    while (StackSize(s))
    {
        arr[StackSize(s) - 1] = *(Mono *)StackTop(s);

        free(StackTop(s));
        StackPop(s);
    }
    StackDestroy(s, NULL);

    Poly temp = PolyAddMonos(mono_count, arr);
    free(arr);
    return temp;
}

Poly ReadPolynomial(InputStream *stream)
{
    bool expecting_mono = false;
    Stack parse_stack = StackInit();
    StackPush(&parse_stack, NewStack());

    PARSE_POLY_EXPECT(IsValidNumberCharacter(PeekCharacter(stream)) ||
                      PeekCharacter(stream) == '(')
    while (PeekCharacter(stream) != '\n')
    {
        if (PeekCharacter(stream) == '(')
        {
            ReadCharacter(stream);

            StackPush(&parse_stack, NewStack());
            expecting_mono = false;
            PARSE_POLY_EXPECT(IsValidNumberCharacter(PeekCharacter(stream)) ||
                              PeekCharacter(stream) == '(')
        }
        else if (IsValidNumberCharacter(PeekCharacter(stream)) &&
                 expecting_mono == false)
        {
            poly_coeff_t coeff = ReadPolyCoefficient(stream);
            PARSE_POLY_EXIT_IF(stream->parse_error)

            if (coeff != 0)
            {
                Poly c = PolyFromCoeff(coeff);
                Mono *m = NewMono(MonoFromPoly(&c, 0));
                StackPush(StackTop(&parse_stack), m);
            }
        }
        else if (PeekCharacter(stream) == ',' && expecting_mono == false &&
                 StackSize(&parse_stack) > 1)
        {

            ReadCharacter(stream);
            poly_exp_t exponent = ReadExponent(stream);
            PARSE_POLY_EXIT_IF(stream->parse_error)
            PARSE_POLY_EXPECT(StackSize(&parse_stack) > 1)

            Poly p = CollapseMonoStackIntoAPoly(StackTop(&parse_stack));
            free(StackTop(&parse_stack));
            StackPop(&parse_stack);

            if (!PolyIsZero(&p))
            {
                Mono *m = NewMono(MonoFromPoly(&p, exponent));
                StackPush(StackTop(&parse_stack), m);
            }
            else {
                PolyDestroy(&p);
            }

            PARSE_POLY_EXPECT(PeekCharacter(stream)==')')
            ReadCharacter(stream);

            if (PeekCharacter(stream) == '+')
            {
                ReadCharacter(stream);
                expecting_mono = true;
            }
            else {
                PARSE_POLY_EXPECT(PeekCharacter(stream) == '\n' ||
                                  PeekCharacter(stream) == ',')
            }
        }
        else {
            PARSE_POLY_EXPECT(false)
        }
    }
    PARSE_POLY_EXPECT(StackSize(&parse_stack) == 1 && expecting_mono == false)
    ReadCharacter(stream);

    Poly result = CollapseMonoStackIntoAPoly(StackTop(&parse_stack));
    free(StackTop(&parse_stack));
    StackDestroy(&parse_stack, NULL);

    return result;
}
