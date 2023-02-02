#include <LiquidCrystal.h>
#include <Keypad.h>
#include <RTClib.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include "NewPing.h"
#include <Servo.h>

// PUSH BUTTON DEFINITIONS
#define START 43
#define CONTINUE 45

// SERVO DEFINITIONS
Servo servo;
#define SERVO_PIN 8

// ULTRASONIC SENSOR DEFINITIONS
#define TRIGGER_PIN 47
#define ECHO_PIN 49
#define MAX_DISTANCE 2000 // Maximum distance we want to ping for (in centimeters)

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
int iterations = 20;

// SD CARD DEFINITIONS
#define PIN_SPI_CS 53
File myFile;
char filename[] = "yyyymmdd.txt"; // filename (without extension) should not exceed 8 chars

// RTC DEFINITIONS
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"};
uint8_t DAILY_EVENT_START_HH = 00; // event start time: hour
uint8_t DAILY_EVENT_START_MM = 00; // event start time: minute

uint8_t DAILY_EVENT_RECORD_HH = 17; // event start time: hour
uint8_t DAILY_EVENT_RECORD_MM = 00; // event start time: minute

// SIM800L DEFINITIONS
SoftwareSerial sim(19, 18); // SIM800L TX & RX is connected to Arduino #19 & #18
String number = "0705002513";

// LCD PIN DEFINITIONS
const int rs = A0, en = A1, d4 = A2, d5 = A3, d6 = A4, d7 = A5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// KEYPAD DEFINITIONS
const int ROW_NUM = 4;
const int COLUMN_NUM = 4;
char keys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

byte pin_rows[ROW_NUM] = {32, 34, 36, 38};      // connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {40, 42, 44, 46}; // connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// FILLING VARIABLES
int FlowSensorPin = 9;
bool state;

// IR PINS DEFINITIONS
const int IR_FILL = 4;
const int IR_CAPHOLDER = 5;
const int IR_CAP = 6;
const int IR_COUNT = 7;

// START OR INPUT VARIABLES
int cursorColumn, i = 0;
char botArray[10] = {};
char capArray[10] = {};
char inputDay[2] = {};
char inputMonth[2] = {};
char inputYear[4] = {};
char inputHour[2] = {};
char inputMinute[2] = {};

int bottles = 0;
int capacity = 0;

int botTrack = 0;
int capTrack = 0;

int count = 0;
int countTrack = 0;

int index1 = -1;
int index2 = -1;

int lastState1 = HIGH; // the previous state from the input pin
int currentState1;     // the current reading from the input pin

int lastState2 = HIGH; // the previous state from the input pin
int currentState2;     // the current reading from the input pin

int lastState3 = HIGH; // the previous state from the input pin
int currentState3;     // the current reading from the input pin

int lastState4 = HIGH; // the previous state from the input pin
int currentState4;     // the current reading from the input pin

// ACTUATOR PINS DEFINITIONS
const int capMotorRelay = 22;
const int TimerRelay = 24;
const int conveyorRelay = 26;
const int PumpRelay = 25;
const int BuzzerRelay = 37;

const int ledPinBlue = 27;
const int ledPinGreen = 29;
const int ledPinYellow = 31;
const int ledPinRed = 33;

