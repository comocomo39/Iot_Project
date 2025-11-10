#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/etimer.h"
#include "os/dev/leds.h"
#include "../cJSON-master/cJSON.h"
#include "global_variable/global_variables.h"
#include "os/dev/button-hal.h"

/*---------------------------------------------------------------------------*/
/* Costanti e variabili globali */
#define SERVER_EP "coap://[fd00::1]:5683"
#define MAX_REGISTRATION_ATTEMPTS 5
#define REGISTRATION_DELAY 10  // secondi
#define GOOD_ACK 65            // codice ACK CoAP

static char *service_url_reg   = "/registrationActuator";
static char *service_url_temp  = "predict-temp";
static char  sensor_addr_str[64] = "";  // IPv6 del sensore env_sample

// --- INIZIO MODIFICA: Converti variabili in interi (x100) ---
int   temp_tresh_x100 = 3150; // (31.5 * 100) -> Questa è la variabile esterna modificata dalla risorsa
int   nRisktemp       = 0;
int   shutdown        = 0;
int   high_temp_count = 0;
int   low_temp_count  = 0;

static int predicted_temperature_x100 = 0; // (Valore predetto * 100)
// Rimuoviamo la variabile 'temp_threshold' statica, useremo quella globale 'temp_tresh_x100'
// static float temp_threshold = 31.5; 
// --- FINE MODIFICA ---

static int registered = 0;
static int registration_attempts = 0;

/* Risorse locali (per threshold e shutdown) */
extern coap_resource_t res_tresh;
extern coap_resource_t res_shutdown;

/*---------------------------------------------------------------------------*/
/* Prototipi */
void registration_handler(coap_message_t *response);
void handle_notification_temp(struct coap_observee_s *observee,
                              void *notification,
                              coap_notification_flag_t flag);
void response_handler_temp(coap_message_t *response);

/*---------------------------------------------------------------------------*/
PROCESS(coap_client_process, "CoAP Client Process");
AUTOSTART_PROCESSES(&coap_client_process);

