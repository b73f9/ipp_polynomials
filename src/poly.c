/** @file
   Implementacja klasy wielomianów

   @date 2017-05-25
*/

#include "poly.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "stack.h"
#include "utils.h"

/**
 * Struktura przechowująca stan składania wielomianów
 */
typedef struct ComposeState
{
    Poly result; ///< Dotychczasowy wynik
    Mono *mono; ///< Przetwarzany jednomian
} ComposeState;

/**
 * Funkcja tworząca nowy obiekt przechowywujący informacje o składaniu
 *
 * @param[in] mono : jednomian
 * @param[in] constant : stała
 */
ComposeState* NewComposeState(Mono *mono, poly_coeff_t constant){
    ComposeState *new_state = malloc(sizeof(ComposeState));
    assert(new_state != NULL);
    new_state->result = PolyFromCoeff(constant);
    new_state->mono = mono;
    return new_state;
}

/**
 * Właściwa funkcja odpowiedzialna za wypisywanie wielomianu
 * @param[in] p : wielomian
 * @param[in] constant : stała wielomianu nadrzędnego
 */
static void PolyPrintWithConstant(const Poly *p, poly_coeff_t constant)
{
    constant += p->constant;

    if (PolyIsCoeff(p))
    {
        printf("%ld", constant);
        return;
    }

    if (constant != 0 && p->first_mono->exp != 0)
    {
        printf("(%ld,0)+", constant);
    }

    Mono *current_mono = p->first_mono;
    while (current_mono != NULL)
    {
        printf("(");
        if (current_mono->exp == 0)
        {
            PolyPrintWithConstant(&current_mono->p, constant);
        }
        else {
            PolyPrintWithConstant(&current_mono->p, 0);
        }
        printf(",%u)", current_mono->exp);

        if (current_mono->next_mono != NULL)
            printf("+");

        current_mono = current_mono->next_mono;
    }
}

void PolyPrint(const Poly *p)
{
    PolyPrintWithConstant(p, 0);
}

/**
 * Zwraca większy z dwóch wykładników
 * @param[in] a : wykładnik jednomianu
 * @param[in] b : wykładnik jednomianu
 * @return większy z wykładników a i b
 */
static inline poly_exp_t Max(poly_exp_t a, poly_exp_t b)
{
    return (a > b) ? a : b;
}

/**
 * Szybkie potęgowanie współczynnika
 *
 * Implementuje algorytm szybkiego potęgowania w wersji iteracyjnej
 * @param[in] x : liczbowy współczynnik wielomianu
 * @param[in] n : potęga do której współczynnik ma być podniesiony
 * @return `x^n`
 */
static inline poly_coeff_t FastCoeffPow(poly_coeff_t x, poly_exp_t n)
{
    poly_coeff_t result = 1;
    while (n != 0)
    {
        if (n % 2 == 1)
        {
	        result *= x;
		}
        n /= 2;
        x *= x;
    }

    return result;
}

/**
 * Komparator dla typu Mono
 *
 * Porównuje wykładaniki dwóch jednomianów
 * @param[in] left_v : Jednomian
 * @param[in] right_v : Jednomian
 * @return 0 dla left_v.exp = right_v.exp, 1 dla left_v.exp > right_v.exp,
 * -1 dla left_v.exp < right_v.exp
 */
static int MonoCompare(const void *left_v, const void *right_v)
{
    const Mono *left  = (const Mono *)left_v;
    const Mono *right = (const Mono *)right_v;

    if (left->exp > right->exp)
    {
        return 1;
    }

    if (left->exp < right->exp)
    {
        return -1;
    }

    return 0;
}

/**
 * Sprawdza czy tablica jednomianów jest posortowana
 * @param[in] count : liczba elementów tablicy @p monos
 * @param[in] monos : tablica jednomianów
 * @return True dla posortowanej tablicy, False w przeciwnym wypadku
 */