void setup()
{
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("****WELCOME!****");
    delay(200);
    lcd.setCursor(0, 1);
    lcd.print("----------------");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("--SETTING UP..--");
    delay(200);
    lcd.setCursor(0, 1);
    lcd.print("--PLEASE WAIT---");
    delay(2000);

    servo.attach(SERVO_PIN);

    pinMode(FlowSensorPin, INPUT);
    pinMode(PumpRelay, OUTPUT);
    pinMode(conveyorRelay, OUTPUT);
    pinMode(BuzzerRelay, OUTPUT);
    pinMode(TimerRelay, OUTPUT);
    pinMode(capMotorRelay, OUTPUT);
    pinMode(IR_FILL, INPUT);
    pinMode(IR_CAP, INPUT);
    pinMode(IR_CAPHOLDER, INPUT);
    pinMode(IR_COUNT, INPUT);
    pinMode(START, INPUT);
    pinMode(CONTINUE, INPUT);
    pinMode(ledPinBlue, OUTPUT);
    pinMode(ledPinGreen, OUTPUT);
    pinMode(ledPinRed, OUTPUT);
    pinMode(ledPinYellow, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(2), emergencyStop, RISING); //  function for creating external interrupts at pin #2 on Rising (LOW to HIGH)

    digitalWrite(PumpRelay, LOW);
    digitalWrite(TimerRelay, LOW);
    digitalWrite(conveyorRelay, LOW);
    digitalWrite(capMotorRelay, LOW);
    digitalWrite(BuzzerRelay, LOW);
    digitalWrite(ledPinBlue, LOW);
    digitalWrite(ledPinGreen, LOW);
    digitalWrite(ledPinRed, LOW);
    digitalWrite(ledPinYellow, LOW);

    // SETUP RTC MODULE
    if (!rtc.begin())
    {
        lcd.clear();
        lcd.print("RTC FAILED");
        digitalWrite(ledPinYellow, HIGH);
        while (1)
            ; // don't do anything more
    }
    // sets the RTC to the date & time on PC this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // rtc.adjust(DateTime(2021, 1, 21, 3, 0, 0));

    // SETUP SD CARD MODULE
    if (!SD.begin(PIN_SPI_CS))
    {
        lcd.clear();
        lcd.print("SD CARD FAILED!");
        digitalWrite(ledPinYellow, HIGH);
        while (1)
            ; // don't do anything more:
    }

    // SIM800L SETUP MODULE
    delay(5000); // delay for 5 seconds to make sure the modules get the signal
    Serial.begin(9600);
    sim.begin(9600);
    delay(1000);
    sim.println("AT"); // Once the handshake test is successful, it will reply OK
    delay(500);
    sim.println("AT+CSQ"); // Signal Quality Test, value range is 0-31, 31 is the best
    delay(500);
    sim.println("AT+CCID"); // Read SIM information to confirm whether the SIM is plugged
    delay(500);
    sim.println("AT+CREG?"); // Check whether it has registered in the network
    delay(500);
    sim.println("AT+CNMI=2,2,0,0,0"); // AT Command to receive a live SMS
    delay(500);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SETTING UP DONE!");
    delay(200);
    lcd.setCursor(0, 1);
    lcd.print("READY TO BEGIN!");
    delay(3000);
}

void loop()
{
    // DEFAULT STATE
    while (1)
    {
        char key = keypad.getKey();
        countTrack = 0;
        defaultDisplay();
        digitalWrite(ledPinBlue, HIGH);
        if (key == keys[2][3]) // 'C'
        {
            displayTodaysLogs(filename);
            defaultDisplay();
            // setDateTime(inputDay, inputMonth, inputYear, inputHour, inputMinute, cursorColumn, i)
        }
        else if (key == keys[3][0]) // '*'
        {
            displayDate();
            defaultDisplay();
        }
        else if (key == keys[3][2]) // '#'
        {
            displayTrackedLogs(botTrack, capTrack);
            defaultDisplay();
        }
        else if (digitalRead(START) == HIGH)
        {
            debounceStart();
            digitalWrite(ledPinBlue, LOW);
            digitalWrite(ledPinGreen, HIGH);
            break;
        }
        else
        {
            updateName(filename);
            resetVariables(botTrack, capTrack);
            sendTodaysLogsScheduled(number, filename, botTrack, capTrack);
            sendLogsRequest(number, botTrack, capTrack, index1, index2);
        }
    }

    // OPERATION STATE
    inputFunction(botArray, capArray, cursorColumn, i);
    convertVariables(botArray, capArray);

    while (1)
    {
    OperationLoop:
        digitalWrite(conveyorRelay, HIGH);
        if (digitalRead(IR_FILL) == LOW)
            goto fill;
        else if (digitalRead(IR_CAP) == LOW)
            goto cap;
        else if (digitalRead(IR_COUNT) == LOW)
            goto cnt;
        else
            goto OperationLoop;

    fill:
        digitalWrite(conveyorRelay, LOW);
        digitalWrite(BuzzerRelay, HIGH);
        delay(500);
        digitalWrite(BuzzerRelay, LOW);
        lcd.clear();
        lcd.print("BOTTLE @ FILLING");
        while (1)
        {
            currentState1 = digitalRead(IR_FILL);

            if (lastState1 == HIGH && currentState1 == LOW)
            {
                filling(capacity, bottles);
                break;
            }

            else if (lastState1 == LOW && currentState1 == HIGH)
                digitalWrite(PumpRelay, LOW);
            delay(50);
            //  save the the last state
            lastState1 = currentState1;
        }
        digitalWrite(conveyorRelay, HIGH);
        goto OperationLoop;

    cap:
        digitalWrite(conveyorRelay, LOW);
        digitalWrite(BuzzerRelay, HIGH);
        delay(500);
        digitalWrite(BuzzerRelay, LOW);
        lcd.clear();
        lcd.print("BOTTLE @ CAPPING");
        while (1)
        {
            currentState2 = digitalRead(IR_CAP);

            if (lastState2 == HIGH && currentState2 == LOW)
            {
                capping();
                break;
            }

            else if (lastState2 == LOW && currentState2 == HIGH)
                digitalWrite(capMotorRelay, LOW);
            delay(50);
            //  save the the last state
            lastState2 = currentState2;
        }

        digitalWrite(conveyorRelay, LOW);
        goto OperationLoop;
    cnt:
        countTrack++;
        while (1)
        {
            currentState3 = digitalRead(IR_COUNT);

            if (lastState3 == HIGH && currentState3 == LOW)
            {
                counter(cursorColumn, i, capacity, bottles, botTrack, capTrack, count, botArray, capArray);
                break;
            }

            else if (lastState3 == LOW && currentState3 == HIGH)
                delay(50);
            //  save the the last state
            lastState3 = currentState3;
        }
        if (countTrack == bottles)
            break;
    }
}

