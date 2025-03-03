#ifndef GLOBAL_ENV_VARIABLES_H
#define GLOBAL_ENV_VARIABLES_H

// Struttura per i dati del sensore ambientale (Environmental Sensor)
typedef struct {
    int timeid;
    float temperature;   // Temperatura
    float humidity;      // Umidità
    float dust_density;  // Densità della polvere
} Sample;

extern Sample env_samples[10]; // Array per memorizzare gli ultimi 10 campioni ambientali

#endif /* GLOBAL_ENV_VARIABLES_H */
