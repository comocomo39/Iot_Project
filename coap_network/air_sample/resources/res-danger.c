#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include <stdio.h>
#include "../global_variable/global_variables.h"
#include "model/functionsML.h"

extern AirSample air_samples[10]; // Array di campioni della qualità dell'aria

static int prediction = 0;
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_danger,
         "title=\"Air Quality Danger Level\"; rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

// Handler per la richiesta GET per predire il livello di pericolo
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                             uint16_t preferred_size, int32_t *offset) {
    printf("🔴 Predizione del livello di pericolo dell'aria in corso...\n");

    // Prendiamo gli ultimi dati disponibili
    float features[3] = {
        air_samples[9].co,
        air_samples[9].air_quality,
        air_samples[9].tvoc
    };

    // Chiamiamo il modello ML per classificare il pericolo
    prediction = predict_class(features, 3);
    
    int val = prediction; // Il valore di classificazione è 0, 1 o 2
    if(val < 0) {
        printf("❌ Errore nel modello\n");
    }

    printf("🔎 Livello di Pericolo Predetto: %i\n", val);
    
    // Formatta il valore come stringa
    int length = snprintf((char *)buffer, preferred_size, "%d", val);

    if (length < 0) {
        coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
        return;
    }

    // Verifica che il buffer abbia abbastanza spazio
    if (length >= preferred_size) {
        coap_set_status_code(response, BAD_OPTION_4_02);
        return;
    }

    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, buffer, length);
}

// Notifica gli osservatori ogni volta che c'è una nuova predizione
static void res_event_handler(void) {
    printf("📡 Notifica agli attuatori del nuovo livello di pericolo\n");
    coap_notify_observers(&res_danger);
}
