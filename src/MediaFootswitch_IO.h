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

#ifndef MEDIAFOOTSWITCH_IO_H
#define MEDIAFOOTSWITCH_IO_H

// --------------------------------------------------------
// Debouncer                                              |
//     Debounces an boolean input for x milliseconds      |
// --------------------------------------------------------

class Debouncer {
public:
	Debouncer(unsigned long t, boolean sState) :
		BounceTime(t)
	{
		lastChange = 0 - t;
		state = sState;
	}

	boolean bouncing() const {
		return millis() - lastChange <= BounceTime;
	}

	void debounce(boolean s) {
		if (s == state || bouncing()) return;
		state = s;
		lastChange = millis();
	}

	boolean getState() const {
		return state;
	}

	unsigned long heldFor() const {
		return millis() - lastChange;
	}

protected:
	const unsigned long BounceTime;  // Debounce time in us
	unsigned long lastChange;
	boolean state;
};

// --------------------------------------------------------
// InputHandler                                           |
//     Reads a pin input, debounces it, and keeps track   |
//     of current and previous states.                    |
// --------------------------------------------------------

class InputHandler {
public:
	InputHandler(uint8_t p, boolean up = true) :
		Pin(p), pullup(up), bounce(DebounceTime, HIGH) {}

	void begin() {
		pinMode(Pin, pullup ? INPUT_PULLUP : INPUT);
	}

	void update() {
		lastState = state();
		bounce.debounce(digitalRead(Pin));
	}

	boolean state() const {
		return bounce.getState() != pullup;
	}

	boolean changed() const {
		return state() != lastState;
	}

	boolean rising() const {
		return changed() && state() == HIGH;
	}

	boolean falling() const {
		return changed() && state() == LOW;
	}

	unsigned long heldFor() const {
		return bounce.heldFor();
	}

protected:
	const uint8_t Pin;
	const boolean pullup;
	Debouncer bounce;
	boolean lastState = 0;
};

// --------------------------------------------------------
// PressCounter                                           |
//     For a given pin input, keeps track of how many     |
//     times the pin is toggled with a timeout of x       |
// --------------------------------------------------------

class PressCounter {
public:
	PressCounter(InputHandler &p, unsigned long speed) :
		Pin(p), PressSpeed(speed) {}

	void check() {
		if (Pin.rising()) {
			if (finished == true) { count = 0; }
			finished = false;
			count++;
		}

		if (count > 0) {
			if (Pin.state() == HIGH) {
				lastUpdate = millis();
			}
			else if (millis() - lastUpdate > PressSpeed) {
				finished = true;
			}
		}
	}

	uint8_t getCount() const {
		return count;
	}

	boolean isDone() const {
		return finished;
	}

	void reset() {
		count = 0;
		lastUpdate = millis() - PressSpeed;
	}

	InputHandler & Pin;

protected:
	const unsigned long PressSpeed;

	boolean finished = false;
	unsigned long lastUpdate;
	uint8_t count = 0;
};

// --------------------------------------------------------
// CommandIndex                                             |
//     For a given pin input, reports an index number for |
//     the corresponding # of presses or held input       |
// --------------------------------------------------------

class CommandIndex {
public:
	CommandIndex(InputHandler & p, uint8_t rmax, unsigned long rspeed, unsigned long t_hold) :
		MaxCount(rmax), HoldTime(t_hold), pin(p), counter(pin, rspeed) {}

	void update() {
		counter.check();
		setIndex();
	}

	uint8_t getIndex() {
		return index;
	}

	const uint8_t MaxCount;
	const unsigned long HoldTime;

	InputHandler & pin;
	PressCounter counter;

protected:
	void setIndex() {
		if (counter.isDone()) {
			index = counter.getCount();
			if (index > MaxCount) {
				index = MaxCount;
			}
			counter.reset();
		}
		else if (pin.state() == HIGH && pin.heldFor() >= HoldTime) {
			index = MaxCount;
			counter.reset();
		}
		else {
			index = 0;
		}
	}

	uint8_t index = 0;
};

// --------------------------------------------------------
// LEDHandler                                             |
//     For a given pin controlling an LED, switch it on   |
//     or off according to a set brightness value         |
// --------------------------------------------------------

class LEDHandler {
public:
	LEDHandler(uint8_t p, boolean inv = false) :
		pin(p), activeLow(inv) 
	{
		state = 0;  // starts as low
		brightness = 255;  // max brightness
	}

	void begin() {
		pinMode(pin, OUTPUT);
		off();
	}

	void on() {
		set(1);
	}

	void off() {
		set(0);
	}

	void set(boolean s) {
		if (s == state) return;  // no change
		state = s;
		analogWrite(pin, activeLow ? 255 - (brightness * s) : brightness * s);
	}

	void setBrightness(uint8_t b) {
		brightness = b;
	}

protected:
	const uint8_t pin;
	const boolean activeLow;
	boolean state;
	uint8_t brightness;
};

#endif
