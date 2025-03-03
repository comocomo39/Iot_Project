#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

// Variabili globali per il sistema di temperatura e ventilazione
extern float temp_tresh;  // Soglia della temperatura per attivare la ventilazione
extern int nRisktemp;     // Numero di volte in cui la temperatura è stata sopra la soglia
extern int shutdown;      // Variabile di spegnimento del sistema

#endif /* GLOBAL_VARIABLES_H */
