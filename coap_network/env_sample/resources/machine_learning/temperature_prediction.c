#include "temperature_prediction.h"
#include "contiki.h"
#include "random.h"
#include "temperature_prediction_model.h"
#include "eml_trees.h"
#include <stdio.h>

// Definizione dei range di normalizzazione
#define TEMP_MIN   5.0f
#define TEMP_MAX   35.0f
#define HUM_MIN    30.0f
#define HUM_MAX    95.0f
#define DUST_MIN    0.0f
#define DUST_MAX  500.0f



static float temperatures[10];
static float humidities[10];
static float dust_densities[10];
static int index = 0;
static float input[30];

// Funzione per normalizzare i valori
float normalize(float value, float min, float max) {
    return (value - min) / (max - min);
}

// Funzione per denormalizzare il valore predetto
float denormalized_prediction(float prediction) {
    return prediction * (TEMP_MAX - TEMP_MIN) + TEMP_MIN;
}

// Funzione per aggiornare i valori del sensore
void update_sensor_values(float new_temperature, float new_humidity, float new_dust) {
    temperatures[index] = normalize(new_temperature, TEMP_MIN, TEMP_MAX);
    humidities[index] = normalize(new_humidity, HUM_MIN, HUM_MAX);
    dust_densities[index] = normalize(new_dust, DUST_MIN, DUST_MAX);
    index = (index + 1) % 10;
}

// Funzione per predire la temperatura futura
float predict_next_temperature_from_values(float temp[], float hum[], float dust[]) {

    for (int i = 0; i < 10; i++) {
        input[i] = temp[i];
        input[10 + i] = hum[i];
        input[20 + i] = dust[i];
    }

    float predicted_temperature = eml_trees_regress1(&temperature_prediction_model, input, 30);
    return predicted_temperature;
}
