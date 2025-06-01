#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include <stdio.h>
#include <string.h>
#include "global_variable/global_variables.h"

extern Sample samples[10];

// 👇 DICHIARAZIONE DELLA FUNZIONE PRIMA DELLA MACRO
static void res_shutdown_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* Resource definition */
RESOURCE(res_shutdown,
         "title=\"Shutdown Resource\";rt=\"shutdown\"",
         res_shutdown_get_handler, // ora è dichiarata
         NULL,
         NULL,
         NULL);

/* DEFINIZIONE DELLA FUNZIONE */
static void res_shutdown_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{ 
    printf("Shutdown richiesto.\n");
    samples[0].humidity = -1; // Imposta shutdown
}
