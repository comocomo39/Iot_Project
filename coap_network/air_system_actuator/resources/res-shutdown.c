#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include <stdio.h>
#include <string.h>
#include "../global_variable/global_variables.h"

/* Handler function for the GET request */
static void res_shutdown_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

extern int shutdown;

/* Resource definition */
RESOURCE(res_shutdown,
         "title=\"Shutdown Resource\";rt=\"shutdown\"",
         res_shutdown_get_handler,
         NULL,
         NULL,
         NULL);

static void res_shutdown_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    printf("🚨 Shutdown activated!\n");
    shutdown = 1;
    
    // Send JSON response
    char shutdown_msg[] = "{ \"shutdown\": \"activated\" }";
    int length = snprintf((char *)buffer, preferred_size, "%s", shutdown_msg);
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, (uint8_t *)buffer, length);
}
