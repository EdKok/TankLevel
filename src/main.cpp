// Tank levels are supposed to be sent to Signal K as a "ratio" - a number between 0 and 1, where 0 is empty and 1 is full
// Sensors covered with glass or lens will need to be calibrated
#include <Wire.h>                               // IIC
#include "Adafruit_VL53L0X.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp_app_builder.h"
#include "sensesp/transforms/moving_average.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/transforms/lambda_transform.h"

using namespace sensesp;

reactesp::ReactESP app;

Adafruit_VL53L0X mySensor = Adafruit_VL53L0X();
uint delayTime = 1000;
SKOutput<float>* distance_output;
const char* sensor_config_path = "/sensors/vl53l0x/distance";
const char* sk_path = "tanks.blackwater.level";

float read_distance_callback() { return (mySensor.readRange()); }
int Height = 450; //Height of tank in mm

void setup() {
  Serial.begin(115200);
  delay(100);

  if (!mySensor.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  } else {
      Serial.println(F("Adafruit VL53L0X started"));
  }
  mySensor.setMeasurementTimingBudgetMicroSeconds(200000);

  SensESPAppBuilder builder;
  sensesp_app = builder.set_hostname("SensESP BlackWaterLevel")
                    ->set_wifi("", "")
                    ->set_sk_server("10.10.10.1", 3000)
                    ->enable_ota("myOTApwd")
                    // ->enable_wifi_signal_sensor()
                    ->get_app();

auto Lambda_function = [](int input) ->float{
  if (input <=8) { input = 0; }
  int Diff = (Height - input);
  float output = (float(Diff) / float(Height));
  return output;
};

auto Lambda_transform = new LambdaTransform<int, float>(Lambda_function);
auto* tanklevel = new RepeatSensor<int>(delayTime, read_distance_callback);           // returns mm
      tanklevel->connect_to(new Linear(1.0, -32, "/BlackWaterLevel/Linear"))          // mutiplier, offset
               ->connect_to(Lambda_transform)                                         // calculate ratio (0....1)
               ->connect_to(new SKOutputFloat(sk_path));

  sensesp_app->start();
}

void loop() {
  app.tick();
}
