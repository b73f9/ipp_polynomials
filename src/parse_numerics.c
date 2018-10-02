/** @file
   Implementacja wczytywania liczb

   @date 2017-05-11
*/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "parse.h"
#include "utils.h"

/**
 * Wczytuje liczby do tablicy
 * @param[in,out] stream : wskaźnik na InputStream
 * @param[out] array : tablica
 * @param[in] max_length : długość tablicy
 */
static inline size_t ReadDigitsIntoAnArray(InputStream *stream,
                                           char *array,
                                           size_t max_length){
    size_t length = 0;
    while (IsValidDigit(PeekCharacter(stream)) && length < max_length)
    {
        array[length] = ReadCharacter(stream);
        ++length;
    }
    return length;
}


/**
 * Wczytuje liczbę zgodnie z określonymi wymaganiami
 *
 * ( value = true : argument AT, value = false : współczynnik )
 * Szczegółowe wymagania w ReadAtArgument / ReadPolyCoefficient
 * @param[in,out] stream : wskaźnik na InputStream
 * @param[in] isValue : czy argument AT
 */
static poly_coeff_t ReadValueOrCoefficient(InputStream *stream, bool isValue)
{
    char *value = calloc(MAX_VALUE_AND_COEFF_LENGTH, sizeof(char));
    assert(value != NULL);

    bool negative = false;
    if (PeekCharacter(stream) == '-')
    {
        negative = true;
        ReadCharacter(stream);
    }
    size_t length = ReadDigitsIntoAnArray(stream, value,
                                          MAX_VALUE_AND_COEFF_LENGTH);

    if (isValue)
    {
        if (PeekCharacter(stream) != '\n' || length == 0)
        {
            fprintf(stderr, "ERROR %u WRONG VALUE\n", stream->line_number + 1);
            SkipLine(stream);
            stream->parse_error = true;

            free(value);
            return 0;
        }
    }
    else {
        if ( (PeekCharacter(stream) != ',' && PeekCharacter(stream) != '\n') ||
             length == 0)
        {
            fprintf(stderr, "ERROR %u %u\n",
                    stream->line_number + 1, stream->column_number + 1);
            SkipLine(stream);
            stream->parse_error = true;

            free(value);
            return 0;
        }
    }
    if (length == MAX_VALUE_AND_COEFF_LENGTH)
    {
        poly_coeff_t reference;
        if (negative)
        {
            reference = LONG_MIN;
        }
        else {
            reference = LONG_MAX;
        }

        for (int i = length - 1; i >= 0; --i)
        {
            const poly_coeff_t c = value[i] - '0';
            poly_coeff_t ref_c = reference % ((poly_coeff_t)10);
            if (ref_c < 0)
            {
                ref_c *= (poly_coeff_t) -1;
            }
            reference /= (poly_coeff_t) 10;

            if (c > ref_c)
            {
                if (isValue)
                {
                    fprintf(stderr, "ERROR %u WRONG VALUE\n",
                            stream->line_number + 1);
                }
                else {
                    fprintf(stderr, "ERROR %u %u\n", stream->line_number + 1,
                            stream->column_number);
                }

                SkipLine(stream);
                stream->parse_error = true;

                free(value);
                return 0;
            }
        }
    }

    poly_coeff_t result = 0;
    for (unsigned i = 0; i < length; ++i)
    {
        result *= (poly_coeff_t) 10;
        result += (poly_coeff_t)(value[i] - '0');
    }
    if (negative)
    {
        result *= (poly_coeff_t)-1;
    }

    free(value);
    return result;
}

inline poly_coeff_t ReadAtCommandArgument(InputStream *stream)
{
    return ReadValueOrCoefficient(stream, true);
}

inline poly_coeff_t ReadPolyCoefficient(InputStream *stream)
{
    return ReadValueOrCoefficient(stream, false);
}

