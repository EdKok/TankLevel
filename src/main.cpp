// Tank levels are supposed to be sent to Signal K as a "ratio" - a number between 0 and 1, where 0 is empty and 1 is full
// Sensors covered with glass or a lens will need to be calibrated

#include "Wire.h"                               // IIC for sensor
#include "Display.h"                            // Separate file for outputting to display
#include "splash.h"                             // Show splash screen at startup
#include "Adafruit_VL53L0X.h"                   // The driver for the sensor
#include "sensesp/signalk/signalk_output.h"
#include "sensesp_app_builder.h"
#include "sensesp/transforms/moving_average.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/transforms/lambda_transform.h"
#include "sensesp/net/ws_client.h"

using namespace sensesp;

reactesp::ReactESP app;

Adafruit_VL53L0X mySensor = Adafruit_VL53L0X();
uint delayTime = 2000;
SKOutput<float>* distance_output;
const char* sensor_config_path = "/sensors/vl53l0x/distance";
const char* sk_path = "tanks.blackwater.level";

int read_distance_callback() {
    int distance = mySensor.readRange();
    Serial.print("Distance: "); Serial.println(distance); // Print the distance
    return distance;
}

void setup() {
    Serial.begin(115200);
    delay(100);

    // Initialize display driver and show splash screen
    u8g2.begin();
    u8g2.firstPage();
    do {
        u8g2.drawXBMP(0, 0, 128, 64, u8g2_logo);  // Show splash during startup
    } while (u8g2.nextPage());

    // Initialize the sensor and handle failure
    if (!mySensor.begin()) {
        Serial.println(F("Failed to init VL53L0X"));
        while (1) {
            delay(100);  // Add a delay to avoid busy-waiting
        }
    } else {
        Serial.println(F("Adafruit VL53L0X started"));
    }

    // Set high accuracy for the sensor
    mySensor.setMeasurementTimingBudgetMicroSeconds(200000);

    // Build the SensESP application
    SensESPAppBuilder builder;
    sensesp_app = builder.set_hostname("SensESP BlackWaterLevel")
                         ->set_wifi("SSID", "password")
                         ->set_sk_server("10.10.10.1", 3000)
                         ->enable_ota("myOTApwd")
                         //->enable_wifi_signal_sensor()
                         ->get_app();

    // Lambda function to calculate the tank level ratio
    int oldOutput = 0;                                  // Initialize oldOutput
    bool firstRun = true;                               // Flag to check if it's the first run

    auto Lambda_function = [oldOutput, firstRun](int input, int deadzone, int height) mutable -> float {
        if (input <= deadzone) { input = 0; }
        if (input > height) { input = height; }
        int output = (height - input) * 100 / height;   // From ratio to percent and convert to integer

        if ((firstRun) && (output > 0)) {               // Check if it's the first run and the output is greater than 0
            oldOutput = output;                         // Initialize oldOutput with the actual value on the first run
            firstRun = false;                           // Set the flag to false after the first run
        } else {
            if (output > oldOutput) {          
                output = oldOutput + 1;
                if (output > 100) { output = 100; }
            } else if (output < oldOutput) {
                output = oldOutput - 1;
                if (output < 0) { output = 0; }
            }
        }

        oldOutput = output;                              // Save the actual value for the next run

        Display_function(output);                        // Pass the calculated value to Display_function
        return static_cast<float>(output) / 100.0f;      // Ensure floating-point division
    };

    // Parameters for the lambda transform
    const ParamInfo* lambda_param_data = new ParamInfo[2]{
        {"deadzone", "DeadZone of sensor in mm"}, {"height", "Height of tank in mm"}
    };

    // Create the lambda transform
    auto Lambda_transform = new LambdaTransform<int, float, int, int>(
         Lambda_function, 46, 400, lambda_param_data, "/BlackWaterLevel/Params"
    );

    // Create the tank level sensor and connect the transforms
    auto* tanklevel = new RepeatSensor<int>(delayTime, read_distance_callback);         // returns mm

          tanklevel->connect_to(new Linear(1, 0, "/BlackWaterLevel/Linear"))            // mutiplier (1), offset (-30)
                   ->connect_to(Lambda_transform)                                       // calculate ratio (0....1)
                   ->connect_to(new SKOutputFloat(
                    sk_path,                                                            // Signal K path
                    new SKMetadata("ratio",                                             // Define output units
                                   "Blackwater Tank Level")                             // Value description
                   ));
                            
    sensesp_app->start();                                                               // Start the SensESP application
}

void loop() {
    app.tick();
}
