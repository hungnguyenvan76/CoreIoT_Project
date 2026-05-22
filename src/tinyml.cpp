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
                        case 4: class_name = "AC_ON"; break;
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

float generate_random(float min, float max) {
    return min + (float)random(1000) / 1000.0 * (max - min);
}

void evaluate_tinyml_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(5000)); // Delay 5s before starting to open monitor
    
    Serial.println("==================================================");
    Serial.println("Starting TinyML Evaluation Task...");
    Serial.println("==================================================");
    
    setupTinyML();
    
    int correct_predictions[5] = {0};
    int total_predictions[5] = {0};
    int confusion_matrix[5][5] = {0};
    uint64_t total_inference_time = 0; // Microseconds

    String class_names[5] = {"NORMAL", "FIRE_RISK", "MOLD_RISK", "SENSOR_ERROR", "HVAC_ON"};

    for (int cls = 0; cls < 5; cls++) {
        Serial.printf("\n--- Evaluating Class %d: %s ---\n", cls, class_names[cls].c_str());
        int wrong_printed = 0;
        
        for (int sample = 0; sample < 200; sample++) {
            float window_data[WINDOW_SIZE][NUM_FEATURES];
            
            // Generate synthetic data
            float T_base, H_base, T_slope = 0, H_slope = 0;
            switch(cls) {
                case 0: // NORMAL - Push to boundaries with high noise
                    T_base = generate_random(25.0, 37.0); 
                    H_base = generate_random(54.0, 76.0); 
                    break;
                case 1: // FIRE_RISK - Sudden exponential-like jump
                    T_base = generate_random(29.0, 36.0); 
                    T_slope = generate_random(1.0, 2.5);
                    H_base = generate_random(35.0, 50.0);
                    H_slope = generate_random(2.0, 4.0);
                    break;
                case 2: // MOLD_RISK - Extremely high humidity
                    T_base = generate_random(20.0, 36.0);
                    H_base = generate_random(85.0, 98.0);
                    break;
                case 3: // SENSOR_ERROR - Stuck values or huge jumps
                    T_base = generate_random(-20.0, 100.0);
                    H_base = generate_random(0.0, 100.0);
                    break;
                case 4: // HVAC_ON - Start hot, rapid drop, then stable
                    T_base = generate_random(28.0, 32.0);
                    H_base = generate_random(60.0, 70.0);
                    break;
            }
            
            for (int step = 0; step < WINDOW_SIZE; step++) {
                if (cls == 0) { // NORMAL
                    window_data[step][0] = T_base + generate_random(-1.0, 1.0);
                    window_data[step][1] = H_base + generate_random(-2.0, 2.0);
                }
                else if (cls == 1) { // FIRE_RISK
                    window_data[step][0] = T_base + T_slope * step + (step > 5 ? generate_random(1.0, 3.0) : 0) + generate_random(-0.5, 0.5);
                    window_data[step][1] = H_base - H_slope * step - (step > 5 ? generate_random(2.0, 5.0) : 0) + generate_random(-1.0, 1.0);
                } 
                else if (cls == 2) { // MOLD_RISK
                    window_data[step][0] = T_base + generate_random(-0.5, 0.5);
                    window_data[step][1] = H_base + generate_random(-1.0, 1.0);
                }
                else if (cls == 3) { // SENSOR_ERROR
                    if (random(100) < 30) {
                        window_data[step][0] = generate_random(-50.0, -10.0);
                        window_data[step][1] = generate_random(120.0, 150.0);
                    } else if (random(100) < 30) {
                        window_data[step][0] = T_base;
                        window_data[step][1] = H_base;
                    } else {
                        window_data[step][0] = T_base + generate_random(-5.0, 5.0);
                        window_data[step][1] = H_base + generate_random(-10.0, 10.0);
                    }
                } 
                else if (cls == 4) { // HVAC_ON
                    if (step < 4) {
                        T_base -= generate_random(0.5, 1.5);
                        H_base -= generate_random(1.0, 2.5);
                    }
                    window_data[step][0] = T_base + generate_random(-0.5, 0.5);
                    window_data[step][1] = H_base + generate_random(-1.0, 1.0);
                }
            }
            
            // Feed to model
            int tensor_idx = 0;
            for (int i = 0; i < WINDOW_SIZE; i++) {
                input->data.f[tensor_idx++] = window_data[i][0];
                input->data.f[tensor_idx++] = window_data[i][1];
            }
            
            // Invoke and measure time
            uint32_t start_time = micros();
            TfLiteStatus invoke_status = interpreter->Invoke();
            uint32_t end_time = micros();
            
            if (invoke_status != kTfLiteOk) {
                Serial.println("[AI] Invoke failed!");
                continue;
            }
            total_inference_time += (end_time - start_time);
            
            // Predict
            float max_confidence = -100.0;
            int predicted_class = 0;
            for(int i = 0; i < 5; i++) {
                float confidence = output->data.f[i];
                if(confidence > max_confidence) {
                    max_confidence = confidence;
                    predicted_class = i;
                }
            }
            
            // Record results
            total_predictions[cls]++;
            confusion_matrix[cls][predicted_class]++;
            if (predicted_class == cls) {
                correct_predictions[cls]++;
            }
            
            if (predicted_class != cls && wrong_printed < 3) { 
                char out_buf[512];
                int offset = 0;
                
                wrong_printed++;
                offset += snprintf(out_buf + offset, sizeof(out_buf) - offset, "WRONG Predict: Sample %d -> True: %s, Predict: %s (Conf: %.1f%%)\n", sample+1, class_names[cls].c_str(), class_names[predicted_class].c_str(), max_confidence*100.0);
                
                offset += snprintf(out_buf + offset, sizeof(out_buf) - offset, "  T: [");
                for (int j=0; j<WINDOW_SIZE; j++) {
                    offset += snprintf(out_buf + offset, sizeof(out_buf) - offset, "%.1f%s", window_data[j][0], (j < WINDOW_SIZE-1) ? ", " : "]\n");
                }
                
                offset += snprintf(out_buf + offset, sizeof(out_buf) - offset, "  H: [");
                for (int j=0; j<WINDOW_SIZE; j++) {
                    offset += snprintf(out_buf + offset, sizeof(out_buf) - offset, "%.1f%s", window_data[j][1], (j < WINDOW_SIZE-1) ? ", " : "]\n");
                }
                
                Serial.print(out_buf);
                Serial.flush();
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            
        }
    }
    
    // Print Summary
    Serial.println("\n==================================================");
    Serial.println("EVALUATION RESULTS");
    Serial.println("==================================================");
    
    int total_correct = 0;
    int total_samples = 0;
    
    for (int cls = 0; cls < 5; cls++) {
        total_correct += correct_predictions[cls];
        total_samples += total_predictions[cls];
        float acc = (float)correct_predictions[cls] / total_predictions[cls] * 100.0;
        Serial.printf("Class %d (%s) Accuracy: %.2f%% (%d/%d)\n", cls, class_names[cls].c_str(), acc, correct_predictions[cls], total_predictions[cls]);
    }
    
    float overall_acc = (float)total_correct / total_samples * 100.0;
    Serial.println("--------------------------------------------------");
    Serial.printf("OVERALL ACCURACY: %.2f%% (%d/%d)\n", overall_acc, total_correct, total_samples);
    
    // Print timing metrics
    float avg_time_ms = (float)total_inference_time / total_samples / 1000.0;
    Serial.printf("AVG INFERENCE TIME: %.2f ms/sample\n", avg_time_ms);
    Serial.println("==================================================");
    
    // Print Confusion Matrix
    Serial.println("Confusion Matrix:");
    Serial.println("              | NORMAL | FIRE_RISK | MOLD_RISK | SENSOR_ERR| HVAC_ON");
    for (int i = 0; i < 5; i++) {
        Serial.printf("%-13s | %-6d | %-9d | %-9d | %-10d | %-7d\n", 
                      class_names[i].c_str(), 
                      confusion_matrix[i][0], 
                      confusion_matrix[i][1], 
                      confusion_matrix[i][2], 
                      confusion_matrix[i][3], 
                      confusion_matrix[i][4]);
        Serial.flush();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    Serial.println("==================================================");

    // Stop task
    vTaskDelete(NULL);
}