/**
 * Wczytuje liczbę zgodnie z określonymi wymaganiami
 *
 * ( isDegBy = true : argument DEG_BY, isDegBy = false : argument COMPOSE)
 * Szczegółowe wymagania w ReadDegByArgument / ReadComposeArgument
 * @param[in,out] stream : wskaźnik na InputStream
 * @param[in] isDegBy : czy argument DEG_BY
 */
unsigned ReadDegByOrComposeCommandArgument(InputStream *stream, bool isDegBy)
{
    char *value = calloc(MAX_VARIABLE_LENGTH, sizeof(char));
    assert(value != NULL);

    size_t length = ReadDigitsIntoAnArray(stream, value, MAX_VARIABLE_LENGTH);

    if (length == 0 || ReadCharacter(stream) != '\n')
    {
        if (isDegBy)
        {
            fprintf(stderr, "ERROR %u WRONG VARIABLE\n", stream->line_number + 1);
        }
        else {
            fprintf(stderr, "ERROR %u WRONG COUNT\n", stream->line_number + 1);
        }
        SkipLine(stream);
        stream->parse_error = true;

        free(value);
        return 0;
    }
    if (length == MAX_VARIABLE_LENGTH)
    {
        unsigned reference = UINT_MAX;
        for (int i = length - 1; i >= 0; --i)
        {
            const unsigned c = value[i] - '0';
            const unsigned ref_c = reference % 10;
            reference /= 10;
            if (c > ref_c)
            {
                if (isDegBy)
                {
                    fprintf(stderr, "ERROR %u WRONG VARIABLE\n",
                            stream->line_number);
                }
                else {
                    fprintf(stderr, "ERROR %u WRONG COUNT\n",
                            stream->line_number);
                }
                stream->parse_error = true;

                free(value);
                return 0;
            }
        }
    }

    unsigned result = 0;
    for (unsigned i = 0; i < length; ++i)
    {
        result *= 10;
        result += value[i] - '0';
    }

    free(value);
    return result;
}

unsigned ReadDegByCommandArgument(InputStream *stream){
    return ReadDegByOrComposeCommandArgument(stream, true);
}

unsigned ReadComposeCommandArgument(InputStream *stream){
    return ReadDegByOrComposeCommandArgument(stream, false);
}

poly_exp_t ReadExponent(InputStream *stream)
{
    char *value = calloc(MAX_EXPONENT_LENGTH, sizeof(char));
    assert(value != NULL);

    // https://moodle.mimuw.edu.pl/mod/forum/discuss.php?d=354#p1165
    bool negative_zero_expected = false;
    unsigned last_column = stream->column_number+2;
    if (PeekCharacter(stream) == '-')
    {
        negative_zero_expected = true;
        ReadCharacter(stream);
    }

    size_t length = ReadDigitsIntoAnArray(stream, value, MAX_EXPONENT_LENGTH);

    if (negative_zero_expected && (length != 1 || value[0] != '0')){
        fprintf(stderr, "ERROR %u %u\n", stream->line_number + 1, last_column);
        SkipLine(stream);
        stream->parse_error = true;

        free(value);
        return 0;
    }

    if (length == 0)
    {
        fprintf(stderr, "ERROR %u %u\n", stream->line_number + 1,
                stream->column_number + 1);
        SkipLine(stream);
        stream->parse_error = true;

        free(value);
        return 0;
    }
    if (length == MAX_EXPONENT_LENGTH)
    {
        unsigned reference = INT_MAX;
        for (int i = length - 1; i >= 0; --i)
        {
            const unsigned c = value[i] - '0';
            const unsigned ref_c = reference % 10;
            reference /= 10;

            if (c > ref_c)
            {
                fprintf(stderr, "ERROR %u %u\n", stream->line_number + 1,
                        stream->column_number);
                SkipLine(stream);
                stream->parse_error = true;

                free(value);
                return 0;
            }
        }
    }

    poly_exp_t result = 0;
    for (unsigned i = 0; i < length; ++i)
    {
        result *= 10;
        result += value[i] - '0';
    }

    free(value);
    return result;
}
