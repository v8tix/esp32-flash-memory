#include <driver/dac.h>
#include <math.h>
#include <FS.h>
#include <SPIFFS.h>

const char *carrierFilePath = "/carrier_wave.txt";
const char *modulatingFilePath = "/modulating_wave.txt";
const char *modulatedFilePath = "/modulated_wave.txt";
const int carrierFrequency = 250;             // Frequency of the carrier wave in Hz
const float carrierAmplitude = 1.0;            // Amplitude of the carrier wave
const int modulatingFrequency = 50;            // Frequency of the modulating wave in Hz
const float modulatingAmplitude = 1.0;         // Amplitude of the modulating wave
const int sampleRate = 16000;                   // Sample rate in samples per second
const int outputDelay = 50;                    // Delay in microseconds between DAC outputs

int numCarrierSamples;      // Number of samples in the carrier buffer
uint8_t *carrierDacValues;  // Buffer to hold the carrier DAC values

int numModulatingSamples;      // Number of samples in the modulating buffer
uint8_t *modulatingDacValues;  // Buffer to hold the modulating DAC values

int numModulatedSamples;        // Number of samples in the modulated buffer
uint8_t *modulatedDacValues;    // Buffer to hold the modulated DAC values

void setup() {
  Serial.begin(9600);

  // Mount SPIFFS
  if (!SPIFFS.begin(true)) { // true: format on fail
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  if (SPIFFS.exists(carrierFilePath) && SPIFFS.exists(modulatingFilePath) && SPIFFS.exists(modulatedFilePath)) {
    eraseFiles();
    generateAndSaveWave(carrierFilePath, carrierFrequency, carrierAmplitude);
    generateAndSaveWave(modulatingFilePath, modulatingFrequency, modulatingAmplitude);
    generateAndSaveModulatedWave(modulatedFilePath, carrierFrequency, carrierAmplitude, modulatingFrequency, modulatingAmplitude, sampleRate);
  } else {
    generateAndSaveWave(carrierFilePath, carrierFrequency, carrierAmplitude);
    generateAndSaveWave(modulatingFilePath, modulatingFrequency, modulatingAmplitude);
    generateAndSaveModulatedWave(modulatedFilePath, carrierFrequency, carrierAmplitude, modulatingFrequency, modulatingAmplitude, sampleRate);
  }

  // Initialize DACs
  dac_output_enable(DAC_CHANNEL_1);
  dac_output_enable(DAC_CHANNEL_2);

  // Read the files into the buffers
  readFromFile(carrierFilePath, numCarrierSamples, carrierDacValues);
  readFromFile(modulatingFilePath, numModulatingSamples, modulatingDacValues);
  readFromFile(modulatedFilePath, numModulatedSamples, modulatedDacValues);
}

void loop() {
    // Play the carrier wave
    //playWave(carrierDacValues, numCarrierSamples, DAC_CHANNEL_1);

    // Play the modulating wave
    //playWave(modulatingDacValues, numModulatingSamples, DAC_CHANNEL_2);

    // Play the modulated wave
    playWave(modulatedDacValues, numModulatedSamples, DAC_CHANNEL_2);
}

void generateAndSaveModulatedWave(
    const char *filePath, 
    int carrierFrequency, 
    float carrierAmplitude,
    int modulatingFrequency, 
    float modulatingAmplitude,
    int sampleRate
) {
    // Create a file
    File file = SPIFFS.open(filePath, FILE_WRITE);
    if (file) {
        // Calculate the number of samples for one period
        int numSamples = static_cast<int>(sampleRate / carrierFrequency);

        // Create a buffer to hold the DAC values
        uint8_t *dacValues = new uint8_t[numSamples];

        for (int i = 0; i < numSamples; ++i) {
            // Calculate the value of the modulated wave at the current time
            float t = static_cast<float>(i) / sampleRate;
            float carrierValue = carrierAmplitude * sin(2 * M_PI * carrierFrequency * t);
            float modulatingValue = modulatingAmplitude * sin(2 * M_PI * modulatingFrequency * t);
            float modulatedFrequency = carrierFrequency + modulatingValue * carrierFrequency;

            // Convert the value to an 8-bit DAC value (0-255)
            dacValues[i] = static_cast<uint8_t>((sin(2 * M_PI * modulatedFrequency * t) + 1.0) * 127.5);
        }

        // Write the entire buffer to the file
        file.write(dacValues, numSamples * sizeof(uint8_t));

        // Clean up: close the file
        file.close();

        Serial.println("Modulated wave saved to SPIFFS");
    } else {
        Serial.println("Error creating file for modulated wave");
    }
}

void generateAndSaveWave(const char *filePath, int frequency, float amplitude) {
  // Create a file
  File file = SPIFFS.open(filePath, FILE_WRITE);
  if (file) {
    // Calculate the number of samples for one period
    int numSamples = static_cast<int>(sampleRate / frequency);

    // Create a buffer to hold the DAC values
    uint8_t *dacValues = new uint8_t[numSamples];

    for (int i = 0; i < numSamples; ++i) {
      // Calculate the value of the sine wave at the current time
      float t = static_cast<float>(i) / sampleRate;
      float value = amplitude * sin(2 * M_PI * frequency * t);

      // Convert the value to an 8-bit DAC value (0-255)
      dacValues[i] = static_cast<uint8_t>((value + 1.0) * 127.5);
    }

    // Write the entire buffer to the file
    file.write(dacValues, numSamples * sizeof(uint8_t));

    // Clean up: close the file
    file.close();

    Serial.println("Wave saved to SPIFFS");
  } else {
    Serial.println("Error creating file for wave");
  }
}

void playWave(uint8_t *dacValues, int numSamples, dac_channel_t dacChannel) {
  for (int i = 0; i < numSamples; ++i) {
    dac_output_voltage(dacChannel, dacValues[i]);
    delayMicroseconds(outputDelay); // Adjust delay as needed
  }
}

void readFromFile(const char *filePath, int &numSamples, uint8_t *&dacValues) {
  // Read the file
  File file = SPIFFS.open(filePath, "r");
  if (file) {
    // Calculate the number of samples in the file
    numSamples = file.size() / sizeof(uint8_t);

    // Create a buffer to hold the DAC values
    dacValues = new uint8_t[numSamples];

    // Read the entire buffer from the file
    if (file.read(dacValues, numSamples * sizeof(uint8_t)) != numSamples * sizeof(uint8_t)) {
      Serial.println("Error reading file");
    }

    // Clean up: close the file
    file.close();
  } else {
    Serial.println("Error opening file");
  }
}

void eraseFiles() {
  if (SPIFFS.remove(carrierFilePath) && SPIFFS.remove(modulatingFilePath) && SPIFFS.remove(modulatedFilePath)) {
    Serial.println("Files erased successfully");
  } else {
    Serial.println("Error erasing files");
  }
}
