#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "machine_learning/temperature_prediction.h"
#include "global_variable/global_variables.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_SAMPLES 10
#define TIME_PREDICT 10

extern Sample samples[10];
static float temp_history[10];
static float hum_history[10];
static float dust_history[10];
static float next_temperature; // Questo rimane il nostro "stato" in float
// static int temperature; // Rimuoviamo questo, non serve

extern int sample_count;

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_predict_temp,
         "title=\"Temperature Prediction\";rt=\"Text\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

static void predict_temperature(void){
  // --- INIZIO FIX LOGICO (No 20-sec warmup) ---
  if (sample_count == 0) {
      next_temperature = 0.0; // Valore di default se non ci sono campioni
      return;
  }
  
  if(sample_count < MAX_SAMPLES) {
    // Buffer non pieno: invia l'ultimo valore grezzo disponibile
    next_temperature = samples[sample_count - 1].temperature;
  } else {
    // Buffer pieno: esegui la predizione completa
    for(int i = 0; i < MAX_SAMPLES; i++){
      temp_history[i] = samples[i].temperature;
      hum_history[i]  = samples[i].humidity;
      dust_history[i] = samples[i].dust_density;
    }
    next_temperature = predict_next_temperature_from_values(
                           temp_history, hum_history, dust_history);
  }
  // --- FINE FIX LOGICO ---

  // Log con intero, che funziona
  printf("Predizione calcolata: %d (C*100)\n", (int)(next_temperature * 100));
}

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    
    // --- INIZIO FIX COMUNICATIVO (Invia x100) ---
    // Converti il float in un intero con 2 decimali di precisione
    int temp_to_send_x100 = (int)(next_temperature * 100);
    
    // Invia l'intero come stringa. Questo usa "%d" e funziona.
    int length = snprintf((char *)buffer, preferred_size, "%d", temp_to_send_x100);
    // --- FINE FIX COMUNICATIVO ---

    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_header_etag(response, (uint8_t *)&length, 1);
    coap_set_payload(response, (uint8_t *)buffer, length);

    printf("📤 Invio risposta Observe con valore %d (C*100)\n", temp_to_send_x100);
}


static void res_event_handler(void) {
    // 1. Calcola il nuovo valore (predizione o ultimo grezzo)
    predict_temperature();
    
    // 2. Logga che stiamo per notificare (usa l'intero)
    printf("📤 Invio notifica (valore %d C*100)\n", (int)(next_temperature * 100));
    
    // 3. Notifica gli osservatori
    coap_notify_observers(&res_predict_temp);
}

