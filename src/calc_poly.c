/** @file
   Implementacja kalkulatora

   @date 2017-05-25
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "parse.h"
#include "utils.h"

/**
 * Główna funkcja kalkulatora
 */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    Stack   poly_stack = StackInit();
    InputStream stream = InputStreamInit(STDIN_FILENO);

    while (EOF != PeekCharacter(&stream))
    {
        if (IsValidCommandCharacter(PeekCharacter(&stream)))
        {
            ReadAndExecuteCommand(&stream, &poly_stack);
        }
        else {
            Poly *p = malloc(sizeof(Poly));
            assert(p != NULL);
            *p = ReadPolynomial(&stream);

            if (stream.parse_error)
            {
                PolyDestroy(p);
                free(p);
                continue;
            }

            StackPush(&poly_stack, p);
        }
    }

    InputStreamDestroy(&stream);
    StackDestroy(&poly_stack, &PolyDestroy);

    return 0;
}
