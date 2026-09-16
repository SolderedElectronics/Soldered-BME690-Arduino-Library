/**
 **************************************************
 *
 * @file        BME690_AI_Studio_Logger.ino
 * @brief       Records BME690 raw data into a .bmerawdata file on an SD card.
 *              The file follows the Bosch BME AI-Studio Raw Data Format, so it
 *              can be imported into BME AI-Studio and used to train a gas
 *              classification algorithm.
 *
 *              The sketch runs the sensor in parallel mode with a ten step
 *              heater profile, logs every valid gas measurement for
 *              LOG_DURATION_MS and then writes the finished file.
 *
 *              AI-Studio splits a recording into Specimens wherever the label
 *              tag changes, and it measures the duration of a Specimen from
 *              its first to its last point. A recording whose label tag never
 *              changes therefore imports as a Specimen of zero length. The
 *              sketch starts the recording already tagged, and the button on
 *              LABEL_BUTTON_PIN switches the tag while it runs, which marks
 *              the moment as the start of a new Specimen. That is the same
 *              thing buttons S1 and S2 do on the BME688 Development Kit.
 *
 *              Needed hardware:
 *              - an ESP32 board (WiFi and NTP are used for the real time clock)
 *              - a BME690 breakout on the I2C pins, or connected via Qwiic
 *              - an SD card module on the SPI pins, chip select on SD_CS
 *
 *              Needed libraries:
 *              - ArduinoJson (version 7 or newer)
 *
 *              The AI-Studio workflow is described here:
 *              https://www.bosch-sensortec.com/software/bme/docs/overview/getting-started.html
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Josip Šimun Kuči @ Soldered.com
 ***************************************************/

#include "BME690-SOLDERED.h"
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

// WiFi credentials, only used to get the current time over NTP.
const char *ssid = "YOUR_SSID_HERE";
const char *password = "YOUR_PASSWORD_HERE";

// Chip select pin of the SD card module.
#define SD_CS 5

// Button which marks the start of a new Specimen while recording. It is
// active low, so the internal pull up is enough and no wiring is needed if the
// board already has a button on this pin. Set it to -1 to record the whole
// session as a single Specimen.
#define LABEL_BUTTON_PIN 0

// Debounce time of that button.
#define LABEL_BUTTON_DEBOUNCE_MS 250

// Total duration of one measurement session.
#define LOG_DURATION_MS (10UL * 60000UL)

// Duration of a single heater profile step in milliseconds. The Raw Data
// Format expects the heater profile time base to be 140 ms.
#define MEAS_DUR 140

// New data, gas measurement valid and heater stable, all at once.
#define BME690_VALID_DATA (BME69X_NEW_DATA_MSK | BME69X_GASM_VALID_MSK | BME69X_HEAT_STAB_MSK)

// Identifier of the board which recorded the data. AI-Studio uses it to tell
// recordings of different boards apart, so give every board its own value.
#define BOARD_ID "E0E2E69BA804"

// Unique id of the sensor element, used to trace a row back to one sensor.
#define SENSOR_ID 1903381786UL

// Index of the sensor on the board. A single BME690 breakout only has one.
#define SENSOR_INDEX 0

BME690 sensor;

// Heater temperature profile in degrees Celsius.
uint16_t heaterTemp[10] = {320, 100, 100, 100, 200, 200, 200, 320, 320, 320};

// Multipliers of the shared heater duration, one per profile step.
uint16_t heaterMul[10] = {5, 2, 10, 30, 5, 5, 5, 5, 5, 5};

const uint8_t HEATER_LEN = 10;

// Stops the sketch if the sensor reported an error, prints warnings.
void checkSensorStatus()
{
    int8_t status = sensor.checkStatus();

    if (status == BME69X_ERROR)
    {
        Serial.print("BME690 error: ");
        Serial.println(sensor.statusString());
        while (1)
            ;
    }
    else if (status == BME69X_WARNING)
    {
        Serial.print("BME690 warning: ");
        Serial.println(sensor.statusString());
    }
}