static bool MonosAreSorted(unsigned count, const Mono monos[])
{
    for (unsigned i = 1; i < count; ++i)
    {
        if (MonoCompare(&monos[i-1], &monos[i]) > 0)
        {
            return false;
        }
    }

    return true;
}

/**
 * Zlicza liczbę jednomianów w wielomianie
 * @param[in] p : Wielomian
 * @return Liczba jednomianów z których składa się wielomian
 * (bez jednomianu stałego)
 */
static unsigned MonoCount(const Poly *p)
{
    unsigned count = 0;

    Mono *current_mono = p->first_mono;
    while (current_mono != NULL)
    {
        ++count;
        current_mono = current_mono->next_mono;
    }

    return count;
}

/**
 * Zwraca głęboką kopię listy jednomianów
 * @param[in] m : lista jednomianów
 * @return Głęboka kopia listy jednomianów
 */
static Mono* MonoListClone(Mono *m)
{
    if (m == NULL)
    {
        return NULL;
    }

    Mono *result = malloc(sizeof(Mono));
    assert(result != NULL);
    *result = MonoClone(m);

    Mono *last_copied_mono = result;
    Mono *current_mono = m->next_mono;
    while (current_mono != NULL)
    {
        last_copied_mono->next_mono = malloc(sizeof(Mono));
        assert(last_copied_mono->next_mono != NULL);
        *(last_copied_mono->next_mono) = MonoClone(current_mono);

        current_mono = current_mono->next_mono;
        last_copied_mono = last_copied_mono->next_mono;
    }

    return result;
}

/**
 * Zwraca głęboką kopię listy jednomianów, pomnożoną przez stałą
 * @param[in] first_mono : Wskaźnik na pierwszy element listy
 * @param[in] constant : Stała przez którą lista ma być pomnożona
 * @param[out] array : Wskaźnik na pierwszy element tablicy w której
 * znajdzie się kopia
 */
static void CloneMonosMultipliedByAConstant(
             Mono *first_mono, poly_coeff_t constant, Mono *array)
{
    Poly *const_poly = malloc(sizeof(Poly));
    assert(const_poly != NULL);
    *const_poly = PolyFromCoeff(constant);

    unsigned i = 0;
    Mono *current_mono = first_mono;
    while (current_mono != NULL)
    {
        array[i].p   = PolyMul(&current_mono->p, const_poly);
        array[i].exp = current_mono->exp;

        ++i;
        current_mono = current_mono->next_mono;
    }

    PolyDestroy(const_poly);
    free(const_poly);
}

/**
 * Usuwa jednomiany tożsamościowo równe zeru z listy
 * jednomianów wielomianu @p p
 * @param[in,out] p : Wielomian
 */
static void RemoveEmptyMonosFromPoly(Poly * const p)
{
    Mono *first_nonempty_mono = p->first_mono;
    while (first_nonempty_mono != NULL &&
          PolyIsZero(&(first_nonempty_mono->p)) == true)
    {
        Mono * const next_mono = first_nonempty_mono->next_mono;
        free(first_nonempty_mono);
        first_nonempty_mono = next_mono;
    }

    p->first_mono = first_nonempty_mono;

    if (first_nonempty_mono != NULL)
    {
        Mono *last_nonempty_mono = first_nonempty_mono;
        Mono *current_mono       = p->first_mono->next_mono;
        while (current_mono != NULL)
        {
            Mono* const next_mono = current_mono->next_mono;

            if (PolyIsZero(&current_mono->p) == false)
            {
                last_nonempty_mono->next_mono = current_mono;
                last_nonempty_mono = current_mono;
            }
            else {
                last_nonempty_mono->next_mono = NULL;
                free(current_mono);
            }

            current_mono = next_mono;
        }
    }
}

/**
 * "Szybkie" potęgowanie wielomianów
 *
 * Implementuje algorytm szybkiego potęgowania w wersji iteracyjnej
 * @param[in] p : wielomian
 * @param[in] n : potęga do której wielomian ma być podniesiony
 * @return `p^n`
 */
