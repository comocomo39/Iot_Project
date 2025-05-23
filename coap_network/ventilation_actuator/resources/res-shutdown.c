#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include <stdio.h>
#include <string.h>
#include "../global_variable/global_variables.h"

extern float temp_tresh;
extern int high_temp_count;
extern int low_temp_count;
extern int shutdown;

/* Handler per la richiesta GET */
static void res_shutdown_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* Definizione della risorsa */
RESOURCE(res_shutdown,
         "title=\"Shutdown Resource\";rt=\"shutdown\"",
         res_shutdown_get_handler,
         NULL,
         NULL,
         NULL);

static void res_shutdown_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    printf("Spegnimento del sistema di ventilazione attivato!\n");
    temp_tresh = -1;  // Segnale di arresto per il sistema
    high_temp_count = 0;
    low_temp_count = 0;
    shutdown = 1;
    // Formattazione della risposta
    char shutdown_msg[] = "{ \"shutdown\": \"activated\" }";
    int length = snprintf((char *)buffer, preferred_size, "%s", shutdown_msg);
    
    coap_set_payload(response, (uint8_t *)buffer, length);
}