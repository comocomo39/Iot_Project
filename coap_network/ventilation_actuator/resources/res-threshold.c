#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "../../cJSON-master/cJSON.h"
#include "../global_variable/global_variables.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                             uint16_t preferred_size, int32_t *offset);

// Variabili globali
extern float temp_tresh;  // Soglia della temperatura
extern int nRisktemp;
extern int high_temp_count;
extern int low_temp_count;

RESOURCE(res_tresh,
         "title=\"Set Temperature Threshold\";rt=\"Text\"",
         res_get_handler,
         res_post_handler,
         NULL,
         NULL);

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    printf("GET ricevuta - restituzione della soglia attuale\n");
    
    // Creazione di un JSON con la soglia attuale
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "temperature_threshold", temp_tresh);
    cJSON_AddNumberToObject(json, "risk_temperature", nRisktemp);
    cJSON_AddNumberToObject(json, "high_temp_count", high_temp_count);
    cJSON_AddNumberToObject(json, "low_temp_count", low_temp_count);

    // Conversione in stringa JSON
    char *json_str = cJSON_Print(json);

    // Copia nel buffer della risposta
    int length = strlen(json_str);
    if (length > preferred_size) {
        length = preferred_size;
    }
    memcpy(buffer, json_str, length);

    // Imposta i campi della risposta CoAP
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_header_etag(response, (uint8_t *)&length, 1);
    coap_set_payload(response, buffer, length);
    
    cJSON_Delete(json);
}

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                             uint16_t preferred_size, int32_t *offset) {
    printf("POST ricevuta - aggiornamento soglia\n");
    nRisktemp = 0;
    high_temp_count = 0;
    low_temp_count = 0;
    
    const uint8_t *payload = NULL;
    int payload_len = coap_get_payload(request, &payload);

    if (payload_len) {
        char temp_str[16];
        if (payload_len >= sizeof(temp_str)) {
            coap_set_status_code(response, BAD_REQUEST_4_00);
            return;
        }

        // Copia il valore della nuova soglia
        memcpy(temp_str, payload, payload_len);
        temp_str[payload_len] = '\0';

        // Converte il valore e aggiorna la soglia
        temp_tresh = atof(temp_str);
        printf("Nuova soglia di temperatura impostata: %.2f°C\n", temp_tresh);

        // Costruisce la risposta
        int length = snprintf((char *)buffer, preferred_size, "Threshold set to: %.2f", temp_tresh);
        coap_set_header_content_format(response, TEXT_PLAIN);
        coap_set_header_etag(response, (uint8_t *)&length, 1);
        coap_set_payload(response, buffer, length);
    }
}