static coap_observee_t *obs_temp = NULL;
/*---------------------------------------------------------------------------*/
/* Funzione di controllo della ventilazione ---------------------------------*/
// --- INIZIO MODIFICA: Firma e logica a interi ---
void
control_ventilation(int predicted_temp, int threshold)
{
  if(predicted_temp < threshold) {
    low_temp_count++;
    high_temp_count = 0;

    if(low_temp_count >= 2) {
      // --- MODIFICA: Stampa interi (formattati) ---
      printf("🌡️ Temperatura stabile (%d.%02d°C). Ventilazione SPENTA.\n", 
             predicted_temp / 100, predicted_temp % 100);
      // --- FINE MODIFICA ---

    leds_off(LEDS_RED);
    leds_single_off(LEDS_YELLOW);
    leds_single_on(LEDS_RED);
    }

  } else if(predicted_temp < threshold + 200) { // (threshold + 2.0 * 100)
    high_temp_count++;
    low_temp_count = 0;

    // --- MODIFICA: Stampa interi (formattati) ---
    printf("🌡️ Temperatura alta (%d.%02d°C). Ventilazione MEDIA attivata.\n", 
           predicted_temp / 100, predicted_temp % 100);
    // --- FINE MODIFICA ---

    leds_single_off(LEDS_RED);
    leds_off(LEDS_RED);
    leds_single_on(LEDS_YELLOW);

  } else {
    high_temp_count++;
    low_temp_count = 0;

    if(high_temp_count >= 3) {
      // --- MODIFICA: Stampa interi (formattati) ---
      printf("🔥 Temperatura molto alta (%d.%02d°C). Ventilazione MASSIMA attivata!\n", 
             predicted_temp / 100, predicted_temp % 100);
      // --- FINE MODIFICA ---
      leds_single_off(LEDS_RED);
      leds_single_off(LEDS_YELLOW);
      leds_on(LEDS_RED);
    }
  }
}
// --- FINE MODIFICA ---
/*---------------------------------------------------------------------------*/
/* Gestione della risposta puntuale (non-observe) ---------------------------*/
void
response_handler_temp(coap_message_t *response)
{
  if(response == NULL) {
    printf("Nessuna risposta ricevuta dal server.\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if(len <= 0) return;

  char temp_str[len + 1];
  memcpy(temp_str, chunk, len);
  temp_str[len] = '\0';

  // --- INIZIO MODIFICA: Parsing (atoi) e Stampa (interi) ---
  // Il payload è un intero (es. "3050"), usiamo atoi()
  predicted_temperature_x100 = atoi(temp_str);
  
  // Stampa formattata (es. 3050 -> "30.50")
  printf("📈 Temperatura Predetta: %d.%02d°C\n", 
         predicted_temperature_x100 / 100, predicted_temperature_x100 % 100);

  // Usiamo la variabile globale 'temp_tresh_x100' (impostata dalla risorsa)
  control_ventilation(predicted_temperature_x100, temp_tresh_x100);
  // --- FINE MODIFICA ---
}
/*---------------------------------------------------------------------------*/
/* Handler della risposta di registrazione ----------------------------------*/
void
registration_handler(coap_message_t *response)
{
// ... (codice invariato) ...
  if(response == NULL) {
    printf("⛔ Nessuna risposta dal server di registrazione.\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if(len <= 0) {
    printf("⚠️ Payload di registrazione vuoto.\n");
    return;
  }

  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';
  printf("📡 Risposta dal server: %s\n", payload);

  /* Parso il JSON di risposta */
  cJSON *json = cJSON_Parse(payload);
  if(json == NULL) {
    printf("⛔ Errore nel parsing del JSON.\n");
    return;
  }

  /* Estraggo il campo "e" con l’IPv6 del sensore */
  cJSON *ipv6_item = cJSON_GetObjectItemCaseSensitive(json, "e");
  if(cJSON_IsString(ipv6_item) && ipv6_item->valuestring && ipv6_item->valuestring[0] != '\0') {
    snprintf(sensor_addr_str, sizeof(sensor_addr_str), "%s", ipv6_item->valuestring);
    printf("✅ IPv6 ricevuto dal server: %s\n", sensor_addr_str);
    registered = 1;
  } else {
    printf("⚠️ Nessun campo \"e\" valido nel JSON di registrazione!\n");
  }

  cJSON_Delete(json);

  if(response->code == GOOD_ACK)
    printf("✅ ACK ricevuto dal server (code %u).\n", response->code);
  else
    printf("⚠️ Registrazione non confermata (code %u).\n", response->code);
}
/*---------------------------------------------------------------------------*/
/* Callback delle notifiche Observe -----------------------------------------*/
void
handle_notification_temp(struct coap_observee_s *observee,
                         void *notification,
                         coap_notification_flag_t flag)
{
  // --- INIZIO MODIFICA: Aggiungi gestione flag=4 ---
  // Questo gestisce il caso in cui il sensore annulla la sottoscrizione
  if(flag == 4) {
      printf("🚫 Sottoscrizione cancellata (flag=4). Riprovo.\n");
      process_poll(&coap_client_process); // Dice al processo principale di riprovare
      return;
  }
  // --- FINE MODIFICA ---

  if(notification) {
    printf("🔔 Notifica ricevuta dal sensore env_sample (/predict-temp)\n");
    response_handler_temp((coap_message_t *)notification);
  } else {
    printf("⚠️ Nessuna notifica ricevuta (flag=%d)\n", flag);
  }
}
/*---------------------------------------------------------------------------*/
/* Processo principale ------------------------------------------------------*/
PROCESS_THREAD(coap_client_process, ev, data)
{
  static coap_endpoint_t server_ep;
  static coap_endpoint_t server_ep_temp;
  static coap_message_t  request[1];
  static struct etimer   registration_timer;

  PROCESS_BEGIN();

  leds_single_on(LEDS_RED);  // LED rosso acceso: avvio
  PROCESS_WAIT_EVENT_UNTIL(ev == button_hal_press_event);
  leds_single_off(LEDS_RED);

  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

  /* Tentativi di registrazione */
  while(registration_attempts < MAX_REGISTRATION_ATTEMPTS && !registered) {
// ... (codice invariato) ...
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, service_url_reg);
    coap_set_payload(request, (uint8_t *)"ventilation", strlen("ventilation"));

    printf("📡 Invio richiesta di registrazione...\n");
    COAP_BLOCKING_REQUEST(&server_ep, request, registration_handler);

    if(!registered) {
      registration_attempts++;
      printf("⏳ Ritento tra %d secondi (tentativo %d/%d)...\n",
             REGISTRATION_DELAY, registration_attempts, MAX_REGISTRATION_ATTEMPTS);
      etimer_set(&registration_timer, CLOCK_SECOND * REGISTRATION_DELAY);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));
    }
  }

  /* Dopo la registrazione */
  if(registered) {
// ... (codice invariato) ...
    printf("✅ Attuatore di ventilazione registrato correttamente.\n");

    /* Controllo che l’indirizzo IPv6 sia valido */
    if(strlen(sensor_addr_str) < 5) {
      printf("⛔ IPv6 non valido, annullo osservazione.\n");
      PROCESS_EXIT();
    }

    /* Costruisco l’endpoint completo */
    char addr_temp[100] = "coap://[";
    strcat(addr_temp, sensor_addr_str);
    strcat(addr_temp, "]:5683");

    coap_endpoint_parse(addr_temp, strlen(addr_temp), &server_ep_temp);
    printf("✅ Endpoint sensore predict-temp: %s\n", addr_temp);

    /* Registro l’osservazione della risorsa */
    obs_temp = coap_obs_request_registration(&server_ep_temp,
                                             service_url_temp,
                                             handle_notification_temp,
                                             NULL);

    /* Attivo risorse locali */
    coap_activate_resource(&res_tresh,   "threshold");
    coap_activate_resource(&res_shutdown,"shutdown");

    /* Ciclo principale */
    while(1) {
      PROCESS_WAIT_EVENT();
      
      // --- INIZIO MODIFICA: Aggiungi ri-sottoscrizione e controllo intero ---
      // Se riceviamo un evento 'poll' (inviato da handle_notification_temp)
      if (ev == PROCESS_EVENT_POLL) {
          printf("Riprovo sottoscrizione...\n");
          if(obs_temp) coap_obs_remove_observee(obs_temp); // Rimuovi il vecchio osservatore
          // Crea una nuova sottoscrizione
          obs_temp = coap_obs_request_registration(&server_ep_temp,
                                             service_url_temp,
                                             handle_notification_temp,
                                             NULL);
      }
      
      // Usa la variabile globale intera per il controllo
      if(temp_tresh_x100 == -1 || shutdown) {
      // --- FINE MODIFICA ---
        printf("Spegnimento ventilazione...\n");
        if(obs_temp)
          coap_obs_remove_observee(obs_temp);
        process_exit(&coap_client_process);
        PROCESS_EXIT();
      }
    }

  } else {
// ... (codice invariato) ...
    printf("❌ Registrazione fallita dopo %d tentativi.\n", MAX_REGISTRATION_ATTEMPTS);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/