// Formats a unix timestamp as an ISO 8601 UTC string, as the format requires.
String iso8601(time_t t)
{
    struct tm timeInfo;
    char buffer[32];

    gmtime_r(&t, &timeInfo);
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S+00:00", &timeInfo);

    return String(buffer);
}

// Formats a unix timestamp as the yyyy_mm_dd_hh_mm date used in the file name.
String fileNameDate(time_t t)
{
    struct tm timeInfo;
    char buffer[24];

    gmtime_r(&t, &timeInfo);
    strftime(buffer, sizeof(buffer), "%Y_%m_%d_%H_%M", &timeInfo);

    return String(buffer);
}

// Builds the random seed which labels all files of one measurement session.
// The format expects sixteen lowercase alphanumeric characters.
String makeSeed()
{
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    String seed = "";

    for (uint8_t i = 0; i < 16; i++)
    {
        seed += alphabet[random(sizeof(alphabet) - 1)];
    }

    return seed;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("\nBME690 AI-Studio raw data logger");

    // Connect to WiFi and get the current time, the format stores both an
    // absolute creation date and a unix timestamp for every measurement.
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWiFi connected");

    configTime(0, 0, "pool.ntp.org");
    time_t now = 0;
    Serial.print("Waiting for NTP");
    while ((now = time(nullptr)) < 1000000000UL)
    {
        Serial.print(".");
        delay(200);
    }
    Serial.println("\nTime synced");

    String isoNow = iso8601(now);

    // Seed the generator with the synced time so every session gets its own
    // seed instead of repeating the same one after every reset.
    randomSeed(now);
    String seedPowerOnOff = makeSeed();

    if (LABEL_BUTTON_PIN >= 0)
    {
        pinMode(LABEL_BUTTON_PIN, INPUT_PULLUP);
    }

    if (!SD.begin(SD_CS))
    {
        Serial.println("SD card initialization failed!");
        while (1)
            ;
    }
    Serial.println("SD card ready");

    sensor.begin();
    checkSensorStatus();

    // Default temperature, pressure and humidity oversampling.
    sensor.setTPH();
    checkSensorStatus();

    // The shared heating duration is the total measurement duration minus the
    // time needed for the temperature, pressure and humidity measurement.
    uint16_t sharedHeatrDur = MEAS_DUR - (sensor.getMeasDur(BME69X_PARALLEL_MODE) / 1000);

    sensor.setHeaterProf(heaterTemp, heaterMul, sharedHeatrDur, HEATER_LEN);
    checkSensorStatus();

    sensor.setOpMode(BME69X_PARALLEL_MODE);
    checkSensorStatus();

    // The file name carries the date, the board, the power on counter, the
    // session seed and the file counter, each part separated by an underscore.
    String fileName = "/" + fileNameDate(now) + "_Board_" + BOARD_ID + "_PowerOnOff_1_" + seedPowerOnOff +
                      "_File_0.bmerawdata";

    File file = SD.open(fileName, FILE_WRITE);
    if (!file)
    {
        Serial.println("Could not create the file on the SD card!");
        while (1)
            ;
    }

    // Everything except the measurements themselves is small enough to build
    // in memory, so the board configuration and the column description are
    // assembled with ArduinoJson first.
    JsonDocument doc;

    // The board configuration, the same two objects are stored in a .bmeconfig
    // file by AI-Studio.
    JsonObject configHeader = doc["configHeader"].to<JsonObject>();
    configHeader["dateCreated_ISO"] = isoNow;
    configHeader["appVersion"] = "2.2.0";
    configHeader["boardType"] = "soldered_bme690";
    configHeader["boardMode"] = "burn_in";
    configHeader["boardLayout"] = "grouped";

    JsonObject configBody = doc["configBody"].to<JsonObject>();

    JsonObject heaterProfile = configBody["heaterProfiles"].to<JsonArray>().add<JsonObject>();
    heaterProfile["id"] = "heater_354";
    heaterProfile["timeBase"] = MEAS_DUR;
    JsonArray temperatureTimeVectors = heaterProfile["temperatureTimeVectors"].to<JsonArray>();
    for (uint8_t i = 0; i < HEATER_LEN; i++)
    {
        JsonArray pair = temperatureTimeVectors.add<JsonArray>();
        pair.add(heaterTemp[i]);
        pair.add(heaterMul[i]);
    }

    // The sensor scans continuously, so there are no sleeping cycles.
    JsonObject dutyCycleProfile = configBody["dutyCycleProfiles"].to<JsonArray>().add<JsonObject>();
    dutyCycleProfile["id"] = "duty_1";
    dutyCycleProfile["numberScanningCycles"] = 1;
    dutyCycleProfile["numberSleepingCycles"] = 0;

    JsonObject sensorConfiguration = configBody["sensorConfigurations"].to<JsonArray>().add<JsonObject>();
    sensorConfiguration["sensorIndex"] = SENSOR_INDEX;
    sensorConfiguration["heaterProfile"] = "heater_354";
    sensorConfiguration["dutyCycleProfile"] = "duty_1";

    // The header of the recording itself, the counters and the seed repeat the
    // matching parts of the file name.
    JsonObject rawDataHeader = doc["rawDataHeader"].to<JsonObject>();
    rawDataHeader["counterPowerOnOff"] = 1;
    rawDataHeader["seedPowerOnOff"] = seedPowerOnOff;
    rawDataHeader["counterFileLimit"] = 0;
    rawDataHeader["dateCreated"] = String((uint32_t)now);
    rawDataHeader["dateCreated_ISO"] = isoNow;
    rawDataHeader["firmwareVersion"] = "1.0.0";
    rawDataHeader["boardId"] = BOARD_ID;

    // The column description of the data block which follows it. The order of
    // the columns here is the order of the values in every recorded row.
    JsonObject rawDataBody = doc["rawDataBody"].to<JsonObject>();
    JsonArray dataColumns = rawDataBody["dataColumns"].to<JsonArray>();

    const char *columns[][4] = {
        {"Sensor Index", "", "integer", "sensor_index"},
        {"Sensor ID", "", "integer", "sensor_id"},
        {"Time Since PowerOn", "Milliseconds", "integer", "timestamp_since_poweron"},
        {"Real time clock", "Unix Timestamp: seconds since Jan 01 1970. (UTC); 0 = missing", "integer",
         "real_time_clock"},
        {"Temperature", "DegreesCelcius", "float", "temperature"},
        {"Pressure", "Hectopascals", "float", "pressure"},
        {"Relative Humidity", "Percent", "float", "relative_humidity"},
        {"Resistance Gassensor", "Ohms", "float", "resistance_gassensor"},
        {"Heater Profile Step Index", "", "integer", "heater_profile_step_index"},
        {"Scanning Mode Enabled", "", "boolean", "scanning_enabled"},
        {"Scanning Cycle Index", "", "integer", "scanning_cycle_index"},
        {"Label Tag", "", "integer", "label_tag"},
        {"Error Code", "", "integer", "error_code"},
    };

    for (uint8_t i = 0; i < sizeof(columns) / sizeof(columns[0]); i++)
    {
        JsonObject column = dataColumns.add<JsonObject>();
        column["name"] = columns[i][0];
        column["unit"] = columns[i][1];
        column["format"] = columns[i][2];
        column["key"] = columns[i][3];
        column["colId"] = i + 1;
    }

    // A ten minute recording holds a few thousand rows, which is far more than
    // fits in RAM as a JSON document. The part built above is therefore written
    // out first, with its two closing braces removed, so that the rows can be
    // appended to the file one by one as they are measured.
    String header;
    serializeJson(doc, header);
    header.remove(header.length() - 2);
    doc.clear();

    file.print(header);
    file.print(",\"dataBlock\":[");

    Serial.println("Recording, this takes " + String(LOG_DURATION_MS / 60000UL) + " minutes.");

    uint32_t start = millis();

    // The Raw Data Format counts the Scanning Cycles from one.
    uint32_t scanningCycleIndex = 1;

    uint32_t rows = 0;
    uint32_t droppedCycles = 0;

    // Recording starts already tagged, so that the whole session forms a
    // Specimen even when the button is never pressed. Pressing the button
    // switches the tag, which starts the next Specimen.
    uint8_t labelTag = 1;
    uint32_t lastButtonPress = 0;

    // No step has been seen yet, the first one is expected to be step zero.
    int16_t lastStepIndex = -1;

    uint8_t nFieldsLeft = 0;
    bme69xData data;
    char row[192];

    while (millis() - start < LOG_DURATION_MS)
    {
        // The shortest heater step of the profile lasts two time bases, so
        // polling twice per time base is fast enough not to miss one.
        delay(MEAS_DUR / 2);

        // Mark the start of a new Specimen when the button is pressed. The tag
        // alternates between one and two, the same way the two buttons of the
        // BME688 Development Kit are used.
        if (LABEL_BUTTON_PIN >= 0 && digitalRead(LABEL_BUTTON_PIN) == LOW &&
            millis() - lastButtonPress > LABEL_BUTTON_DEBOUNCE_MS)
        {
            lastButtonPress = millis();
            labelTag = (labelTag == 1) ? 2 : 1;
            Serial.println("New specimen, label tag is now " + String(labelTag));
        }

        if (!sensor.fetchData())
        {
            continue;
        }

        do
        {
            nFieldsLeft = sensor.getData(data);

            // Skip the fields which hold no valid gas measurement.
            if (data.status != BME690_VALID_DATA)
            {
                continue;
            }

            // One scanning cycle is one full sweep through the heater profile,
            // so the counter moves on whenever the step index wraps around.
            if (data.gas_index <= lastStepIndex)
            {
                scanningCycleIndex++;
            }

            // A step which is not the one after the previous step means the
            // measurement in between was lost. The format reports that as
            // error code 2, and AI-Studio drops the whole cycle on import.
            uint8_t errorCode = 0;
            if (data.gas_index != (lastStepIndex + 1) % HEATER_LEN)
            {
                errorCode = 2;
                droppedCycles++;
            }

            lastStepIndex = data.gas_index;

            // The format wants the pressure in hectopascals, the sensor
            // reports it in pascals.
            snprintf(row, sizeof(row), "%s[%d,%lu,%lu,%lu,%f,%f,%f,%f,%d,%d,%lu,%d,%d]", rows == 0 ? "" : ",",
                     SENSOR_INDEX, (unsigned long)SENSOR_ID, (unsigned long)(millis() - start),
                     (unsigned long)time(nullptr), data.temperature, data.pressure / 100.0f, data.humidity,
                     data.gas_resistance, data.gas_index, 1, (unsigned long)scanningCycleIndex, labelTag,
                     errorCode);
            file.print(row);
            rows++;

            Serial.println("T: " + String(data.temperature) + " C | P: " + String(data.pressure / 100.0f) +
                           " hPa | H: " + String(data.humidity) + " % | R: " + String(data.gas_resistance) +
                           " Ohm | step: " + String(data.gas_index));
        } while (nFieldsLeft);
    }

    // Close the data block, the rawDataBody object and the root object.
    file.print("]}}");
    file.close();

    Serial.println("Recording finished, " + String(rows) + " rows written, " + String(droppedCycles) +
                   " cycles marked as lost.");
    Serial.println("File saved as: " + fileName);
    Serial.println("Import it into BME AI-Studio to label the data and train an algorithm.");

    WiFi.disconnect(true);
}

void loop()
{
}
