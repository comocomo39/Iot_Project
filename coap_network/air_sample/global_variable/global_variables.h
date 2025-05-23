#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

// Struttura per memorizzare i dati del sensore di qualità dell'aria
typedef struct {
    float co;           // MQ7_CO (monossido di carbonio)
    float air_quality;  // MQ135_AirQuality
    float tvoc;         // TVOC (composti organici volatili)
} AirSample;

// Dichiarazione della variabile globale per i dati del sensore di qualità dell'aria
extern AirSample air_sample;  

#endif /* GLOBAL_VARIABLES_H */
