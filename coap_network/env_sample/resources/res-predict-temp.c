#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "machine_learning/temperature_prediction.h"
#include "global_variables.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TIME_PREDICT 10  // Predizione ogni 10 secondi

extern Sample samples[10];
static float temp_history[10];
static float hum_history[10];
static float dust_history[10];
static float next_temperature;

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_predict_temp,
         "title=\"Temperature Prediction\";rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

// Funzione per raccogliere i dati e fare la predizione
static void predict_temperature() {
    for (int i = 0; i < 10; i++) {
        temp_history[i] = samples[i].temperature;
        hum_history[i] = samples[i].humidity;
        dust_history[i] = samples[i].dust_density;
    }

    // Passiamo i tre array corretti
    next_temperature = predict_next_temperature_from_values(temp_history, hum_history, dust_history);

    printf("🔵 Predizione Temperatura: %.2f°C\n", next_temperature);
}


// Handler per richiesta GET
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    predict_temperature();

    int length = snprintf((char *)buffer, preferred_size, "Predicted Temp: %.2f", 
                      predict_next_temperature_from_values(temp_history, hum_history, dust_history));
    coap_set_payload(response, (uint8_t *)buffer, length);
}

// Notifica gli osservatori ogni 10 secondi
static void res_event_handler(void) {
    coap_notify_observers(&res_predict_temp);
}
