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
#define GOOD_ACK 65
#define SERVER_EP "coap://[fd00::1]:5683"
#define MAX_REGISTRATION_ENTRY 5

static char *service_url_temp = "predict-temp";
static char  sensor_addr_str[64] = "";         /* ← endpoint reale del sensore */

static float predicted_temperature = 0;
static int   registered              = 0;
static int   registration_retry_count = 0;

int   high_temp_count = 0;
int   low_temp_count  = 0;
float temp_tresh      = 31.5;   /* soglia °C per attivare la ventilazione */
int   nRisktemp       = 0;
int   shutdown        = 0;

/* Risorse CoAP locali (threshold / shutdown) */
extern coap_resource_t res_tresh;
extern coap_resource_t res_shutdown;

/* Prototipi */
void registration_handler(coap_message_t *response);
void handle_notification_temp(struct coap_observee_s *observee,
                              void *notification,
                              coap_notification_flag_t flag);

/*---------------------------------------------------------------------------*/
PROCESS(coap_client_process, "CoAP Client Process");
AUTOSTART_PROCESSES(&coap_client_process);

static coap_observee_t *obs_temp = NULL;
/*---------------------------------------------------------------------------*/
/* Funzione di controllo della ventilazione ---------------------------------*/
void
control_ventilation(float predicted_temp, float threshold)
{
  if(predicted_temp < threshold) {
    low_temp_count++;
    high_temp_count = 0;

    if(low_temp_count >= 2) {
      printf("Temperatura stabile (%.2f°C). Ventilazione SPENTA.\n",
             predicted_temp);
      leds_single_off(LEDS_RED);
      leds_single_on(LEDS_GREEN);
    }
  } else if(predicted_temp < threshold +2) {
    high_temp_count++;
    low_temp_count = 0;

    printf("Temperatura alta (%.2f°C). Ventilazione MEDIA attivata.\n",
           predicted_temp);
    leds_single_off(LEDS_GREEN);
    leds_single_on(LEDS_RED);

  } else {
    high_temp_count++;
    low_temp_count = 0;

    if(high_temp_count >= 3) {
      printf("Temperatura molto alta (%.2f°C). Ventilazione MASSIMA attivata!\n",
             predicted_temp);
      leds_single_off(LEDS_GREEN);
      leds_single_on(LEDS_RED);
    }
  }
}
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

  char temp_str[len + 1];
  memcpy(temp_str, chunk, len);
  temp_str[len] = '\0';

  predicted_temperature = atof(temp_str);

  printf("Temperatura Predetta: %.2f°C\n", predicted_temperature);
  control_ventilation(predicted_temperature, temp_tresh);
}
/*---------------------------------------------------------------------------*/
/* Handler della risposta di registrazione ----------------------------------*/
void
registration_handler(coap_message_t *response)
{
  if(response == NULL) {
    printf("Nessuna risposta dal server di registrazione.\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);

  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  printf("📡 Risposta dal server di registrazione: %s\n", payload);

  /* Estrai l'indirizzo IPv6 del sensore dal campo "e" del JSON */
  cJSON *root = cJSON_Parse(payload);
  if(root) {
    const cJSON *endp = cJSON_GetObjectItem(root, "e");
    if(endp && cJSON_IsString(endp)) {
      strncpy(sensor_addr_str, endp->valuestring, sizeof(sensor_addr_str) - 1);
      sensor_addr_str[sizeof(sensor_addr_str) - 1] = '\0';
      printf("Endpoint sensore salvato: %s\n", sensor_addr_str);
    } else {
      printf("Campo \"e\" non trovato nel JSON!\n");
    }
    cJSON_Delete(root);
  }

  if(response->code == GOOD_ACK) {
    registered = 1;
    printf("Registrazione completata con successo.\n");
  } else {
    printf("Registrazione fallita (code %u).\n", response->code);
  }
}
/*---------------------------------------------------------------------------*/
/* Callback delle notifiche Observe -----------------------------------------*/
void
handle_notification_temp(struct coap_observee_s *observee,
                         void *notification,
                         coap_notification_flag_t flag)
{
  if(notification) {
    printf("Notifica ricevuta dalla predizione della temperatura.\n");
    response_handler_temp((coap_message_t *)notification);
  } else {
    printf("Nessuna notifica ricevuta.\n");
  }
}
/*---------------------------------------------------------------------------*/
/* Processo principale ------------------------------------------------------*/
PROCESS_THREAD(coap_client_process, ev, data)
{
  static coap_endpoint_t server_ep;
  static coap_message_t  request[1];
  static struct etimer   registration_timer;

  PROCESS_BEGIN();

  leds_single_on(LEDS_RED);  /* LED rosso acceso per indicare avvio */

  PROCESS_WAIT_EVENT_UNTIL(ev == button_hal_press_event);

  leds_single_off(LEDS_RED); /* LED rosso spento dopo il pulsante */

  /* Endpoint del server centrale (per la POST di registrazione) */
  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

  /* ---------------- Registrazione ---------------- */
  while(registration_retry_count < MAX_REGISTRATION_ENTRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/registrationActuator");
    coap_set_payload(request, (uint8_t *)"ventilation", strlen("ventilation"));

    printf("📡 Invio richiesta di registrazione...\n");
    COAP_BLOCKING_REQUEST(&server_ep, request, registration_handler);

    if(!registered) {
      registration_retry_count++;
      etimer_set(&registration_timer, CLOCK_SECOND * 10);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));
    }
  }

  /* ---------------- Dopo la registrazione ---------------- */
  if(registered) {
    printf("✅ Ventilatore registrato con successo.\n");

    /* 1. Costruisci l'endpoint del sensore usando l'indirizzo salvato */
    static coap_endpoint_t server_ep_temp;
    char addr_temp[128];
    snprintf(addr_temp, sizeof(addr_temp),
             "coap://[%s]:5683", sensor_addr_str);
    coap_endpoint_parse(addr_temp, strlen(addr_temp), &server_ep_temp);

    printf("Invio richiesta di osservazione su %s\n", addr_temp);

    /* 2. Richiesta Observe su predict-temp */
    obs_temp = coap_obs_request_registration(&server_ep_temp,
                                             service_url_temp,
                                             handle_notification_temp,
                                             NULL);

    /* 3. Attiva le risorse locali */
    coap_activate_resource(&res_tresh,   "threshold");
    coap_activate_resource(&res_shutdown,"shutdown");

    /* Loop principale del processo */
    while(1) {
      PROCESS_WAIT_EVENT();
      if(temp_tresh ==-1 || shutdown) {
        printf("Spegnimento ventilazione...\n");
        if(obs_temp) {
          coap_obs_remove_observee(obs_temp);
        }
        process_exit(&coap_client_process);
        PROCESS_EXIT();
      }
    }
  } else {
    printf("Registrazione fallita dopo %d tentativi.\n",
           MAX_REGISTRATION_ENTRY);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