char *inputFunction(char botArray[], char capArray[], int cursorColumn, int i)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NO OF BOTTLES:");
    while (i >= 0 && i < 10)
    {
    back:
        char key = keypad.getKey();
        if (key)
        {
            lcd.setCursor(cursorColumn, 1); // move cursor to cursorColumn,1
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                lcd.print(key);
            }
            if (key == keys[0][3])
            {
                while (i != 0)
                {
                    botArray[i--] = {};
                }
                for (int j = 0; j < 16; j++)
                {
                    lcd.setCursor(j, 1);
                    lcd.print(" ");
                }
                cursorColumn = 0;
                goto back;
            }
            else if (key == keys[3][3])
                goto proceed;
            else if (key == keys[1][3])
            {
                if (cursorColumn != 0)
                {
                    cursorColumn--;
                    botArray[i--] = {};
                    lcd.setCursor(cursorColumn, 1);
                    lcd.print(" ");
                    goto back;
                }
                else
                    goto back;
            }
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                botArray[i] = key;
            }
            cursorColumn++; // next position
            i++;
        }
        else
            goto back;
    }

proceed:
    lcd.clear();
    lcd.print("INPUT STORED!");
    delay(500);
    cursorColumn = 0;
    i = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CAPACITY IN ML:");
    while (i >= 0 && i < 10)
    {
    back1:
        char key = keypad.getKey();
        if (key)
        {
            lcd.setCursor(cursorColumn, 1); // move cursor to cursorColumn,1
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                lcd.print(key);
            }
            if (key == keys[0][3])
            {
                while (i != 0)
                {
                    capArray[i--] = {};
                }
                for (int j = 0; j < 16; j++)
                {
                    lcd.setCursor(j, 1);
                    lcd.print(" ");
                }
                cursorColumn = 0;
                goto back1;
            }
            else if (key == keys[3][3])
                goto proceed1;
            else if (key == keys[1][3])
            {
                if (cursorColumn != 0)
                {
                    cursorColumn--;
                    capArray[i--] = {};
                    lcd.setCursor(cursorColumn, 1);
                    lcd.print(" ");
                    goto back1;
                }
                else
                    goto back1;
            }
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                capArray[i] = key;
            }
            cursorColumn++; // next position
            i++;
        }
        else
            goto back1;
    }
