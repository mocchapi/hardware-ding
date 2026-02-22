#include <wifi.h>
#include <mqtt.h>

//// Stuff you may wanna change

// Set amount of switches in use
const int SWITCH_COUNT = 5;
// Amount of pages (action layers)
const int PAGE_COUNT = 2;
// Values higher than this are considered touched
const uint32_t TOUCH_THRESHOLD = 3000;



//// Structs

// PINs for a single switch
struct Switch {
  int up;
  int down;
  int touch;
};
// For detecting changes in switch state between polls
struct SwitchState {
  bool up;
  bool down;
  bool touch;
};

// PINs for a single rotary encoder
struct Rotary {
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
  void (*switch_callback)(Action* action, SwitchState prev_state, SwitchState new_state); // Called when the switch state changes (moved up/down/neutral or touched)
};

struct Page {
  String name;
  Action actions[SWITCH_COUNT] = {};
};


const Switch switches[SWITCH_COUNT] = {
  Switch{1, 2, 3}
};
SwitchState switch_states[SWITCH_COUNT] = {};

Rotary rotary;
Page pages[PAGE_COUNT] = {};

int current_page = 0;


void setup() {
  Serial.begin(9600);
  Serial.println("Hardware-ding v0.0.0");
  Serial.println(SWITCH_COUNT + " switches, 1 rotary encoder, 1 display");

  for (int i = 0; i < SWITCH_COUNT; i++) {
    // Initialise pinmodes for each switch
    Switch target = switches[i];
    pinMode(target.up, INPUT_PULLUP);
    pinMode(target.down, INPUT_PULLUP);

    // Poll initial state
    switch_states[i] = poll_switch(target);
  }

  // Define pages & actions
  pages[0] = Page{
    "First page",
    {
      Action{"Action 1", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback},
      Action{"Action 2", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback},
      Action{"Action 3", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback},
      Action{"Action 4", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback},
      Action{"Action 5", "Dummy action", "Dummy rotary action", true, default_rotary_callback, default_switch_callback},
    }
  };
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("loop.");

  // Poll switches & run callbacks
  update_switches();

  delay(0.1);
}

bool is_touched(int pin) {
  return touchRead(pin) > TOUCH_THRESHOLD;
}

// Reads switch states & runs callbacks if theres a change
void update_switches() {
  for (int i=0; i < SWITCH_COUNT; i++) {
    Switch target = switches[i];
    SwitchState switch_state = switch_states[i];
    Page page = pages[current_page];
    Action action = page.actions[i];
    
    SwitchState prev_state = switch_states[i];
    // Get current state
    SwitchState new_state = poll_switch(target);

    maybe_run_callbacks(&action, prev_state, new_state);

    // Update state array with new data
    switch_states[i] = new_state;
  }
}

void maybe_run_callbacks(Action* action, SwitchState prev_state, SwitchState new_state) {
  if (prev_state.up != new_state.up || prev_state.down != new_state.down || prev_state.touch != new_state.touch) {
    action->switch_callback(action, prev_state, new_state);
  }
}

SwitchState poll_switch(Switch target) {
  return SwitchState{
    digitalRead(target.up) == HIGH,
    digitalRead(target.down) == HIGH,
    is_touched(target.touch),
  };
}


void default_rotary_callback(Action* action, float change, bool is_pressed) {
  Serial.println("Default rotary callback: " + action->name +", change: "+change+", is_pressed: "+is_pressed);
}

void default_switch_callback(Action* action, SwitchState prev_state, SwitchState new_state) {
  Serial.println("Default switch callback: "+action->name+" :)");
}
