// #include <wifi.h>
// #include <mqtt.h>
#include <Matter.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RotaryEncoderPCNT.h>

//// Stuff you may wanna change

// Set amount of switches in use
const int SWITCH_COUNT = 5;
// Amount of pages (action layers)
const int PAGE_COUNT = 1;
// Values higher than this are considered touched
const uint32_t TOUCH_THRESHOLD = 0;

// Rotary encoder pins
const int ROTARY_A = 38;
const int ROTARY_B = 39;
const int ROTARY_BTN = 40;

// OLED pins
const int SCREEN_SDA = 11;
const int SCREEN_SCL = 12;
// OLED i2c address
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
};

// current state of a switch, where true being pressed down
struct PanelSwitchState {
  bool up;
  bool down;
  bool touch;
};

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
  PanelSwitch{36,35, 1},
  PanelSwitch{34,33, 2},
  PanelSwitch{18,17, 4},
  PanelSwitch{16,15, 5},
  PanelSwitch{14,13, 6},
};


// State

// Filled in setup
Page pages[PAGE_COUNT] = {};

// Runtime
int current_page = 0;
volatile bool was_touched[SWITCH_COUNT] = {};
PanelSwitchState previous_switch_state[SWITCH_COUNT] = {};

// Components
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RotaryEncoderPCNT rotary(ROTARY_A, ROTARY_B);


void setup() {
  // Init serial, send some signs of life*0.8
  Serial.begin(9600);
  delay(100);
  Serial.println("Hardware-ding v0.0.0");
  Serial.print(SWITCH_COUNT);
  Serial.println(" switches, 1 rotary encoder, 1 display");

  pinMode(ROTARY_A, INPUT_PULLUP);
  pinMode(ROTARY_B, INPUT_PULLUP);
  pinMode(ROTARY_BTN, INPUT_PULLUP);

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
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialise pinmodes for each switch
  for (int i = 0; i < SWITCH_COUNT; i++) {
    PanelSwitch panel_switch = switches[i];
    was_touched[i] = false;

    Serial.print("Registering switch ");
    Serial.print(i);
    Serial.println(" with pins:");
    Serial.print("  up: ");
    Serial.println(panel_switch.up);
    Serial.print("  down: ");
    Serial.println(panel_switch.down);
    Serial.print("  touch: ");
    Serial.println(panel_switch.touch);

    pinMode(panel_switch.up, INPUT_PULLUP);
    pinMode(panel_switch.down, INPUT_PULLUP);
    touchAttachInterruptArg(panel_switch.touch, touch_interrupt, (void*)&was_touched[i], TOUCH_THRESHOLD);

    // Poll initial state
    previous_switch_state[i] = poll_switch(i);
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

void touch_interrupt(void* pointer) {
  *((volatile bool*) pointer) = true;
}

void draw_circles(int selected = -1, int amount=SWITCH_COUNT, uint16_t radius=8) {
  uint16_t y = SCREEN_HEIGHT/3 * 2;

  // Serial.print("draw_circles (");
  // Serial.print(selected);
  // Serial.print(", ");
  // Serial.print(amount);
  // Serial.print(", ");
  // Serial.print(radius);
  // Serial.print(", ");
  // Serial.print(y);
  // Serial.println(")");

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
  display.setCursor(0,0);
  display.print(rotary.position());
  //Serial.print(rotary.position());
  //Serial.print(" :: ");
  //Serial.println(digitalRead(ROTARY_BTN));
  // delay(50);

  // for (int i=0; i < SWITCH_COUNT; i++) {
  //   if (was_touched[i]) {
  //     Serial.print(i);
  //     Serial.println(" was touched!");
  //     was_touched[i] = false;
  //     //display.clearDisplay();
  //     draw_circles(i);
  //   }
  // }


  // for (int i=0; i < SWITCH_COUNT; i++) {
  //  display.clearDisplay();
  //  draw_circles(i);
  //  display.display();
  //  delay(50);
  // }

  // display.clearDisplay();
  // display.setCursor(0, 0);

  // put your main code here, to run repeatedly:
  // Serial.println("loop.");

  // Poll switches & run callbacks
  draw_circles();
  update_switches();

  display.display();

  delay(100);
}


// Reads switch states & runs callbacks if theres a change
void update_switches() {
  // Serial.println("Update switches");
  for (int i=0; i < SWITCH_COUNT; i++) {
    PanelSwitch panel_switch = switches[i];
    Page page = pages[current_page];
    Action action = page.actions[i];

    PanelSwitchState prev_state = previous_switch_state[i];
    // Get current state
    PanelSwitchState new_state = poll_switch(i);

    if (new_state.touch) {
      Serial.print("TOUCHED: ");
      Serial.println(i);
      draw_circles(i);
    }
    // Serial.print("CHECKING SWITCH ");
    // Serial.println(i);
    // print_switch_state(new_state);
    // Serial.println("");

    maybe_run_callbacks(&action, prev_state, new_state);

    // Update state array with new data
    previous_switch_state[i] = new_state;
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

PanelSwitchState poll_switch(int switch_index) {
  PanelSwitch panel_switch = switches[switch_index];

  bool touched = was_touched[switch_index];
  was_touched[switch_index] = false;

  return PanelSwitchState{
    digitalRead(panel_switch.up) == HIGH,
    digitalRead(panel_switch.down) == HIGH,
    touched,
  };
}

void set_screen_text(String title, String description = "") {
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print(title);

  display.setTextSize(1);
  display.setCursor(0,20);
  display.print(description);

  display.display();
}

void print_switch_state(PanelSwitchState state) {
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
    // display.clearDisplay();
    set_screen_text(action->name, action->switch_description);
    // display.display();
  }
}

void default_switch_callback(Action* action, PanelSwitchState prev_state, PanelSwitchState new_state) {
  Serial.println("Default switch callback: "+action->name+" :)");
  print_switch_state(new_state);
  Serial.println("");
}
