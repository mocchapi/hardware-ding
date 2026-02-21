const bool UP = true;
const bool DOWN = false;

const int SWITCH_COUNT = 5;

// PINs for a single switch
struct Switch {
  int up;
  int down;
  int touch;
};

// PINs for a single rotary encoder
struct Rotary {
  int a;
  int b;
  int press; // Button pin
};

// Attaches functionality to a switch+
struct Action {
  String name;
  bool uses_rotary;
  
  void (*rotary_callback)(Action*, float* change, bool* is_pressed);   // positive or negative change in rotary steps
  void (*switch_callback)(Action*, bool* direction, bool* is_pressed); // UP/DOWN, True/False for on-off
  void (*touch_callback)(Action*, bool* state); // Called when switch is touched and let go

};

struct Page {
  String name;
  Action actions[SWITCH_COUNT];
};


const Switch switches[SWITCH_COUNT] = [
  Switch{1, 2, 3}
];
Rotary rotary;



void setup() {
  Serial.begin(9600);
  Serial.println("Hardware-ding v0.0.0");
  Serial.println(SWITCH_COUNT + " switches, 1 rotary encoder, 1 display");

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("joe biden");

}
