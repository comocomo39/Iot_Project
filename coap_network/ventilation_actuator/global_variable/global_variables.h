#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

// Variabili globali per il sistema di temperatura e ventilazione
extern float temp_tresh;  // Soglia della temperatura per attivare la ventilazione
extern int nRisktemp;     // Numero di volte in cui la temperatura è stata sopra la soglia
extern int shutdown;      // Variabile di spegnimento del sistema
extern int high_temp_count; // Numero di iterazioni con temperatura elevata
extern int low_temp_count;  // Numero di iterazioni con temperatura bassa

#endif /* GLOBAL_VARIABLES_H */
