#include "tinyml.h"

// Globals, for the convenience of one-shot setup.
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; // Adjust size based on your model
    uint8_t tensor_arena[kTensorArenaSize];
} // namespace

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht20_anomaly_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters)
{
    setupTinyML();
    QueueHandle_t sensorQueue = (QueueHandle_t)pvParameters;
    SensorData_t receivedData;

    while (1){
        if(sensorQueue && xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
            if(receivedData.temperature == -1 && receivedData.humidity == -1) {
                Serial.println("[AI] Sensor Error Detected! Skipping inference.");
            } else {
                // DATA NORMALIZATION - AVOID SOFTMAX OVERFLOW
                float t_scaled = receivedData.temperature / 100.0;
                float h_scaled = receivedData.humidity / 100.0;

                // Copy sensor data to input tensor
                input->data.f[0] = t_scaled;
                input->data.f[1] = h_scaled;

                // Run inference
                TfLiteStatus invoke_status = interpreter->Invoke();
                if (invoke_status != kTfLiteOk)
                {
                    error_reporter->Report("Invoke failed");
                } else{
                    float max_confidence = -100.0;
                    int predicted_class = 0;
                    
                    for(int i = 0; i < 4; i++) {
                        float confidence = 0.0;
                        confidence = output->data.f[i];

                        if(confidence > max_confidence) {
                            max_confidence = confidence;
                            predicted_class = i;
                        }
                    }
                    
                    switch (predicted_class) {
                        case 0:
                            Serial.printf("[AI] Predict: NORMAL (Confidence: %.0f%%)\n", max_confidence * 100);
                            break;
                        case 1:
                            Serial.printf("[AI] Predict: SENSOR ERROR (Confidence: %.0f%%)\n", max_confidence * 100);
                            break;
                        case 2:
                            Serial.printf("[AI] Predict: MOLD/WET (Confidence: %.0f%%)\n", max_confidence * 100);
                            break;
                        case 3:
                            Serial.printf("[AI] Predict: FIRE RISK/HOT (Confidence: %.0f%%)\n", max_confidence * 100);
                            break;
                    }
                }
            }
        }
    
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}