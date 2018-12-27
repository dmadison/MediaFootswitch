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

#ifndef MEDIAFOOTSWITCH_HID_H
#define MEDIAFOOTSWITCH_HID_H

 // --------------------------------------------------------
 // MediaCommands                                          |
 //     Sends media commands after # of button presses     |
 // --------------------------------------------------------

template <uint8_t max_ncmds>
class MediaCommands {
public:
	static void begin() {
		Consumer.begin();
	}

	static void releaseAll() {
		Consumer.releaseAll();
	}

	MediaCommands(InputHandler &p) :
		pin(p),
		index(pin, NumCommands, RepeatTimeout, HoldTime) {}

	void run() {
		index.update();
		uint8_t command = index.getIndex();

		if (command > max_ncmds) { command = 0; }

		if (command != lastCommand) {
			if (lastCommand != 0) {
				Consumer.release(commands[lastCommand - 1]);
			}
			if (command != 0) {
				Consumer.press(commands[command - 1]);
			}
		}

		lastCommand = command;
	}

	template<uint8_t ncmds>
	void setCommands(const ConsumerKeycode(&cmds)[ncmds]) {
		if (ncmds < max_ncmds) return;

		for (size_t i = 0; i < max_ncmds; i++) {
			commands[i] = cmds[i];
		}
	}

protected:
	InputHandler & pin;
	CommandIndex index;

	uint8_t lastCommand = 0;
	ConsumerKeycode commands[max_ncmds] = { MEDIA_PLAY_PAUSE };
};


// --------------------------------------------------------
// KeyboardCommand                                        |
//     Sends keyboard command link to pin state           |
// --------------------------------------------------------

class KeyboardCommand {
public:
	static void begin() {
		Keyboard.begin();
	}

	static void releaseAll() {
		Keyboard.releaseAll();
	}

	KeyboardCommand(InputHandler & p) : Pin(p) {}

	void run() {
		if (Pin.rising()) {
			Keyboard.press(command);
		}
		else if (Pin.falling()) {
			Keyboard.release(command);
		}
	}

	void setCommand(const KeyboardKeycode cmd) {
		command = cmd;
	}

protected:
	KeyboardKeycode command;
	InputHandler & Pin;
};

// --------------------------------------------------------
// Buttons                                                |
//     Holds input pin, LED, and USB command classes      |
//     linked to a specific button input                  |
// --------------------------------------------------------

class Buttons {
public:
	static void begin_usb() {
		begun = true;

		switch (mode) {
		case(MODE_MEDIA):
			MediaCommands<NumCommands>::begin();
			break;
		case(MODE_KEYBOARD):
			KeyboardCommand::begin();
			break;
		}
	}

	static void setMode(CommandType t) {
		if (t == mode) return;
		mode = t;

		if (begun) {
			switch (mode) {
			case(MODE_MEDIA):
				MediaCommands<NumCommands>::releaseAll();
				break;
			case(MODE_KEYBOARD):
				KeyboardCommand::releaseAll();
				break;
			}
		}
	}

	static void switchMode() {
		if (mode == MODE_MEDIA) {
			setMode(MODE_KEYBOARD);
		}
		else {
			setMode(MODE_MEDIA);
		}
	}

	static CommandType getMode() {
		return mode;
	}

	Buttons(uint8_t pin_button, uint8_t pin_led) :
		pin(pin_button),
		led(pin_led),
		media(pin),
		keyboard(pin)
	{}

	void begin() {
		pin.begin();  // Set button pin input mode
		led.begin();  // Set LED pin output mode
	}

	void checkInput() {
		pin.update();  // Save current pin register state, debounced
		led.set(pin.state());  // Set LED based off of input pin
	}

	void runCommands() {
		switch (mode) {
		case(MODE_MEDIA):
			media.run();
			break;
		case(MODE_KEYBOARD):
			keyboard.run();
			break;
		}
	}

	InputHandler pin;
	LEDHandler led;
	MediaCommands<NumCommands> media;
	KeyboardCommand keyboard;

protected:
	static boolean begun;
	static CommandType mode;
};

boolean Buttons::begun = false;
CommandType Buttons::mode = DefaultMode;

boolean validConfig(CommandType mode) {
	return mode == MODE_MEDIA || mode == MODE_KEYBOARD;
}

#endif