static inline Poly FastPolyPow(const Poly *p, poly_exp_t n)
{
    if (PolyIsCoeff(p))
    {
        return PolyFromCoeff(FastCoeffPow(p->constant, n));
    }

    Poly result = PolyFromCoeff(1);
    Poly x = PolyClone(p);
    while (n != 0)
    {
        if (n % 2 == 1)
        {
            Poly new_result = PolyMul(&result, &x);
            PolyDestroy(&result);
            result = new_result;
        }
        n /= 2;
        Poly new_x = PolyMul(&x, &x);
        PolyDestroy(&x);
        x = new_x;
    }
    PolyDestroy(&x);
    return result;
}

Poly PolyCompose(const Poly *p, unsigned count, const Poly x[]) {
    Stack calc_stack = StackInit();
    StackPush(&calc_stack, NewComposeState(p->first_mono, p->constant));
    while (StackSize(&calc_stack) > 1 ||
           ((ComposeState*)StackTop(&calc_stack))->mono != NULL)
    {
        assert(StackSize(&calc_stack) != 0);
        ComposeState *current_state = StackTop(&calc_stack);

        if (current_state->mono == NULL || StackSize(&calc_stack) > count)
        {
            Poly lower_result = current_state->result;
            free(current_state);
            StackPop(&calc_stack);
            if (StackSize(&calc_stack) == 0)
            {
                StackDestroy(&calc_stack, NULL);
                return lower_result;
            }

            ComposeState *next_state = StackTop(&calc_stack);
            Poly poly_power = FastPolyPow(&x[StackSize(&calc_stack) - 1],
                                          next_state->mono->exp);
            Poly result = PolyMul(&lower_result, &poly_power);
            PolyDestroy(&poly_power);
            PolyDestroy(&lower_result);

            PolyAddInPlace(&next_state->result, &result);
            next_state->mono = next_state->mono->next_mono;
            continue;
        }

        StackPush(&calc_stack,
                  NewComposeState(current_state->mono->p.first_mono,
                                  current_state->mono->p.constant));
    }
    assert(StackSize(&calc_stack) == 1);

    Poly result = *(Poly*)StackTop(&calc_stack);
    free(StackTop(&calc_stack));
    StackDestroy(&calc_stack, NULL);
    return result;
}

void PolyAddInPlace(Poly *p, Poly *q)
{
    assert(p != NULL && q != NULL);

    p->constant += q->constant;

    if (q->first_mono == NULL)
    {
        return;
    }

    if (p->first_mono == NULL)
    {
        p->first_mono = q->first_mono;

        return;
    }

    if (p->first_mono->exp > q->first_mono->exp)
    {
        Mono * const old_first_mono = p->first_mono;
        p->first_mono = q->first_mono;
        q->first_mono = q->first_mono->next_mono;
        p->first_mono->next_mono = old_first_mono;
    }

    Mono *p_mono = p->first_mono;
    Mono *q_mono = q->first_mono;
    Mono *prev_p_mono = p->first_mono;

    while (p_mono != NULL && q_mono != NULL)
    {
        if (p_mono->exp < q_mono->exp)
        {
            prev_p_mono = p_mono;
            p_mono = p_mono->next_mono;
        }
        else if (p_mono->exp > q_mono->exp)
        {
            Mono * const next_mono = q_mono->next_mono;
            prev_p_mono->next_mono = q_mono;
            q_mono->next_mono = p_mono;
            prev_p_mono = q_mono;
            q_mono = next_mono;
        }
        else {
            PolyAddInPlace(&p_mono->p, &q_mono->p);

            Mono * const next_mono = q_mono->next_mono;
            free(q_mono);
            q_mono = next_mono;
        }
    }

    if (q_mono != NULL)
    {
        prev_p_mono->next_mono = q_mono;
    }

    if(p->first_mono != NULL && p->first_mono->exp == 0){
        p->constant += p->first_mono->p.constant;
        p->first_mono->p.constant = 0;
    }

    RemoveEmptyMonosFromPoly(p);
}