proceed1:
    cursorColumn = 0;
    i = 0;
    lcd.clear();
    lcd.print("INPUT STORED!");
    delay(500);
    lcd.clear();
    lcd.print("PRESS ENTER");
    while (1)
    {
        char key = keypad.getKey();
        if (key)
        {
            if (key == keys[3][3])
                goto proceed2;
        }
    }
proceed2:
    return botArray, capArray;
}

char *setDateTime(char inputDay[], char inputMonth[], char inputYear[], char inputHour[], char inputMinute[], int cursorColumn, int i)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("-DATE SETTING-");
    delay(500);
    lcd.serCursor(0, 1);
    lcd.print("---------------");
    delay(500);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTER DAY:");
    while (i >= 0 && i < 2)
    {
    back:
        char key = keypad.getKey();
        if (key)
        {
            lcd.setCursor(cursorColumn, 1); // move cursor to cursorColumn,1
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                lcd.print(key);
            }
            if (key == keys[0][3])
            {
                while (i != 0)
                {
                    inputDay[i--] = {};
                }
                for (int j = 0; j < 16; j++)
                {
                    lcd.setCursor(j, 1);
                    lcd.print(" ");
                }
                cursorColumn = 0;
                goto back;
            }
            else if (key == keys[3][3])
                goto proceed;
            else if (key == keys[1][3])
            {
                if (cursorColumn != 0)
                {
                    cursorColumn--;
                    inputDay[i--] = {};
                    lcd.setCursor(cursorColumn, 1);
                    lcd.print(" ");
                    goto back;
                }
                else
                    goto back;
            }
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                inputDay[i] = key;
            }
            cursorColumn++; // next position
            i++;
        }
        else
            goto back;
    }

proceed:
    lcd.clear();
    lcd.print("DAY STORED!");
    delay(500);
    cursorColumn = 0;
    i = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTER THE MONTH:");
    while (i >= 0 && i < 2)
    {
    back1:
        char key = keypad.getKey();
        if (key)
        {
            lcd.setCursor(cursorColumn, 1); // move cursor to cursorColumn,1
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                lcd.print(key);
            }
            if (key == keys[0][3])
            {
                while (i != 0)
                {
                    inputMonth[i--] = {};
                }
                for (int j = 0; j < 16; j++)
                {
                    lcd.setCursor(j, 1);
                    lcd.print(" ");
                }
                cursorColumn = 0;
                goto back1;
            }
            else if (key == keys[3][3])
                goto proceed1;
            else if (key == keys[1][3])
            {
                if (cursorColumn != 0)
                {
                    cursorColumn--;
                    inputMonth[i--] = {};
                    lcd.setCursor(cursorColumn, 1);
                    lcd.print(" ");
                    goto back1;
                }
                else
                    goto back1;
            }
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                inputMonth[i] = key;
            }
            cursorColumn++; // next position
            i++;
        }
        else
            goto back1;
    }
proceed1:
    cursorColumn = 0;
    i = 0;
    lcd.clear();
    lcd.print("MONTH STORED!");
    delay(500);
    cursorColumn = 0;
    i = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTER THE YEAR:");
    while (i >= 0 && i < 4)
    {
    back2:
        char key = keypad.getKey();
        if (key)
        {
            lcd.setCursor(cursorColumn, 1); // move cursor to cursorColumn,1
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                lcd.print(key);
            }
            if (key == keys[0][3])
            {
                while (i != 0)
                {
                    inputYear[i--] = {};
                }
                for (int j = 0; j < 16; j++)
                {
                    lcd.setCursor(j, 1);
                    lcd.print(" ");
                }
                cursorColumn = 0;
                goto back2;
            }
            else if (key == keys[3][3])
                goto proceed2;
            else if (key == keys[1][3])
            {
                if (cursorColumn != 0)
                {
                    cursorColumn--;
                    inputYear[i--] = {};
                    lcd.setCursor(cursorColumn, 1);
                    lcd.print(" ");
                    goto back2;
                }
                else
                    goto back2;
            }
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                inputYear[i] = key;
            }
            cursorColumn++; // next position
            i++;
        }
        else
            goto back2;
    }
