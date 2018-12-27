/*
 *  Project     Media Footswitch Controller
 *  @author     David Madison
 *  @link       partsnotincluded.com/electronics/diy-media-key-footswitch-for-pc
 *  @license    MIT - Copyright (c) 2018 David Madison
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

enum CommandType {
	MODE_MEDIA,
	MODE_KEYBOARD,
};

// Pin Definitions
const uint8_t Pin_Button1 = 2;  // Input, internal pull-up
const uint8_t Pin_LED1 = 3;     // Output

const uint8_t Pin_Button2 = 7;  // Input, internal pull-up
const uint8_t Pin_LED2 = 9;     // Output

// Size Limits
const uint8_t NumButtons = 2;   // 2 physical buttons
const uint8_t NumCommands = 3;  // Max of 3 commands (press 3 times)

// Config Mode
const uint8_t EEPROM_Addr = 8;  // Address for the EEPROM enumeration storage
const CommandType DefaultMode = MODE_MEDIA;  // Initial mode if bad EEPROM read

// Timing Constants, ms (adjust to liking)
const unsigned long DebounceTime = 30;  // Maximum switching speed for inputs
const unsigned long RepeatTimeout = 375;  // Max time between presses for counter
const unsigned long HoldTime = 325;  // Minimum time before a button is considered "held" down

// User Options
const uint8_t MaxBrightness = 255;  // LED brightness, 0-255
const unsigned long ConfigTime = 2000;  // Max time for both buttons held down before mode change, ms

// ----------------------------------------------------------------------------

// Libraries
#include <HID-Project.h>  // NicoHood HID Library
#include <EEPROM.h>

// Local Headers
#include "src/MediaFootswitch_IO.h"
#include "src/MediaFootswitch_HID.h"


Buttons buttons[NumButtons]{
	Buttons(Pin_Button1, Pin_LED1),
	Buttons(Pin_Button2, Pin_LED2),
};

ConsumerKeycode media_commands[NumButtons][NumCommands] = {
	{ MEDIA_PLAY_PAUSE, MEDIA_VOLUME_MUTE, MEDIA_VOLUME_DOWN },
	{ MEDIA_NEXT, MEDIA_PREVIOUS, MEDIA_VOLUME_UP },
};

KeyboardKeycode keyboard_commands[NumButtons] = {
	KEY_F21,
	KEY_F22,
};

void blinkAll(uint16_t ntimes, unsigned long period) {
	for (uint16_t i = 0; i < ntimes; i++) {
		for (int8_t state = 0; state <= 1; state++) {
			for (size_t i = 0; i < NumButtons; i++) {
				buttons[i].led.set(state);
			}
			delay(period / 2);
		}
	}
}

void setup() {
	Serial.begin(115200);
	for (size_t i = 0; i < NumButtons; i++) {
		buttons[i].begin();
		buttons[i].media.setCommands(media_commands[i]);
		buttons[i].keyboard.setCommand(keyboard_commands[i]);
		buttons[i].led.setBrightness(MaxBrightness);
		buttons[i].checkInput();
	}

	CommandType mode;
	EEPROM.get(EEPROM_Addr, mode);
	if (!validConfig(mode)) {
		EEPROM.put(EEPROM_Addr, DefaultMode);
		mode = DefaultMode;
	}
	Buttons::setMode(mode);

	boolean buttonsHigh = true;
	do {
		boolean configReady = true;
		for (size_t i = 0; i < NumButtons; i++) {
			buttons[i].checkInput();  // Read pin states
			buttonsHigh &= buttons[i].pin.state();  // Check if any pin goes low
			configReady &= buttons[i].pin.heldFor() >= ConfigTime;  // Check if all pins are high for config time
		}

		if (buttonsHigh && configReady) {
			// Buttons held for X amount of time, time to change the config
			blinkAll(10, 250);  // Flash LEDs at the user to tell them we're changing modes
			Buttons::switchMode();
			EEPROM.put(EEPROM_Addr, Buttons::getMode());
			break;
		}
	}
	while (buttonsHigh == true);  // Stop looping if any button is unpressed

	Buttons::begin_usb();
}

void loop() {
	for (size_t i = 0; i < NumButtons; i++) {
		buttons[i].checkInput();
		buttons[i].runCommands();
	}
}
