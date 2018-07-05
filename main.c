#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}


/*************************************************/
/*Funzioni per riconoscere il pacchetto messaggio*/

/*
 * La struttura del pacchetto è del tipo:
 *
 * i numeri sono i valori decimali dell'equivalente ascii
 *
 * /---|----|---|-----|-----------|----|---\
 * | 1 | Id | 2 | Ora |   TEXT    |'\0'| 3 |
 * \---|----|---|-----|-----------|----|---/
 *
 * -1: in ascii indica "Start Of Heading"
 * -2: in ascii indica "Start Of Text"
 * -3: in ascii indica "End Of Text"
 *
 * (opzionale)
 * -4: -2: in ascii indica "End of Trasmission"
 */