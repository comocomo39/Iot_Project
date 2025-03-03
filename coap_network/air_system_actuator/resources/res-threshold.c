#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "../../cJSON-master/cJSON.h"
#include "../global_variable/global_variables.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern int danger_threshold;  

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                             uint16_t preferred_size, int32_t *offset);

RESOURCE(res_tresh,
         "title=\"Set Danger Threshold\";rt=\"Text\"",
         res_get_handler,
         res_post_handler,
         NULL,
         NULL);

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    char json_str[50];
    snprintf(json_str, sizeof(json_str), "{ \"danger_threshold\": %d }", danger_threshold);
    int length = strlen(json_str);
    memcpy(buffer, json_str, length);
    coap_set_payload(response, buffer, length);
}

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                             uint16_t preferred_size, int32_t *offset) {
    const uint8_t *payload = NULL;
    int payload_len = coap_get_payload(request, &payload);
    if (payload_len) {
        danger_threshold = atoi((char *)payload);
        printf("🔴 Soglia di pericolo aggiornata a: %d\n", danger_threshold);
    }
}
