#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "../cJSON-master/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../global_variable/global_variables.h"
#include "machine_learning/functionsML.h"

#define SAMPLE_COUNT 10
#define TIME_SAMPLE 10  // Campionamento ogni 10 secondi

static int count = 0;
extern AirSample air_sample; // Array di campioni della qualità dell'aria
static float new_co;
static float new_air_quality;
static float new_tvoc;

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_monitoring_air,
         "title=\"Air Quality Monitoring\";rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

// Handler per la richiesta GET
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {

    if (count >= SAMPLE_COUNT) {
            count = 0; // Resetta il buffer se pieno
        }

    new_co = air_sample.co;
    new_air_quality = air_sample.air_quality;
    new_tvoc = air_sample.tvoc;

    // Crea un JSON con gli ultimi dati raccolti
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();
    cJSON_AddItemToArray(data, cJSON_CreateNumber(air_sample.co));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(air_sample.air_quality));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(air_sample.tvoc));
    cJSON_AddItemToObject(root, "air_data", data);

    char *json = cJSON_Print(root);
    int length = snprintf((char *)buffer, preferred_size, "%s", json);
    coap_set_payload(response, (uint8_t *)buffer, length);

    count++;
}

// Notifica gli osservatori ogni 10 secondi
static void res_event_handler(void) {
    coap_notify_observers(&res_monitoring_air);
}

