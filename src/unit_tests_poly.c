/** @file
   Testy składania wielomianów

   @date 2017-06-05
*/
/*
 * Copyright 2008 Google Inc.
 * Copyright 2015 Tomasz Kociumaka
 * Copyright 2016, 2017 IPP team
 * Copyright 2017 b73f9
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include "cmocka.h"
#include "poly.h"

/// Makro zwracające długość tablicy 
#define array_length(x) (sizeof(x) / sizeof((x)[0]))

/// Miejsce gdzie program wraca po wywołaniu exit
static jmp_buf jmp_at_exit;

/// Kod wyjścia z którym wywołano exit
static int exit_status;

/// Oryginalna funkcja main kalkulatora
extern int calc_main(int argc, char *argv[]); 

/**
 * Atrapa funkcji main
*/
int mock_main(int argc, char *argv[]) {
    if (!setjmp(jmp_at_exit))
        return calc_main(argc, argv);
    return exit_status;
}

/**
 * Atrapa funkcji exit
 */
void mock_exit(int status) {
    exit_status = status;
    longjmp(jmp_at_exit, 1);
}

int mock_fprintf(FILE* const file, const char *format, ...) CMOCKA_PRINTF_ATTRIBUTE(2, 3);
int mock_printf(const char *format, ...) CMOCKA_PRINTF_ATTRIBUTE(1, 2);

/// Pomocniczy bufor do którego pisze atrapa fprintf
static char fprintf_buffer[256];
/// Pomocniczy bufor do którego pisze atrapa printf
static char printf_buffer[256];
/// Pozycja zapisu w buforze atrapy fprintf, wskazuje bajt o wartości 0.
static int fprintf_position = 0;
/// Pozycja zapisu w buforze atrapy printf, wskazuje bajt o wartości 0.
static int printf_position = 0;

/**
 * Atrapa funkcji fprintf sprawdzająca poprawność wypisywania na stderr.
 */
int mock_fprintf(FILE* const file, const char *format, ...) {
    int return_value;
    va_list args;

    assert_true(file == stderr);
    /* Poniższa asercja sprawdza też, czy fprintf_position jest nieujemne.
    W buforze musi zmieścić się kończący bajt o wartości 0. */
    assert_true((size_t)fprintf_position < sizeof(fprintf_buffer));

    va_start(args, format);
    return_value = vsnprintf(fprintf_buffer + fprintf_position,
                             sizeof(fprintf_buffer) - fprintf_position,
                             format,
                             args);
    va_end(args);

    fprintf_position += return_value;
    assert_true((size_t)fprintf_position < sizeof(fprintf_buffer));
    return return_value;
}


/**
 * Atrapa funkcji printf sprawdzająca poprawność wypisywania na stdout.
 */
int mock_printf(const char *format, ...) {
    int return_value;
    va_list args;

    /* Poniższa asercja sprawdza też, czy printf_position jest nieujemne.
    W buforze musi zmieścić się kończący bajt o wartości 0. */
    assert_true((size_t)printf_position < sizeof(printf_buffer));

    va_start(args, format);
    return_value = vsnprintf(printf_buffer + printf_position,
                             sizeof(printf_buffer) - printf_position,
                             format,
                             args);
    va_end(args);

    printf_position += return_value;
    assert_true((size_t)printf_position < sizeof(printf_buffer));
    return return_value;
}

/// Pomocniczy bufor, z którego korzystają atrapy funkcji operujących na stdin.
static char input_stream_buffer[256];
/// Pozycja w pomocniczym buforze atrap funkcji korzystających z stdin
static int input_stream_position = 0;
/// Koniec bufora dla atrap funkcji korzystających z stdin
static int input_stream_end = 0;
/// Ilość przeczytanych znaków
int read_char_count;

/**
 * Atrapa funkcji scanf używana do przechwycenia czytania z stdin.
 */
int mock_scanf(const char *format, ...) {
    va_list fmt_args;
    int ret;

    va_start(fmt_args, format);
    ret = vsscanf(input_stream_buffer + input_stream_position, format, fmt_args);
    va_end(fmt_args);

    if (ret < 0) { /* ret == EOF */
        input_stream_position = input_stream_end;
    }
    else {
        assert_true(read_char_count >= 0);
        input_stream_position += read_char_count;
        if (input_stream_position > input_stream_end) {
            input_stream_position = input_stream_end;
        }
    }
    return ret;
}

/**
 * Atrapa funkcji getchar używana do przechwycenia czytania z stdin.
 */
int mock_getchar() {
    if (input_stream_position < input_stream_end)
        return input_stream_buffer[input_stream_position++];
    else
        return EOF;
}

/**
 * Atrapa funkcji ungetc.
 * Obsługiwane jest tylko standardowe wejście.
 */
int mock_ungetc(int c, FILE *stream) {
    assert_true(stream == stdin);
    if (input_stream_position > 0)
        return input_stream_buffer[--input_stream_position] = c;
    else
        return EOF;
}

/**
 * Atrapa funkcji read.
 * Obsługiwane jest tylko standardowe wejście.
 */
