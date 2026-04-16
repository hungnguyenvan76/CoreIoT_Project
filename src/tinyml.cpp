#include "tinyml.h"

// Globals, for the convenience of one-shot setup.
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 32 * 1024; // Adjust size based on your model
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

    while (1)
    {
        if(sensorQueue && xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
            if(receivedData.temperature == -1 && receivedData.humidity == -1) {
                Serial.println("[AI] Sensor Error Detected! Skipping inference.");
            } else {
                Serial.printf("\nSensor -> Temp: %.2f *C | Humidity: %.2f %%\n", receivedData.temperature, receivedData.humidity);

                // CHUẨN HÓA DỮ LIỆU - TRÁNH TRÀN SỐ SOFTMAX
                float t_scaled = receivedData.temperature / 100.0;
                float h_scaled = receivedData.humidity / 100.0;

                // Copy sensor data to input tensor
                if (input->type == kTfLiteFloat32) {
                    input->data.f[0] = t_scaled;
                    input->data.f[1] = h_scaled;
                } else if (input->type == kTfLiteInt8) {
                    // Quantize Float32 to Int8
                    input->data.int8[0] = (int8_t)(t_scaled / input->params.scale + input->params.zero_point);
                    input->data.int8[1] = (int8_t)(h_scaled / input->params.scale + input->params.zero_point);
                }

                // Run inference
                TfLiteStatus invoke_status = interpreter->Invoke();
                if (invoke_status != kTfLiteOk)
                {
                    error_reporter->Report("Invoke failed");
                } else{
                    Serial.printf("\n[DEBUG] Kieu Input: %d | Kieu Output: %d | So Node Output: %d\n", input->type, output->type, output->dims->data[1]);
                    float max_confidence = -100.0;
                    int predicted_class = 0;
                    
                    for(int i = 0; i < 4; i++) {
                        float confidence = 0.0;
                        if (output->type == kTfLiteFloat32) {
                            confidence = output->data.f[i];
                        } else if (output->type == kTfLiteInt8) {
                            // Dequantize Int8 to ratio % Float32
                            confidence = (output->data.int8[i] - output->params.zero_point) * output->params.scale;
                        }
                        if(confidence > max_confidence) {
                            max_confidence = confidence;
                            predicted_class = i;
                        }
                    }
                    
                    switch (predicted_class) {
                        case 0:
                            Serial.printf("NORMAL (Tự tin: %.0f%%)\n", max_confidence * 100);
                            break;
                        case 1:
                            Serial.printf("SENSOR ERROR (Tự tin: %.0f%%)\n", max_confidence * 100);
                            break;
                        case 2:
                            Serial.printf("FIRE RISK/HOT (Tự tin: %.0f%%)\n", max_confidence * 100);
                            break;
                        case 3:
                            Serial.printf("MOLD/WET (Tự tin: %.0f%%)\n", max_confidence * 100);
                            break;
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}