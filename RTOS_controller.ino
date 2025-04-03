#include <Bluepad32.h>

// set up pin connections
const int PWMA = 25;
const int PWMB = 26;
const int AIN2 = 18;
const int BIN2 = 19;
const int AIN1 = 21;
const int BIN1 = 22;
const int STBY = 23;
const int FIRE = 32;
const int LASR = 33;

// Declare global variables
int left_speed;
int right_speed;
bool right_forward;
bool left_forward;
bool fire;
bool lasr;
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Define PWM channels
const int PWM_CHANNEL_A = 0;
const int PWM_CHANNEL_B = 1;
const int PWM_FREQ = 5000;
const int PWM_RES = 8;

void processControllerTask();
void driveTask();

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                           properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl) {
    Serial.printf(
        "axis L: %4d, %4d, left_speed: %4d, right_speed: %4d, left_forward: %1d, right_forward: %1d, brake: %4d, throttle:%4d, laser:%1d, pump: %1d \n",
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        left_speed,
        right_speed,
        left_forward,
        right_forward,
        ctl->brake(),
        ctl->throttle(),
        lasr,
        fire
    );
}

void processGamepad(ControllerPtr ctl) {
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...
    if (ctl->a()) {
        static int colorIdx = 0;
        // Some gamepads like DS4 and DualSense support changing the color LED.
        // It is possible to change it by calling:
        switch (colorIdx % 3) {
            case 0:
                // Red
                ctl->setColorLED(255, 0, 0);
                break;
            case 1:
                // Green
                ctl->setColorLED(0, 255, 0);
                break;
            case 2:
                // Blue
                ctl->setColorLED(0, 0, 255);
                break;
        }
        colorIdx++;
    }

    if (ctl->x()) {
        // Some gamepads like DS3, DS4, DualSense, Switch, Xbox One S, Stadia support rumble.
        // It is possible to set it by calling:
        // Some controllers have two motors: "strong motor", "weak motor".
        // It is possible to control them independently.
        ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
                            0x80 /* strongMagnitude */);
    }

    //fire control
    if (ctl->brake() > 600){
      lasr = true;
      if (ctl->throttle() > 1000){
        fire = true;

        //rumble when firing.
        ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
                            0x80 /* strongMagnitude */);
      }
      else{
        fire = false;
      }
    }
    else{
      lasr = false;
      fire = false;
    } 


    //handle the joystick

    //drive forward and back
    if (ctl->axisY() > 0){
      left_forward = false;
      right_forward = false;
    }
    else{
      left_forward = true;
      right_forward = true;
    }
    
    //adjust speed
    left_speed = abs(ctl->axisY()) / 2;
    right_speed = abs(ctl->axisY()) / 2;

    //turning
    if (ctl->axisX() > 10 ){
      left_speed -= (abs(ctl->axisX()) / 4);
    }
    else if (ctl->axisX() < 10){
      right_speed -= (abs(ctl->axisX()) / 4);
    }
    if (ctl->axisX() < -400){
      left_forward = 0;
      right_forward = 0;
    }
    if (ctl->axisX() > 400){
      left_forward = 0;
      right_forward = 0;
    }

    //normalize
    if (left_speed > 255){
      left_speed = 255;
    }
    if (right_speed > 255){
      right_speed = 255;
    }
    if (left_speed < 0){
      left_speed =+ 255;
      right_forward = !right_forward;
    }
    if (right_speed < 0){
      right_speed =+ 255;
      left_forward = !right_forward;
    }


    // Another way to query controller data is by getting the buttons() function.
    // See how the different "dump*" functions dump the Controller info.
    dumpGamepad(ctl);
}

void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } else {
                Serial.println("Unsupported controller");
            }
        }
    }
}

void processControllerTask(void *parameter){
  // This call fetches all the controllers' data.
  // Call this function in your main loop.
  while(true){
    bool dataUpdated = BP32.update();
    if (dataUpdated)
        processControllers();
    vTaskDelay(10);
  }
}

// This function drives the motor contorller based on the global variables.
void driveTask(void *parameter) {
  while(true){
    //fire control
    if (lasr){
      digitalWrite(LASR,0);
    }
    else{
      digitalWrite(LASR,1);
    }

    if (fire & lasr){
      digitalWrite(FIRE,1);
    }
    else{
      digitalWrite(FIRE,0);
    }

    //right motor control
    if (right_forward) {
      digitalWrite(AIN1, 1);
      digitalWrite(AIN2, 0);
    } else {
      digitalWrite(AIN1, 0);
      digitalWrite(AIN2, 1);
    }
    ledcWrite(PWM_CHANNEL_A, right_speed);

    //left motor control
    if (left_forward) {
      digitalWrite(BIN1, 0);
      digitalWrite(BIN2, 1);
    } else {
      digitalWrite(BIN1, 1);
      digitalWrite(BIN2, 0);
    }
    ledcWrite(PWM_CHANNEL_B, left_speed);

    vTaskDelay(10);
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  Serial.printf("step 1 complete\n");
  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But it might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();

  // Enables mouse / touchpad support for gamepads that support them.
  // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
  // - First one: the gamepad
  // - Second one, which is a "virtual device", is a mouse.
  // By default, it is disabled.
  BP32.enableVirtualDevice(false);

  Serial.printf("step 2 complete\n");
  // put your setup code here, to run once:

  left_speed = 0;
  right_speed = 0;
  right_forward = true;
  left_forward = true;

  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(FIRE, OUTPUT);
  pinMode(LASR, OUTPUT);
  pinMode(13,OUTPUT); //Grounded PIN

  digitalWrite(13,LOW); //Grounded Pin

  digitalWrite(STBY, HIGH);
  digitalWrite(FIRE, LOW);

  Serial.printf("step 3 complete\n");
  // Configure PWM channels
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RES);

  // Attach PWM channels to GPIOs
  ledcAttachPin(PWMA, PWM_CHANNEL_A);
  ledcAttachPin(PWMB, PWM_CHANNEL_B);
  
  // RTOS tasks
  xTaskCreate(processControllerTask,"processController",2048,NULL,1,NULL);
  xTaskCreate(driveTask,"drive",2048,NULL,1,NULL);

  Serial.printf("step 4 complete\n");
}

void loop() {
  // put your main code here, to run repeatedly:

}