int mock_read(int fd, void *buf, size_t count) {
    assert_true(fd == 0);
    unsigned i = 0;
    for (; i < count; ++i)
    {
        if (input_stream_position >= input_stream_end)
            break;
        ((char*)buf)[i] = input_stream_buffer[input_stream_position++];
    }
    return i;
}

/**
 * Funkcja wołana przed każdym testem.
 */
static int test_setup(void **state) {
    (void)state;

    memset(fprintf_buffer, 0, sizeof(fprintf_buffer));
    memset(printf_buffer, 0, sizeof(printf_buffer));
    printf_position = 0;
    fprintf_position = 0;

    /* Zwrócenie zera oznacza sukces. */
    return 0;
}

/**
 * Funkcja inicjująca dane wejściowe dla programu korzystającego ze stdin.
 */
static void init_input_stream(const char *str) {
    memset(input_stream_buffer, 0, sizeof(input_stream_buffer));
    input_stream_position = 0;
    input_stream_end = strlen(str);
    assert_true((size_t)input_stream_end < sizeof(input_stream_buffer));
    strcpy(input_stream_buffer, str);
}

/**
 * Test PolyCompose - p wielomian zerowy, count równe 0
 */
static void test_zero_poly_zero_count(void **state) {
    (void)state;

    Poly p = PolyZero();
    Poly result = PolyCompose(&p, 0, NULL);
    Poly expected_result = PolyZero();
    assert_true(PolyIsEq(&result, &expected_result));
    PolyDestroy(&p);
    PolyDestroy(&result);
    PolyDestroy(&expected_result);
}

/**
 * Test PolyCompose - p wielomian zerowy, count równe 1, x[0] wielomian stały
 */
static void test_zero_poly_one_count_constant(void **state) {
    (void)state;

    Poly p = PolyZero();
    Poly q = PolyFromCoeff(42);
    Poly result = PolyCompose(&p, 1, &q);
    Poly expected_result = PolyZero();
    assert_true(PolyIsEq(&result, &expected_result));
    PolyDestroy(&p);
    PolyDestroy(&q);
    PolyDestroy(&result);
    PolyDestroy(&expected_result);
}

/**
 * Test PolyCompose - p wielomian stały, count równe 0
 */
static void test_const_poly_zero_count(void **state) {
    (void)state;

    Poly p = PolyFromCoeff(43);
    Poly result = PolyCompose(&p, 0, NULL);
    Poly expected_result = PolyFromCoeff(43);
    assert_true(PolyIsEq(&result, &expected_result));
    PolyDestroy(&p);
    PolyDestroy(&result);
    PolyDestroy(&expected_result);
}

/**
 * Test PolyCompose - p wielomian stały, count równe 1, 
 * x[0] wielomian stały różny od p
 */
static void test_const_poly_one_count_constant(void **state) {
    (void)state;

    Poly p = PolyFromCoeff(44);
    Poly q = PolyFromCoeff(45);
    Poly result = PolyCompose(&p, 1, &q);
    Poly expected_result = PolyFromCoeff(44);
    assert_true(PolyIsEq(&result, &expected_result));
    PolyDestroy(&p);
    PolyDestroy(&q);
    PolyDestroy(&result);
    PolyDestroy(&expected_result);
}

/**
 * Test PolyCompose - p wielomian x0, count równe 0
 */
static void test_x0_poly_zero_count(void **state) {
    (void)state;

    Poly c = PolyFromCoeff(1);
    Mono m = MonoFromPoly(&c, 1);
    Poly p = PolyAddMonos(1, &m);
    Poly q = PolyZero();

    Poly result = PolyCompose(&p, 1, &q);
    Poly expected_result = PolyZero();
    assert_true(PolyIsEq(&result, &expected_result));
    PolyDestroy(&p);
    PolyDestroy(&q);
    PolyDestroy(&result);
    PolyDestroy(&expected_result);
}

/**
 * Test PolyCompose - p wielomian x0, count równe 1, x[0] wielomian stały
 */
static void test_x0_poly_one_count_const(void **state) {
    (void)state;

    Poly c = PolyFromCoeff(1);
    Mono m = MonoFromPoly(&c, 1);
    Poly p = PolyAddMonos(1, &m);
    Poly q = PolyFromCoeff(49);

    Poly result = PolyCompose(&p, 1, &q);
    Poly expected_result = PolyFromCoeff(49);
    assert_true(PolyIsEq(&result, &expected_result));
    PolyDestroy(&p);
    PolyDestroy(&q);
    PolyDestroy(&result);
    PolyDestroy(&expected_result);
}

/**
 * Test PolyCompose - p wielomian x0, count równe 1, x[0] wielomian x0
 */
static void test_x0_poly_one_count_x0(void **state) {
    (void)state;

    Poly c = PolyFromCoeff(1);
    Mono m = MonoFromPoly(&c, 1);
    Poly p = PolyAddMonos(1, &m);
    Poly q = PolyClone(&p);
    Poly expected_result = PolyClone(&p);

    Poly result = PolyCompose(&p, 1, &q);
    assert_true(PolyIsEq(&result, &expected_result));
    PolyDestroy(&p);
    PolyDestroy(&q);
    PolyDestroy(&result);
    PolyDestroy(&expected_result);
}

