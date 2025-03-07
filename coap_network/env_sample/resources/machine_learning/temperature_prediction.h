#ifndef TEMPERATURE_PREDICTION_H
#define TEMPERATURE_PREDICTION_H

float denormalized_prediction(float prediction);
float normalize(float value, float min, float max);
void update_sensor_values(float new_temperature, float new_humidity, float new_dust_density);
float predict_next_temperature();

float predict_next_temperature_from_values(float temp[], float hum[], float dust[]);
#endif // TEMPERATURE_PREDICTION_H