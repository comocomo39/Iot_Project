#include "contiki.h"
#include "danger_air_classification.h"
#include "eml_trees.h"
#include <stdio.h>

// Funzione per predire il livello di pericolo
int predict_class(float* features, int len) {
    const int32_t predicted_class = eml_trees_predict(&danger_air_classification, features, len);
    int value_predicted = (int)predicted_class;

    printf("🔮 Predizione Modello ML: %i\n", value_predicted);
    return value_predicted;
}