proceed2:
    cursorColumn = 0;
    i = 0;
    lcd.clear();
    lcd.print("YEAR STORED!");
    delay(500);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("-TIME SETTING-");
    delay(500);
    lcd.serCursor(0, 1);
    lcd.print("---------------");
    delay(500);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTER THE HOUR:");
    while (i >= 0 && i < 2)
    {
    back3:
        char key = keypad.getKey();
        if (key)
        {
            lcd.setCursor(cursorColumn, 1); // move cursor to cursorColumn,1
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                lcd.print(key);
            }
            if (key == keys[0][3])
            {
                while (i != 0)
                {
                    inputHour[i--] = {};
                }
                for (int j = 0; j < 16; j++)
                {
                    lcd.setCursor(j, 1);
                    lcd.print(" ");
                }
                cursorColumn = 0;
                goto back3;
            }
            else if (key == keys[3][3])
                goto proceed3;
            else if (key == keys[1][3])
            {
                if (cursorColumn != 0)
                {
                    cursorColumn--;
                    inputHour[i--] = {};
                    lcd.setCursor(cursorColumn, 1);
                    lcd.print(" ");
                    goto back3;
                }
                else
                    goto back3;
            }
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                inputHour[i] = key;
            }
            cursorColumn++; // next position
            i++;
        }
        else
            goto back3;
    }
proceed3:
    cursorColumn = 0;
    i = 0;
    lcd.clear();
    lcd.print("HOUR STORED!");
    delay(500);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTER THE MINUTE:");
    while (i >= 0 && i < 2)
    {
    back4:
        char key = keypad.getKey();
        if (key)
        {
            lcd.setCursor(cursorColumn, 1); // move cursor to cursorColumn,1
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                lcd.print(key);
            }
            if (key == keys[0][3])
            {
                while (i != 0)
                {
                    inputMinute[i--] = {};
                }
                for (int j = 0; j < 16; j++)
                {
                    lcd.setCursor(j, 1);
                    lcd.print(" ");
                }
                cursorColumn = 0;
                goto back3;
            }
            else if (key == keys[3][3])
                goto proceed4;
            else if (key == keys[1][3])
            {
                if (cursorColumn != 0)
                {
                    cursorColumn--;
                    inputMinute[i--] = {};
                    lcd.setCursor(cursorColumn, 1);
                    lcd.print(" ");
                    goto back4;
                }
                else
                    goto back4;
            }
            if (key != keys[0][3] && key != keys[1][3] && key != keys[2][3] && key != keys[3][3] && key != keys[3][0] && key != keys[3][2])
            {
                inputMinute[i] = key;
            }
            cursorColumn++; // next position
            i++;
        }
        else
            goto back4;
    }
proceed4:
    cursorColumn = 0;
    i = 0;
    lcd.clear();
    lcd.print("MINUTE STORED!");
    delay(500);
    lcd.clear();
    lcd.print("PRESS ENTER");
    while (1)
    {
        char key = keypad.getKey();
        if (key)
        {
            if (key == keys[3][3])
                goto proceed5;
        }
    }
proceed5:
    // sets the RTC to the date & time input by the user
    rtc.adjust(DateTime(F(inputYear, inputMonth, inputDay), F(inputHour, inputMinute, 0))); // rtc.adjust(DateTime(Year, Month, Day, Hour, Minute, 0 seconds))
    inputDay[2] = {};
    inputMonth[2] = {};
    inputYear[4] = {};
    inputHour[2] = {};
    inputMinute[2] = {};
}

void filling(int capacity, int bottles)
{
    reservoirTankCheck(capacity, bottles);
    lcd.setCursor(0, 0);
    lcd.print("SET VALUE:");
    lcd.setCursor(10, 0);
    lcd.print(capacity);
    lcd.print("ml");

    float filled = 0;
    float cal = 0;
    float counter = 0;
    lcd.setCursor(10, 1);
    lcd.print(counter);
    lcd.print("ml ");
    delay(250);
    lcd.setCursor(0, 1);
    lcd.print("FILLING:");

    while (1)
    {
        digitalWrite(PumpRelay, HIGH);
        if (!digitalRead(FlowSensorPin) && state)
        {
            counter++;
            state = false;
        }
        if (digitalRead(FlowSensorPin))
        {
            state = true;
        }
        long val_t = map(capacity, 0, 9999, 50000, 110000); // Calibrate last 2 values to fine tune the machine (minimum 40000 maximum 200000)
        float f_val = val_t * 0.000009;
        cal = capacity * f_val, 0;
        filled = counter / f_val, 0;
        lcd.setCursor(10, 1);
        lcd.print(int(filled));
        lcd.print("ml  ");

        if (counter >= cal)
            break;
    }

    digitalWrite(PumpRelay, LOW);
    delay(30);
    lcd.setCursor(0, 1);
    lcd.print("FILLED: ");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" ***FILLING*** ");
    lcd.setCursor(0, 1);
    lcd.print("****COMPLETE****");
}

