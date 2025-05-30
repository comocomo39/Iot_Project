#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "../cJSON-master/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "machine_learning/temperature_prediction.h"
#include "../global_variable/global_variables.h"

#define SAMPLE_COUNT 10
#define TIME_SAMPLE 10

extern Sample samples[SAMPLE_COUNT];

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_monitoring_temp,
         "title=\"Environmental Monitoring\";rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {

    int last_sample = (sample_count == 0) ? 0 : sample_count - 1;

    // Crea JSON
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();
    cJSON_AddItemToArray(data, cJSON_CreateNumber(samples[last_sample].temperature));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(samples[last_sample].humidity));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(samples[last_sample].dust_density));
    cJSON_AddItemToObject(root, "ss", data);
    cJSON_AddNumberToObject(root, "t", TIME_SAMPLE);

    char *json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
        cJSON_Delete(root);
        return;
    }

    int length = snprintf((char *)buffer, preferred_size, "%s", json);
    coap_set_payload(response, (uint8_t *)buffer, length);

    cJSON_Delete(root);
    free(json);
}

static void res_event_handler(void) {
    coap_notify_observers(&res_monitoring_temp);
}