/**
 * Test czytania wejścia - COMPOSE - brak parametru
 */
static void test_compose_no_param(void **state) {
    (void)state;

    init_input_stream("COMPOSE\n");

    const char *args[] = {"calc_poly"};
    assert_int_equal(mock_main(array_length(args), (char **)args), 0);
    assert_string_equal(printf_buffer, "");
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * Test czytania wejścia - COMPOSE - zerowy parametr
 */
static void test_compose_zero_param(void **state) {
    (void)state;

    init_input_stream("0\nCOMPOSE 0\n");

    const char *args[] = {"calc_poly"};
    assert_int_equal(mock_main(array_length(args), (char **)args), 0);
    assert_string_equal(printf_buffer, "");
    assert_string_equal(fprintf_buffer, "");
}

/**
 * Test czytania wejścia - COMPOSE - maksymalny parametr 
 * (UINT_MAX na studentsie)
 */
static void test_compose_max_param(void **state) {
    (void)state;

    init_input_stream("COMPOSE 4294967295\n");

    const char *args[] = {"calc_poly"};
    assert_int_equal(mock_main(array_length(args), (char **)args), 0);
    assert_string_equal(printf_buffer, "");
    assert_string_equal(fprintf_buffer, "ERROR 1 STACK UNDERFLOW\n");
}

/**
 * Test czytania wejścia - COMPOSE - parametr równy -1
 */
static void test_compose_neg_one_param(void **state) {
    (void)state;

    init_input_stream("COMPOSE -1\n");

    const char *args[] = {"calc_poly"};
    assert_int_equal(mock_main(array_length(args), (char **)args), 0);
    assert_string_equal(printf_buffer, "");
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * Test czytania wejścia - COMPOSE - parametr o jeden wiekszy 
 * od maksymalnego (UINT_MAX+1 na studentsie)
 */
static void test_compose_one_over_max_param(void **state) {
    (void)state;

    init_input_stream("COMPOSE 4294967296\n");

    const char *args[] = {"calc_poly"};
    assert_int_equal(mock_main(array_length(args), (char **)args), 0);
    assert_string_equal(printf_buffer, "");
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * Test czytania wejścia - COMPOSE - parametr dużo większy od maksymalnego
 */
static void test_compose_lots_over_max_param(void **state) {
    (void)state;

    init_input_stream("COMPOSE 13333333333333337\n");

    const char *args[] = {"calc_poly"};
    assert_int_equal(mock_main(array_length(args), (char **)args), 0);
    assert_string_equal(printf_buffer, "");
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * Test czytania wejścia - COMPOSE - parametr złożony z liter
 */
static void test_compose_letters_param(void **state) {
    (void)state;

    init_input_stream("COMPOSE abcd\n");

    const char *args[] = {"calc_poly"};
    assert_int_equal(mock_main(array_length(args), (char **)args), 0);
    assert_string_equal(printf_buffer, "");
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * Test czytania wejścia - COMPOSE - parametr złożony z  
 * cyfr i liter zaczynający się cyfrą
 */
static void test_compose_letters_numbers_param(void **state) {
    (void)state;

    init_input_stream("COMPOSE 32b1cd9\n");

    const char *args[] = {"calc_poly"};
    assert_int_equal(mock_main(array_length(args), (char **)args), 0);
    assert_string_equal(printf_buffer, "");
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * Główna funkcja testów
 */
int main(void) {
    const struct CMUnitTest PolyComposeTests[] = {
        cmocka_unit_test(test_zero_poly_zero_count),
        cmocka_unit_test(test_zero_poly_one_count_constant),
        cmocka_unit_test(test_const_poly_zero_count),
        cmocka_unit_test(test_const_poly_one_count_constant),
        cmocka_unit_test(test_x0_poly_zero_count),
        cmocka_unit_test(test_x0_poly_one_count_const),
        cmocka_unit_test(test_x0_poly_one_count_x0),
    };
    const struct CMUnitTest COMPOSEParseTests[] = {
        cmocka_unit_test_setup(test_compose_no_param, test_setup),
        cmocka_unit_test_setup(test_compose_zero_param, test_setup),
        cmocka_unit_test_setup(test_compose_max_param, test_setup),
        cmocka_unit_test_setup(test_compose_neg_one_param, test_setup),
        cmocka_unit_test_setup(test_compose_one_over_max_param, test_setup),
        cmocka_unit_test_setup(test_compose_lots_over_max_param, test_setup),
        cmocka_unit_test_setup(test_compose_letters_param, test_setup),
        cmocka_unit_test_setup(test_compose_letters_numbers_param, test_setup),
    };
    bool result = cmocka_run_group_tests(PolyComposeTests, NULL, NULL);
    result |= cmocka_run_group_tests(COMPOSEParseTests, NULL, NULL);
    return result;
}