void logData(int bottles, int capacity, char filename[])
{

    // open file for writing
    myFile = SD.open(filename, FILE_WRITE);
    if (myFile)
    {
        // write timestamp
        DateTime now = rtc.now();
        myFile.print(now.year(), DEC);
        myFile.print('/');
        myFile.print(now.month(), DEC);
        myFile.print('/');
        myFile.print(now.day(), DEC);
        myFile.print(' ');
        myFile.print(" (");
        myFile.print(daysOfTheWeek[now.dayOfTheWeek()]);
        myFile.print(") ");
        myFile.print(' ');
        myFile.print(now.hour(), DEC);
        myFile.print(':');
        myFile.print(now.minute(), DEC);
        myFile.print(':');
        myFile.print(now.second(), DEC);
        myFile.print("  "); // delimiter between timestamp and data
        myFile.print("Bottles:");
        myFile.print(bottles);
        myFile.print("  ");
        myFile.print("Total Capacity:");
        myFile.print(capacity * bottles);
        myFile.print("  ");
        myFile.write("\n"); // new line
        myFile.close();
    }

    else
    {
        lcd.clear();
        lcd.print("SD CARD ERROR!");
        while (1)
            ; // don't do anything else
        delay(2000);
    }
}

int convertVariables(char botArray[], char capArray[])
{
    int bottles = atoi(&botArray[0]);
    int capacity = atoi(&capArray[0]);
    return bottles, capacity;
}

char *updateName(char filename[])
{
    DateTime now = rtc.now();
    int hour = now.hour();
    int minute = now.minute();

    if (hour == DAILY_EVENT_START_HH && minute == DAILY_EVENT_START_MM)
    {
        int year = now.year();
        int month = now.month();
        int day = now.day();

        // update filename

        filename[0] = (year / 1000) + '0';
        filename[1] = ((year % 1000) / 100) + '0';
        filename[2] = ((year % 100) / 10) + '0';
        filename[3] = (year % 10) + '0';

        filename[4] = (month / 10) + '0';
        filename[5] = (month % 10) + '0';

        filename[6] = (day / 10) + '0';
        filename[7] = (day % 10) + '0';

        return filename;
    }
    delay(2000);
}

void displayTodaysLogs(char filename[])
{
    myFile = SD.open(filename, FILE_READ);
    lcd.clear();
    lcd.setCursor(16, 0);
    lcd.autoscroll();

    if (myFile)
    {
        while (myFile.available())
        {                                // execute while file is available
            char letter = myFile.read(); // read next character from file
            lcd.print(letter);           // display character
            delay(300);
        }
        myFile.close(); // close file
    }
    lcd.clear();
}

void sendLogs(String number, int botTrack, int capTrack)
{
    sim.println("AT");
    delay(500);
    sim.println("AT+CMGF=1"); // Configuring TEXT mode
    delay(500);
    sim.println("AT+CMGS=\"" + number + "\"\r"); // Mobile phone number to send message
    delay(500);
    String botString = String(botTrack);
    String capString = String(capTrack);
    String SMS = "Bottles:" + botString + "\n" + "Capacity: " + capString + "ml";
    sim.println(SMS);
    delay(1000);
    sim.println((char)26); // ASCII code of CTRL+Z
    delay(1000);
}

