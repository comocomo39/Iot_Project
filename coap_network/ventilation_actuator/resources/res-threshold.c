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

// --- MODIFICA: Variabili globali intere (x100) ---
// Deve corrispondere a ventilation.c
extern int temp_tresh_x100;  // (31.50 * 100)
extern int nRisktemp;
extern int high_temp_count;
extern int low_temp_count;
// --- FINE MODIFICA ---

RESOURCE(res_tresh,
         "title=\"Set Temperature Threshold\";rt=\"Text\"",
         res_get_handler,
         res_post_handler,
         NULL,
         NULL);

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    printf("GET ricevuta - restituzione della soglia attuale\n");
    
    cJSON *json = cJSON_CreateObject();
    // --- MODIFICA: Invia soglia intera ---
    cJSON_AddNumberToObject(json, "temperature_threshold_x100", temp_tresh_x100);
    cJSON_AddNumberToObject(json, "risk_temperature", nRisktemp);
    cJSON_AddNumberToObject(json, "high_temp_count", high_temp_count);
    cJSON_AddNumberToObject(json, "low_temp_count", low_temp_count);
    // --- FINE MODIFICA ---

    char *json_str = cJSON_Print(json);
    
    if (json_str == NULL) {
        coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
        cJSON_Delete(json);
        return;
    }

    int length = strlen(json_str);
    if (length > preferred_size) {
        length = preferred_size;
    }
    memcpy(buffer, json_str, length);

    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_header_etag(response, (uint8_t *)&length, 1);
    coap_set_payload(response, buffer, length);
    
    cJSON_Delete(json);
    free(json_str); // Correzione memory leak
}

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                             uint16_t preferred_size, int32_t *offset) {
    printf("POST ricevuta - aggiornamento soglia\n");
    nRisktemp = 0;
    high_temp_count = 0;
    low_temp_count = 0;
    
    const uint8_t *payload = NULL;
    int payload_len = coap_get_payload(request, &payload);

    if (payload_len > 0) {
        char temp_str[16];
        if (payload_len >= (int)sizeof(temp_str)) {
            coap_set_status_code(response, BAD_REQUEST_4_00);
            return;
        }

        memcpy(temp_str, payload, payload_len);
        temp_str[payload_len] = '\0';

        // --- MODIFICA: Parsing (atoi) ---
        // Il payload POST ora deve essere un intero (es. "3200" per 32.0°C)
        temp_tresh_x100 = atoi(temp_str);
        printf("Nuova soglia di temperatura impostata: %d (C*100)\n", temp_tresh_x100);
        
        int length = snprintf((char *)buffer, preferred_size, "Threshold set to: %d", temp_tresh_x100);
        // --- FINE MODIFICA ---
        
        coap_set_header_content_format(response, TEXT_PLAIN);
        coap_set_header_etag(response, (uint8_t *)&length, 1);
        coap_set_payload(response, buffer, length);
    } else {
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}