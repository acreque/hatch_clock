# Functional Requirements
- The default display shows the current time in HH:MM format.
  - A single dot in the upper right corner of the display is used to show whether there is an alarm set.
- Temporarily show the alarm time in response to a button being pressed on the clock.
  - If no alarm is set, the display should show: --:--
- Temporarily show a single digit number in response to a button being pressed on the
# Do
- Write the code in C.
- Assume that a hardware-level driver for the dot matrix display already exists.
  - Use mock functions for any hardware or driver functionality you want to represent.
- Assume that the display module receives its direction from a higher level state manager.
  - Use mock functions for any RTOS-based functionality you want to represent.
- Use your judgement if anything is unclear. Explain your reasoning.
# Deliverables
- Working C code (doesn’t need to compile/run perfectly – we want to see your approach).
- Brief comments or notes explaining
- At least one unit test demonstrating your testing philosophy.

# Solution Overview

I decided to take the assignment all the way to something that runs on real hardware since I had an ESP32 board with buttons that I use for prototyping code. Below are the assumptions I made and a description of the run-time behavior of the code. I chose this approach because it showed how I go about testing. I always lean toward hardware-in-the-loop testing and typically use either a UART to prove out my code or a few GPIO bits and an oscilloscope.

Based on the following assumptions, I created an ESP32 program using vscode and the ESP-IDF extension. The program uses UART2 as the serial interface to the hardware driver and GPIO16 for the LOAD signal to latch data into the display dot array. The program is a functional clock that displays the hours and minutes in HH:MM format. The clock supports an alarm and the dot in the upper right corner is used to indicate the state of the alarm. 

The program powers up and sets the clock time to 01:23, the alarm time to 04:56, and the alarm state to off. The hardware driver is loaded with new dot data 2 times per second. Two buttons are used to control the behavior of the clock as follows:

## Button 1 (BTN1) is connected to ESP32 GPIO34

When BTN1 is pressed the alarm time is shown for 5 seconds. After 5 seconds the normal clock time display resumes. If the alarm is off, the display shows \--:\-- for 5 seconds. 

## Button 2 (BTN2) is connected to ESP32 GPIO35

Each time BTN2 is pressed, the state of the alarm toggles. Each BTN2 press also displays the value of 1 in the left most digit and the other digits display nothing. The state of the alarm dot always reflects the toggled state. The display remains in this state for 5 seconds and then resumes normal clock operation.

# Solution Architecture

The program is broken into two tasks; ```clock_task``` and ```display_task```. ```clock_task``` is the higher level clock state manager that processes button presses and keeps track of time. ```display_task``` is responsible for rendering the desired content on the dot matrix. The ```display_task``` has 3 different render modes. One mode is the normal clock mode. The other two modes are temporary modes that only last for 5 seconds to provide the button press functionality. 

The most critical data structure is the ```g_clock_state```. It stores the clock and alarm times as well as the alarm state (on/off), render mode, and render mode timer. Because the ```clock_task``` writes the ```g_clock_state``` and the ```display_task``` reads it, there is an inherent race condition. Reads and writes must have exclusive access to ```g_clock_state``` when they need it. To manage this the ``g_state_mutex``` mutex is used to protect the state structure.

The ```clock_task``` ***takes*** the mutex each time a button press occurs, the clock value changes, or the render mode timer expires. The ```clock_task``` mutex ***give*** logic condition-less so the ```display_task``` cannot be starved. The ```display_task``` ***takes*** the mutex 2x per second so it can get a clean snapshot of ```g_clock_state``` before it sends new render data the hardware driver. The ```clock_task``` cannot be staved because the ```display_task``` ***give*** logic is condition-less as well. 

# Assumptions

1.  A dot matrix display comprised of 32 columns of dots (i.e., LEDs), where each column is 8 dots tall, is used as the clock display device (i.e., an LED array that is 32x8).

2.  Columns are numbered 0 through 31 from left to right. Row are numbered 0 through 7 with row zero at the top.

3.  Four digits are needed for the clock. This equates to an 8x8 sub-array per digit.

4.  Because we will want the ability to annotate each digit using dots/symbols above or on the right side of each digit, the top row of dots and the 3 columns of dots on the right are reserved for this. This leaves a 5x7 array for each digit/glyph.

5.  The colon separator uses the middle column of the 3 annotation columns on the right side of each digit.

6.  The hardware driver for the dot matrix supports the ability to update each of the 32 columns individually.

7.  The hardware driver buffers all the column data so that all 32 columns can be loaded into the buffer without changing the illumination state of the dots.

8.  The buffer is loaded using a serial interface where the column number and dot states are specified in each message.

9.  The hardware driver has a LOAD bit. Once the buffer is fully loaded with data for all 32 columns its contents can applied to the entire dot matrix by pulsing the LOAD bit.