void sendLogsRequest(String number, int botTrack, int capTrack, int index1, int index2)
{
    sim.println("AT");
    delay(500);
    sim.println("AT+CMGF=1"); // Configuring TEXT mode
    delay(500);
    sim.println("AT+CNMI=2,2,0,0,0");
    delay(500);
    String sms;
    while (sim.available() > 0)
    {
        char receivedSMS;
        receivedSMS = sim.read();
        sms.concat(receivedSMS);
    }
    index1 = sms.indexOf('Logs');
    index2 = sms.indexOf('logs');
    if (index1 != -1 || index2 != -1)
    {
        sendLogs(number, botTrack, capTrack);
    }
    index1 = -1;
    index2 = -1;
    return index1, index2;
}

void sendTodaysLogsScheduled(String number, char filename[], int botTrack, int capTrack)
{
    DateTime now = rtc.now();
    int hour = now.hour();
    int minute = now.minute();

    if (hour == DAILY_EVENT_RECORD_HH)
    {
        sendLogs(number, botTrack, capTrack);
    }
}

int counter(int cursorColumn, int i, int capacity, int bottles, int botTrack, int capTrack, int count, char botArray[], char capArray[])
{
    count++;
    if (count == bottles)
    {
        lcd.clear();
        lcd.print("PROCESS COMPLETE");
        botTrack += bottles;
        capTrack += capacity;
        logData(bottles, capacity, filename);
        databaseSafaricom(botArray, capArray);
        botArray[10] = {};
        capArray[10] = {};
        count, bottles, capacity, cursorColumn, i = 0;
    }
    return botTrack, capTrack, count, bottles, capacity, cursorColumn, i;
}

int resetVariables(int botTrack, int capTrack)
{
    DateTime now = rtc.now();
    int hour = now.hour();
    int minute = now.minute();

    if (hour == DAILY_EVENT_START_HH && minute == DAILY_EVENT_START_MM)
    {
        botTrack, capTrack = 0;
    }
    return botTrack, capTrack;
}

void emergencyStop()
{
    digitalWrite(PumpRelay, LOW);
    digitalWrite(TimerRelay, LOW);
    digitalWrite(conveyorRelay, LOW);
    digitalWrite(capMotorRelay, LOW);
    digitalWrite(BuzzerRelay, LOW);
    digitalWrite(ledPinYellow, LOW);
    digitalWrite(ledPinGreen, LOW);
    digitalWrite(ledPinBlue, LOW);
    digitalWrite(ledPinRed, HIGH);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MACHINE STOPPED!");
    lcd.setCursor(0, 1);
    lcd.print("****************");
    while (1)
    {
        // don't do anything else until continue button is pressed
        if (digitalRead(CONTINUE) == HIGH)
        {
            debounceContinue();
            break;
        }
    }
    digitalWrite(ledPinRed, LOW);
    lcd.clear();
}

void databaseSafaricom(char botArray[], char capArray[])
{
    sim.println("AT");
    delay(500);
    sim.println("AT+CGATT?"); // Attach or de-attach devive to packet domain service
    delay(1000);
    sim.println("AT+CIPSHUT"); // Close the GPRS PDP context
    delay(1000);
    sim.println("AT+CIPSTATUS"); // Internet connection status of the device
    delay(2000);
    sim.println("AT+CIPMUX=0"); // Configures device for single or multi IP connection
    delay(2000);
    sim.println("AT+CSTT=\"http://www.safaricom.co.ke\""); // start Task and setting the APN
    delay(1000);
    sim.println("AT+CIICR"); // Bring up wireless communication
    delay(3000);
    sim.println("AT+CIFSR"); // Get local IP address
    delay(2000);
    sim.println("AT+CIPSPRT=0");
    delay(3000);
    sim.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\""); // Start up the connection
    delay(6000);
    sim.println("AT+CIPSEND"); // Begin to send remote data to remote server
    delay(4000);
    String str = "GET https://api.thingspeak.com/update?api_key=PVNEDFVY2H6SOPN3&field1=" + String(botArray) + "&field2=" + String(capArray);
    sim.println(str); // Begin to send data to remote server
    delay(4000);
    sim.println((char)26); // Sending
    delay(5000);           // Waiting for reply, it is very important! The time is based on the condition of the internet
    sim.println();
    delay(500);
    sim.println("AT+CIPSHUT"); // Close the connection
    delay(1000);
}

