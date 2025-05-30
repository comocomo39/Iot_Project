#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

// Struttura per i dati del sensore ambientale (Environmental Sensor)
typedef struct {
    int timeid;
    float temperature;   // Temperatura
    float humidity;      // Umidità
    float dust_density;  // Densità della polvere
} Sample;
extern int sample_count;
extern Sample env_samples[10]; // Array per memorizzare gli ultimi 10 campioni ambientali

/* ---- Normalizzazione temperature / humidity / dust / gas ---- */
/* ---- range validi sensore ---- */
#define TEMP_MIN   17.0f
#define TEMP_MAX   32.0f
#define HUM_MIN    30.0f
#define HUM_MAX    95.0f
#define DUST_MIN    0.0f
#define DUST_MAX  500.0f



#endif /* GLOBAL_ENV_VARIABLES_H */