void PolyDestroy(Poly *p)
{
    if (p == NULL)
    {
        return;
    }

    Mono *current_mono = p->first_mono;
    while (current_mono != NULL)
    {
        Mono * const next_mono = current_mono->next_mono;

        MonoDestroy(current_mono);
        free(current_mono);

        current_mono = next_mono;
    }

    p->first_mono = NULL;
    p->constant = 0;
}


Poly PolyClone(const Poly *p)
{
    Poly new_poly;
    new_poly.constant   = p->constant;
    new_poly.first_mono = MonoListClone(p->first_mono);

    return new_poly;
}

Poly PolyAdd(const Poly *p, const Poly *q)
{
    Poly result  = PolyClone(p);
    Poly q_clone = PolyClone(q);

    PolyAddInPlace(&result, &q_clone);

    return result;
}

Poly PolyAddMonos(unsigned count, const Mono monos[])
{

    if (count == 0)
    {
        return PolyZero();
    }

    if (MonosAreSorted(count, monos) == false)
    {
        // https://moodle.mimuw.edu.pl/mod/forum/discuss.php?d=244#p755
        // " Ja bym w takich przypadkach pozwalał na haki, np. pozbywanie
        //   się const za pomocą odpowiednich rzutowań lub innych trików  "
        //
        // Funkcja PolyAddMonos nie tworzy kopii tablicy monos ponieważ
        // wpływałoby to negatywnie na szybkość działania i ilość kodu
        qsort((Mono*)monos, count, sizeof(Mono), MonoCompare);
    }

    Poly result = PolyZero();

    Mono *last_mono = malloc(sizeof(Mono));
    assert(last_mono != NULL);
    *last_mono = monos[0];

    if (last_mono->exp == 0)
    {
        result.constant += last_mono->p.constant;
        last_mono->p.constant = 0;
    }

    result.first_mono = last_mono;

    poly_exp_t last_mono_exp = last_mono->exp;
    for (unsigned i = 1; i < count; ++i)
    {
        if (last_mono_exp == monos[i].exp)
        {
            PolyAddInPlace(&last_mono->p, (Poly*)&monos[i].p);
        }
        else {
            last_mono->next_mono = malloc(sizeof(Mono));
            assert(last_mono->next_mono != NULL);
            *last_mono->next_mono = monos[i];

            last_mono     = last_mono->next_mono;
            last_mono_exp = last_mono->exp;
        }

        if (last_mono->exp == 0)
        {
            result.constant += last_mono->p.constant;
            last_mono->p.constant = 0;
        }
    }

    RemoveEmptyMonosFromPoly(&result);

    return result;
}

Poly PolyMul(const Poly *p, const Poly *q)
{
    const unsigned p_mono_count = MonoCount(p);
    const unsigned q_mono_count = MonoCount(q);
    unsigned all_mono_count;
    all_mono_count = p_mono_count * q_mono_count + p_mono_count + q_mono_count;

    Mono *monos = calloc(all_mono_count, sizeof(Mono));
    assert(monos != NULL);

    unsigned p_id = 0;
    Mono *current_mono_p = p->first_mono;
    while (current_mono_p != NULL)
    {
        unsigned q_id = 0;
        Mono *current_mono_q = q->first_mono;
        while (current_mono_q != NULL)
        {
            const unsigned current_id = q_mono_count * p_id + q_id;
            monos[current_id].p   = PolyMul(&current_mono_p->p,
                                            &current_mono_q->p);
            monos[current_id].exp = current_mono_p->exp + current_mono_q->exp;

            ++q_id;
            current_mono_q = current_mono_q->next_mono;
        }

        ++p_id;
        current_mono_p = current_mono_p->next_mono;
    }

    Mono *first_p_mono = &monos[p_mono_count * q_mono_count];
    Mono *first_q_mono = &monos[p_mono_count * q_mono_count + p_mono_count];
    CloneMonosMultipliedByAConstant(p->first_mono, q->constant, first_p_mono);
    CloneMonosMultipliedByAConstant(q->first_mono, p->constant, first_q_mono);

    Poly result = PolyAddMonos(all_mono_count, monos);
    result.constant = p->constant * q->constant;

    free(monos);

    return result;
}