void reservoirTankCheck(int capacity, int bottles)
{
    int tankCapacity = ((sonar.ping_median(iterations) / 2) * 0.0343);
    int reservoirCapacity = map(tankCapacity, 0, 25, 0, 10000);
    while (1)
    {
        if (reservoirCapacity < (capacity * bottles))
        {
            digitalWrite(BuzzerRelay, HIGH);
            delay(500);
            digitalWrite(BuzzerRelay, LOW);
            delay(500);
            digitalWrite(BuzzerRelay, HIGH);
            delay(500);
            digitalWrite(BuzzerRelay, LOW);
            digitalWrite(ledPinYellow, HIGH);
            lcd.clear();
            lcd.print("REFILL RESERVOIR");
        }
        else
        {
            digitalWrite(ledPinYellow, LOW);
            break;
        }
    }
}

void capping()
{
capcheck:
    if (digitalRead(IR_CAPHOLDER) == LOW)
    {
        while (1)
        {
            currentState4 = digitalRead(IR_CAPHOLDER);

            if (lastState4 == HIGH && currentState4 == LOW)
            {
                goto capContinue;
            }

            else if (lastState4 == LOW && currentState4 == HIGH)
                delay(50);
            //  save the the last state
            lastState4 = currentState4;
        }
    }

    else if (digitalRead(IR_CAPHOLDER) == HIGH)
    {
        digitalWrite(BuzzerRelay, HIGH);
        delay(500);
        digitalWrite(BuzzerRelay, LOW);
        delay(500);
        digitalWrite(BuzzerRelay, HIGH);
        delay(500);
        digitalWrite(BuzzerRelay, LOW);
        digitalWrite(ledPinYellow, HIGH);
        lcd.clear();
        lcd.print("NO CAP IN HOLDER");
        goto capcheck;
    }

capContinue:
    digitalWrite(conveyorRelay, LOW);
    digitalWrite(ledPinYellow, LOW);
    digitalWrite(TimerRelay, HIGH);
    digitalWrite(capMotorRelay, HIGH);

    servo.write(98);
    delay(1600);

    servo.write(92);
    delay(1600);

    servo.write(84);
    delay(1500);

    servo.write(92);
    delay(3000);

    servo.write(98);
    delay(1600);

    servo.write(92);

    digitalWrite(capMotorRelay, LOW);
    digitalWrite(TimerRelay, LOW);

    digitalWrite(conveyorRelay, HIGH);
}

void displayDate()
{
    DateTime now = rtc.now();
    lcd.clear();
    int x = 0;
    while (1)
    {
        lcd.setCursor(0, 0);
        lcd.print("DATE:");
        lcd.print(now.year(), DEC);
        lcd.print('/');
        lcd.print(now.month(), DEC);
        lcd.print('/');
        lcd.print(now.day(), DEC);
        lcd.setCursor(0, 1);
        lcd.print("TIME:");
        lcd.print(now.hour(), DEC);
        lcd.print(':');
        lcd.print(now.minute(), DEC);
        lcd.print(':');
        lcd.print(now.second(), DEC);
        x++;
        if (x > 5000)
            break;
    }
    lcd.clear();
}

void debounceStart()
{
    // DEBOUNCING DEFINITIONS
    unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
    unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers
    int reading = digitalRead(START);

    while (1)
    {
        lastDebounceTime = millis();
        if ((millis() - lastDebounceTime) > debounceDelay)
        {
            break;
        }
    }
}

void debounceContinue()
{
    // DEBOUNCING DEFINITIONS
    unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
    unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers
    int reading = digitalRead(CONTINUE);

    while (1)
    {
        lastDebounceTime = millis();
        if ((millis() - lastDebounceTime) > debounceDelay)
        {
            break;
        }
    }
}

void defaultDisplay()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("***--DONE!--****");
    delay(200);
    lcd.setCursor(0, 1);
    lcd.print("**PRESS START**");
    delay(500);
}

void displayTrackedLogs(int botTrack, int capTrack)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BOTTLES:" + String(botTrack));
    lcd.print("CAPACITY:" + String(capTrack));
    delay(3000);
}
