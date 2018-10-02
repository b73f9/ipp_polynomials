/** @file
   Interfejs biblioteki wczytujacej wejście

   @date 2017-05-19
*/

#ifndef __PARSE_H__
#define __PARSE_H__

#include "input.h"
#include "stack.h"
#include "poly.h"
#include "utils.h"

#define COMMAND_ZERO "ZERO"
///< Nazwa polecenia dodającego zerowy wielomian
#define COMMAND_IS_COEFF "IS_COEFF"
///< Nazwa polecenia sprawdzającego równoważność
#define COMMAND_IS_ZERO "IS_ZERO"
///< Nazwa polecenia sprawdzającego czy wielomian to zero
#define COMMAND_CLONE "CLONE"
///< Nazwa polecenia kopiującego wielomiann
#define COMMAND_ADD "ADD"
///< Nazwa polecenia dodającego dwa wielomiany
#define COMMAND_MUL "MUL"
///< Nazwa polecenia mnożącego dwa wielomiany
#define COMMAND_NEG "NEG"
///< Nazwa polecenia negującego wielomian
#define COMMAND_SUB "SUB"
///< Nazwa polecenia odejmującego dwa wielomiany
#define COMMAND_IS_EQ "IS_EQ"
///< Nazwa polecenia sprawdzającego równość dwóch wielomianów
#define COMMAND_DEG "DEG"
///< Nazwa polecenia sprawdzającego stopień wielomianu
#define COMMAND_DEG_BY "DEG_BY"
///< Nazwa polecenia sprawdzającego stopień wielomianu wg. zmiennej
#define COMMAND_AT "AT"
///< Nazwa polecenia liczącego wielomian dla danej wartości
#define COMMAND_PRINT "PRINT"
///< Nazwa polecenia wypisującego wielomian
#define COMMAND_POP "POP"
///< Nazwa polecenia zdejmującego wielomian ze stosu
#define COMMAND_COMPOSE "COMPOSE"
///< Nazwa polecenia składającego wielomiany

#define MAX_COMMAND_LENGTH 10
///< Maksymalna długość poprawnego polecenia
#define MAX_VALUE_AND_COEFF_LENGTH 19
///< Maksymalna długość argumentu AT lub współczynnika
#define MAX_EXPONENT_LENGTH 10
///< Maksymalna dlugość wykładnika
#define MAX_VARIABLE_LENGTH 10
///< Maksymalna długość argumentu DEG_BY

/**
 * Sprawdza czy znak jest cyfrą
 * @param[in] c : znak
 */
static inline bool IsValidDigit(const char c)
{
    return (c >= '0' && c <= '9');
}

/**
 * Sprawdza czy znak może być częścią liczby
 * @param[in] c : znak
 */
static inline bool IsValidNumberCharacter(const char c)
{
    return IsValidDigit(c) || c == '-';
}

/**
 * Sprawdza czy znak może być początkiem polecenia
 * @param[in] c : znak
 */
static inline bool IsValidCommandStartingCharacter(const char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/**
 * Sprawdza czy znak może być częścią polecenia
 * @param[in] c : znak
 */
static inline bool IsValidCommandCharacter(const char c)
{
    return IsValidCommandStartingCharacter(c) || c == '_';
}

/**
 * Wczytuje liczbę @p x będącą argumentem polecenia AT
 *
 * Wartość parametru polecenia AT uznajemy za niepoprawną,
 * jeśli jest ona mniejsza od LONG_MIN lub większa od LONG_MAX.
 * Ustawia stream->parse_error na true w przypadku błędu.
 * @param[in,out] stream : wskaźnik na wykorzystywany InputStream
 * @return x
 */
poly_coeff_t ReadAtCommandArgument(InputStream *stream);

/**
 * Wczytuje liczbę @p x będącą współczynnikiem wielomianu
 *
 * Wartość współczynnika uznajemy za niepoprawną,
 * jeśli jest ona mniejsza od LONG_MIN lub większa od LONG_MAX.
 * Ustawia stream->parse_error na true w przypadku błędu.
 * @param[in,out] stream : wskaźnik na wykorzystywany InputStream
 * @return x
 */
poly_coeff_t ReadPolyCoefficient(InputStream *stream);

/**
 * Wczytuje liczbę @p x będącą wykładnikiem jednomianu
 *
 * Wartość wykładnika uznajemy za niepoprawną,
 * jeśli jest ona mniejsza od 0 lub większa od INT_MAX.
 * Ustawia stream->parse_error na true w przypadku błędu.
 * @param[in,out] stream : wskaźnik na wykorzystywany InputStream
 * @return x
 */
poly_exp_t ReadExponent(InputStream *stream);

/**
 * Wczytuje liczbę @p x będącą argumentem polecenia DEG_BY
 *
 * Wartość parametru polecenia DEG_BY uznajemy za niepoprawną,
 * jeśli jest ona mniejsza od 0 lub większa od UINT_MAX.
 * Ustawia stream->parse_error na true w przypadku błędu.
 * @param[in,out] stream : wskaźnik na wykorzystywany InputStream
 * @return x
 */
unsigned ReadDegByCommandArgument(InputStream *stream);

/**
 * Wczytuje liczbę @p x będącą argumentem polecenia COMPOSE
 *
 * Wartość parametru polecenia COMPOSE uznajemy za niepoprawną,
 * jeśli jest ona mniejsza od 0 lub większa od UINT_MAX.
 * Ustawia stream->parse_error na true w przypadku błędu.
 * @param[in,out] stream : wskaźnik na wykorzystywany InputStream
 * @return x
 */
unsigned ReadComposeCommandArgument(InputStream *stream);


/**
 * Wczytuje wielomian @p p
 *
 * Ustawia stream->parse_error na true w przypadku błędu.
 * @param[in,out] stream : wskaźnik na wykorzystywany InputStream
 * @return p
 */
Poly ReadPolynomial(InputStream *stream);

/**
 * Wczytuje i wykonuje polecenie
 *
 * Ustawia stream->parse_error na true w przypadku błędu.
 * @param[in,out] stream : wskaźnik na wykorzystywany InputStream
 * @param[in,out] poly_stack : stos wielomianów
 */
void ReadAndExecuteCommand(InputStream *stream, Stack *poly_stack);

#endif /* __PARSE_H__ */