Poly PolyNeg(const Poly *p)
{
    Poly new_poly = PolyZero();
    new_poly.constant = -1 * p->constant;

    if (p->first_mono != NULL)
    {
        new_poly.first_mono = malloc(sizeof(Mono));
        assert(new_poly.first_mono != NULL);
        *new_poly.first_mono = MonoNeg(p->first_mono);

        Mono *current_mono   = new_poly.first_mono;
        Mono *current_p_mono = p->first_mono->next_mono;
        while (current_p_mono != NULL)
        {
            current_mono->next_mono = malloc(sizeof(Mono));
            assert(current_mono->next_mono != NULL);
            *(current_mono->next_mono) = MonoNeg(current_p_mono);

            current_mono = current_mono->next_mono;
            current_p_mono = current_p_mono->next_mono;
        }
    }

    return new_poly;
}

Poly PolySub(const Poly *p, const Poly *q)
{
    Poly result = PolyClone(p);
    Poly neg_q  = PolyNeg(q);
    PolyAddInPlace(&result, &neg_q);

    return result;
}

poly_exp_t PolyDegBy(const Poly *p, unsigned var_idx)
{
    if (PolyIsZero(p))
    {
        return -1;
    }

    poly_exp_t result = 0;
    if (var_idx == 0)
    {
        Mono *current_mono = p->first_mono;
        while (current_mono != NULL)
        {
            result = Max(result, current_mono->exp);

            current_mono = current_mono->next_mono;
        }
    }
    else {
        Mono *current_mono = p->first_mono;
        while (current_mono != NULL)
        {
            result = Max(result, PolyDegBy(&(current_mono->p), var_idx-1));

            current_mono = current_mono->next_mono;
        }
    }

    return result;
}


poly_exp_t PolyDeg(const Poly *p)
{
    poly_exp_t result = -1;
    if (p->constant != 0)
    {
        result = 0;
    }

    Mono *current_mono = p->first_mono;
    while (current_mono != NULL)
    {
        result = Max(result, PolyDeg(&current_mono->p) + current_mono->exp);

        current_mono = current_mono->next_mono;
    }

    return result;
}


bool PolyIsEq(const Poly *p, const Poly *q)
{
    if (p->constant != q->constant)
    {
        return false;
    }

    Mono *p_mono = p->first_mono;
    Mono *q_mono = q->first_mono;
    while (p_mono != NULL && q_mono != NULL)
    {
        if (p_mono->exp != q_mono->exp ||
           PolyIsEq(&p_mono->p, &q_mono->p) == false)
        {
            return false;
        }

        p_mono = p_mono->next_mono;
        q_mono = q_mono->next_mono;
    }

    if (p_mono != NULL || q_mono != NULL)
    {
        return false;
    }

    return true;
}

Poly PolyAt(const Poly *p, poly_coeff_t x)
{
    Poly result = PolyZero();

    Poly *temp_coeff_poly = malloc(sizeof(Poly));
    Poly *temp_mult_poly = malloc(sizeof(Poly));
    assert(temp_coeff_poly != NULL && temp_mult_poly != NULL);

    Mono *current_mono = p->first_mono;
    while (current_mono != NULL)
    {
        *temp_coeff_poly = PolyFromCoeff(FastCoeffPow(x, current_mono->exp));
        *temp_mult_poly  = PolyMul(temp_coeff_poly, &current_mono->p);

        PolyAddInPlace(&result, temp_mult_poly);

        PolyDestroy(temp_coeff_poly);

        current_mono = current_mono->next_mono;
    }

    free(temp_coeff_poly);
    free(temp_mult_poly);

    result.constant += p->constant;
    return result;
}
