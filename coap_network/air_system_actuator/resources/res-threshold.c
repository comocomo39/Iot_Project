#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include <stdio.h>
#include <string.h>
#include "../global_variable/global_variables.h"

/* Handler function for the GET and POST requests */
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_post_handler(coap_message_t *request, coap_message_t *response,
                             uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

extern int danger_threshold;

/* Resource definition */
RESOURCE(res_threshold,
         "title=\"Danger Threshold\";rt=\"threshold\"",
         res_get_handler,
         NULL,
         res_post_handler,
         NULL);

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    printf("Sending current danger threshold: %d\n", danger_threshold);
    int length = snprintf((char *)buffer, preferred_size, "{ \"danger_threshold\": %d }", danger_threshold);
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, length);
}

static void res_post_handler(coap_message_t *request, coap_message_t *response,
                             uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    const uint8_t *payload;
    int payload_len = coap_get_payload(request, &payload);
    if (payload_len > 0) {
        char temp_buffer[payload_len + 1];
        memcpy(temp_buffer, payload, payload_len);
        temp_buffer[payload_len] = '\0';
        
        int new_threshold = atoi(temp_buffer);
        if (new_threshold > 0) {
            danger_threshold = new_threshold;
            printf("Updated danger threshold to: %d\n", danger_threshold);
        } else {
            printf("Invalid threshold value received!\n");
        }
    }
}
