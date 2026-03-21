#include <wifi.h>
#include <mqtt.h>
#include <Matter.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//// Stuff you may wanna change

// Set amount of switches in use
const int SWITCH_COUNT = 5;
// Amount of pages (action layers)
const int PAGE_COUNT = 2;
// Values higher than this are considered touched
const uint32_t TOUCH_THRESHOLD = 3000;

// OLED pins
const int SCREEN_SDA = 11;
const int SCREEN_SCL = 12;
// OLED i2c address
//const int SCREEN_ADDRESS = 0x78;
const int SCREEN_ADDRESS = 0x3C;
// OLED dimensions
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;


//// Structs

// PINs for a single switch
struct PanelSwitch {
  int up;
  int down;
  int touch;
  int id=0;
};
// For detecting changes in switch state between polls
struct PanelSwitchState {
  bool up;
  bool down;
  bool touch;
  int id=0;
};

// PINs for a single rotary encoder
struct PanelRotary {
  int a;
  int b;
  int press; // Button pin
};
// TODO: RotaryState, probably do hardware monitoring

// Attaches functionality to a switch+rotary
struct Action {
  String name; // Name to display when switch is touched
  String switch_description = ""; // Description to display when switch is touched
  String rotary_description = ""; // Description to display when switch is touched first, and then rotary encoder is touched
  bool uses_rotary;

  void (*rotary_callback)(Action* action, float change, bool is_pressed); // Called when rotary encoder is changed
  void (*switch_callback)(Action* action, PanelSwitchState prev_state, PanelSwitchState new_state); // Called when the switch state changes (moved up/down/neutral)
  void (*touch_callback)(Action* action, PanelSwitchState prev_state, PanelSwitchState new_state); // Called when the switch is touched
};

struct Page {
  String name;
  Action actions[SWITCH_COUNT] = {};
};


const PanelSwitch switches[SWITCH_COUNT] = {
  PanelSwitch{38,37, 1},
  PanelSwitch{38,37, 2},
  PanelSwitch{38,37, 4},
  PanelSwitch{38,37, 5},
  PanelSwitch{38,37, 6},
};


// State
int current_page = 0;
Page pages[PAGE_COUNT] = {};

PanelSwitchState switch_states[SWITCH_COUNT] = {};

volatile bool was_touched[SWITCH_COUNT] = {};

PanelRotary rotary;

// Oled display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  // Init serial, send some signs of life*0.8
  Serial.begin(9600);
  Serial.println("Hardware-ding v0.0.0");
  Serial.print(SWITCH_COUNT);
  Serial.println(" switches, 1 rotary encoder, 1 display");

  // Initialise OLED display
  Serial.println("Screen init:");
  Serial.print(" - SDA: ");
  Serial.println(SCREEN_SDA);
  Serial.print(" - SCL: ");
  Serial.println(SCREEN_SCL);
  Serial.print(" - ADDR: ");
  Serial.println(SCREEN_ADDRESS);

  Wire.begin(SCREEN_SDA, SCREEN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Screen initialization failed");
    while (true); // Stop execution if display fails to initialize
    // (or not? seems to never fail despite HW fail)
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialise pinmodes for each switch
  for (int i = 0; i < SWITCH_COUNT; i++) {
    PanelSwitch target = switches[i];
    was_touched[i] = false;
    target.id = i;

    Serial.print("Registering switch ");
    Serial.print(i);
    Serial.println(" with pins:");
    Serial.print("  up: ");
    Serial.println(target.up);
    Serial.print("  down: ");
    Serial.println(target.down);
    Serial.print("  touch: ");
    Serial.println(target.touch);

    pinMode(target.up, INPUT_PULLUP);
    pinMode(target.down, INPUT_PULLUP);
    int ID = i;
    //touchAttachInterruptArg(target.touch, touch_interrupt, &ID, TOUCH_THRESHOLD);

    // Poll initial state
    switch_states[i] = poll_switch(target);
  }

  // Define pages & actions
  pages[0] = Page{
    "First page",
    {
      Action{"Action 1", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback, default_touch_callback},
      Action{"Action 2", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback, default_touch_callback},
      Action{"Action 3", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback, default_touch_callback},
      Action{"Action 4", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback, default_touch_callback},
      Action{"Action 5", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback, default_touch_callback},
    }
  };
}

