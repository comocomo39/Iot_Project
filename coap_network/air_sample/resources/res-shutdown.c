#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include <stdio.h>
#include <string.h>
#include "global_variables.h"

/* Handler function for the GET request */
static void res_shutdown_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
extern AirSample air_sample;

/* Resource definition */
RESOURCE(res_shutdown,
         "title=\"Shutdown Resource\";rt=\"shutdown\"",
         res_shutdown_get_handler,
         NULL,
         NULL,
         NULL);
static void res_shutdown_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset)

{ 
    air_sample.co=-1;
    printf("Shutdown incremented RES SHUTDOWN AIRSENSOR \n");

}