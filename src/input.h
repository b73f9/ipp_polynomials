/** @file
   Biblioteka do obsługi wejścia

   @date 2017-05-19
*/

#ifndef __INPUTSTREAM_H__
#define __INPUTSTREAM_H__

#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include "utils.h"

#define INPUT_BUFFER_SIZE 1024
///< Rozmiar bufora w którym trzymamy wejście

/**
 * Struktura przechowująca informacje o wejściu
 */
typedef struct InputStream
{
    char *buffer; ///< Tablica przechowywująca wczytane dane
    char *current_character_ptr; ///< Wskaźnik na ostatni zwrócony znak
    int remaining_buffer_size; ///< Pozostała liczba wczytanych znaków
    int file_descriptor; ///< Identyfikator pliku do wcztytywania danych
    unsigned column_number; ///< Numer ostatniej zwróconej kolumny
    unsigned line_number; ///< Numer ostatniego zwróconego wiersza
    bool parse_error; ///< Oznacza błędne dane (ustawiana zewnętrznie)
} InputStream;

/**
 * Tworzy nowy InputStream
 * @param[in] file_descriptor : identyfikator pliku z którego czytamy
 */
static inline InputStream InputStreamInit(int file_descriptor)
{
    InputStream stream;
    stream.line_number = 0;
    stream.column_number = 0;
    stream.parse_error = false;
    stream.file_descriptor = file_descriptor;
    stream.buffer = calloc(INPUT_BUFFER_SIZE, sizeof(char));
    stream.current_character_ptr = stream.buffer;
    stream.remaining_buffer_size = 0;
    return stream;
}

/**
 * Zwraca znak z wejścia, nie przechodząc do następnego
 * @param[in,out] stream : wskaźnik na InputStream
 */
static inline char PeekCharacter(InputStream *stream)
{
    assert(stream != NULL);
    stream->parse_error = false;

    if (stream->remaining_buffer_size == 0)
    {
        stream->remaining_buffer_size = read(stream->file_descriptor,
                                             stream->buffer,
                                             INPUT_BUFFER_SIZE);
        stream->current_character_ptr = stream->buffer;
    }

    if (stream->remaining_buffer_size > 0)
    {
        return *stream->current_character_ptr;
    }
    else {
        return EOF;
    }
}

/**
 * Zwraca znak z wejścia, przechodząc do następnego
 * @param[in,out] stream : wskaźnik na InputStream
 */
static inline char ReadCharacter(InputStream *stream)
{
    assert(stream != NULL);
    const char c = PeekCharacter(stream);
    if (c != EOF)
    {
        --stream->remaining_buffer_size;
        ++stream->current_character_ptr;

        ++stream->column_number;
        if (c == '\n')
        {
            ++stream->line_number;
            stream->column_number = 0;
        }
    }

    return c;
}

/**
 * Pomija obecną linię wejścia
 * @param[in,out] stream : wskaźnik na InputStream
 */
static inline void SkipLine(InputStream *stream)
{
    assert(stream != NULL);
    char c = ReadCharacter(stream);
    while (c != '\n' && c != EOF)
    {
        c = ReadCharacter(stream);
    }
}

/**
 * Usuwa InputStream z pamięci
 * @param[in,out] stream : wskaźnik na InputStream
 */
static inline void InputStreamDestroy(InputStream *stream)
{
    assert(stream != NULL);
    free(stream->buffer);
}

#endif /* __INPUTSTREAM_H__ */