void touch_interrupt(void* id_pointer) {
  int id = *((int *) id_pointer);
  was_touched[id] = true;
}

void draw_circles(int selected = 0, int amount=SWITCH_COUNT, uint16_t radius=8) {
  uint16_t y = SCREEN_HEIGHT/3 * 2;

  Serial.print("draw_circles (");
  Serial.print(selected);
  Serial.print(", ");
  Serial.print(amount);
  Serial.print(", ");
  Serial.print(radius);
  Serial.print(", ");
  Serial.print(y);
  Serial.println(")");

  for (int i=0; i < SWITCH_COUNT; i++) {
    uint16_t x = (radius*2.5 * i)+radius;
    //Serial.print("  # ");
    //Serial.print(i);
    //Serial.print(": ");
    //Serial.println(x);

    if (i == selected) {
      display.fillCircle(x, y, radius, WHITE);
    } else {
      display.drawCircle(x, y, radius, WHITE);
    }
  }
}

void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);

  for (int i=0; i < SWITCH_COUNT; i++) {
    display.clearDisplay();
    draw_circles(i);
    display.display();
    delay(400);
  }

  // put your main code here, to run repeatedly:
  Serial.println("loop.");

  // Poll switches & run callbacks
  update_switches();

  delay(100);
}


// Reads switch states & runs callbacks if theres a change
void update_switches() {
  Serial.println("Update switches");
  for (int i=0; i < SWITCH_COUNT; i++) {
    PanelSwitch target = switches[i];
    Page page = pages[current_page];
    Action action = page.actions[i];

    PanelSwitchState prev_state = switch_states[i];
    // Get current state
    PanelSwitchState new_state = poll_switch(target);

    maybe_run_callbacks(&action, prev_state, new_state);

    // Update state array with new data
    switch_states[i] = new_state;
    was_touched[i] = false;
  }
}

void maybe_run_callbacks(Action* action, PanelSwitchState prev_state, PanelSwitchState new_state) {
  if (prev_state.touch != new_state.touch) {
    Serial.println("Run touch callback for "+action->name);
    action->touch_callback(action, prev_state, new_state);
  }

  if (prev_state.up != new_state.up || prev_state.down != new_state.down) {
    Serial.println("Run switch callback for "+action->name);
    action->switch_callback(action, prev_state, new_state);
  }
}

bool is_touched(int pin) {
  return touchRead(pin) > TOUCH_THRESHOLD;
}

PanelSwitchState poll_switch(PanelSwitch target) {
  bool touched = was_touched[target.id];
  was_touched[target.id] = false;

  return PanelSwitchState{
    digitalRead(target.up) == HIGH,
    digitalRead(target.down) == HIGH,
    touched,
    target.id,
  };
}


void set_screen_text(String title, String description = "") {
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print(title);

  display.setTextSize(1);
  display.setCursor(2,0);
  display.print(description);

  display.display();
}

void print_switch_state(PanelSwitchState state) {
  Serial.print("ID: ");
  Serial.print(state.id);
  Serial.print(", UP: ");
  Serial.print(state.up);

  Serial.print(", DOWN: ");
  Serial.print(state.down);

  Serial.print(", TOUCH: ");
  Serial.print(state.touch);
}

void default_rotary_callback(Action* action, float change, bool is_pressed) {
  Serial.println("Default rotary callback: " + action->name +", change: "+change+", is_pressed: "+is_pressed);
}


void default_touch_callback(Action* action, PanelSwitchState prev_state, PanelSwitchState new_state) {
  if (new_state.touch) {
    display.clearDisplay();
    set_screen_text(action->name, action->switch_description);
    draw_circles(new_state.id);
  }
}

void default_switch_callback(Action* action, PanelSwitchState prev_state, PanelSwitchState new_state) {
  Serial.println("Default switch callback: "+action->name+" :)");
  print_switch_state(new_state);
  Serial.println("");
}
