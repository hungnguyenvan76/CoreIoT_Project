#include "tinyml.h"

// Globals, for the convenience of one-shot setup.
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 16 * 1024; // Adjust size based on your model
    uint8_t tensor_arena[kTensorArenaSize];

    // RING BUFFER FOR TIME-SERIES
    constexpr int WINDOW_SIZE = 10;
    constexpr int NUM_FEATURES = 2;
    float ring_buffer[WINDOW_SIZE][NUM_FEATURES] = {0}; // Mảng 2 chiều lưu lịch sử
    int data_count = 0; // Biến đếm số điểm dữ liệu đã thu thập
} // namespace

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_2_tflite);
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
                //Serial.println("[AI] Sensor Error Detected! Skipping inference.");
                receivedData.temperature = 999.0;
                receivedData.humidity = -999.0;
            } 
            
            // Dịch toàn bộ dữ liệu lịch sử sang trái 1 ô
            for (int i = 0; i < WINDOW_SIZE - 1; i++) {
                ring_buffer[i][0] = ring_buffer[i + 1][0];
                ring_buffer[i][1] = ring_buffer[i + 1][1];
            }
            
            // Chèn dữ liệu mới nhất vào ô cuối cùng
            ring_buffer[WINDOW_SIZE - 1][0] = receivedData.temperature;
            ring_buffer[WINDOW_SIZE - 1][1] = receivedData.humidity;
            if (data_count < WINDOW_SIZE) data_count++; // Tăng biến đếm (Tối đa bằng 10)
            
            // Đủ dữ liệu chuỗi thời gian, bắt đầu chạy AI
            if (data_count == WINDOW_SIZE) {
                // Trải phẳng mảng 2 chiều (10x2) vào Input Tensor 1 chiều (20 điểm)
                int tensor_idx = 0;
                for (int i = 0; i < WINDOW_SIZE; i++) {
                    input->data.f[tensor_idx++] = ring_buffer[i][0];
                    input->data.f[tensor_idx++] = ring_buffer[i][1];
                }

                // Chạy AI
                TfLiteStatus invoke_status = interpreter->Invoke();
                if (invoke_status != kTfLiteOk) {
                    Serial.println("[AI] Invoke failed!");
                } else {
                    float max_confidence = -100.0;
                    int predicted_class = 0;
                    
                    // Quét 5 class để tìm xác suất cao nhất
                    for(int i = 0; i < 5; i++) {
                        float confidence = output->data.f[i];
                        if(confidence > max_confidence) {
                            max_confidence = confidence;
                            predicted_class = i;
                        }
                    }
                    
                    String class_name = "";
                    switch (predicted_class){
                        case 0: class_name = "NORMAL"; break;
                        case 1: class_name = "FIRE_RISK"; break;
                        case 2: class_name = "MOLD_RISK"; break;
                        case 3: class_name = "SENSOR_ERROR"; break;
                        case 4: class_name = "HVAC_ON"; break;
                    }

                    String aiMsg = "[AI] Predict: " + class_name + " (Confidence: " + String(max_confidence * 100, 0) + "%)";
                    Serial.println(aiMsg);

                    // GHI KẾT QUẢ VÀO aiQueue
                    if (aiQueue != NULL){
                        xQueueOverwrite(aiQueue, &predicted_class);
                    }
                }
            } else{
                Serial.printf("[AI] Đang gom dữ liệu chuỗi thời gian... (%d/%d)\n", data_count, WINDOW_SIZE);
            }
    
        vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}