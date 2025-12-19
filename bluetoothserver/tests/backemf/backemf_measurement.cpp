#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <signal.h>

// Configuration
const int DIS_PIN = 22;           // GPIO22 for motor disable (BCM numbering)
const int ADS1015_ADDR = 0x48;    // I2C address of ADS1015
const int WAIT_TIME_US = 300;     // Wait time in microseconds after DIS=1

// ADS1015 Registers
const int ADS1015_REG_POINTER_CONVERT = 0x00;
const int ADS1015_REG_POINTER_CONFIG = 0x01;

// Global for cleanup
int i2cFd = -1;
volatile bool running = true;

void signalHandler(int signum) {
    std::cout << "\n\nStopping..." << std::endl;
    running = false;
}

    // Config Register (16-bit):
    // Bit 15:    OS (Operational Status) = 1 (start single conversion)
    // Bit 14-12: MUX = 000 (AIN0 - AIN1 differential)
    // Bit 11-9:  PGA = 001 (001 ±4.096V,    010 ±2.048V)
    // Bit 8:     MODE = 1 (1 single-shot mode, 0=contiuous)
    // Bit 7-5:   DR = 100 (1600SPS - default, 111 3300SPS )
    // Bit 4:     COMP_MODE = 0 (kann alert pin bei grösser oder kleiner aut high setzen, brauch ma ned)
    // Bit 3:     COMP_POL = 0
    // Bit 2:     COMP_LAT = 0
    // Bit 1-0:   COMP_QUE = 11 (disable comparator)
    
    //const uint16_t ADS1015_config = 0b0000001110000011;
    const uint16_t ADS1015_config = 0b0000010111100011;
    //                                OM..P..MD..COMP.
    //                                SU  G  OR
    //                                 X  A  D
    // OS=0, MUX=000, PGA=001, MODE=1, DR=100, rest=00011
    // OS=0, MUX=000, PGA=010, MODE=1, DR=111, rest=00011
// Configure ADS1015 for differential measurement AIN0-AIN1
void configureADS1015(int fd) {
    
    // Write config register (MSB first)
    uint16_t config = __bswap_16 (ADS1015_config) ;
    wiringPiI2CWriteReg16(fd, ADS1015_REG_POINTER_CONFIG, config);
}

// Read conversion result from ADS1015
void start_readADS1015(int fd) {
    // Start conversion
    //uint16_t config = 0x8483;  // Same config as above with OS=1
    uint16_t config = __bswap_16(ADS1015_config | 0x8000);  // Same config as above with OS=1
    wiringPiI2CWriteReg16(fd, ADS1015_REG_POINTER_CONFIG, config);
}
    
int readADS1015(int fd) {
    // Wait for conversion (at 1600 SPS = 0.625ms per conversion)
    // We wait a bit less since we're reading status
    //usleep(100);  // 100µs initial wait
    
    // Poll for conversion complete (optional, for speed we can skip this)
    // For maximum speed, just wait the conversion time
    //usleep(550);  // Rest of conversion time (~650µs total)
    
	#define	CONFIG_OS_MASK		(0x8000)	// Operational Status Register

    for (;;) {
	uint16_t result =  wiringPiI2CReadReg16 (fd, ADS1015_REG_POINTER_CONFIG) ;
	result = __bswap_16 (result) ;
	if ((result & CONFIG_OS_MASK) != 0)
		break ;
	delayMicroseconds (100) ;
	printf("conversion not yet ready\n");
    }

    // Read conversion register
    int16_t raw = wiringPiI2CReadReg16(fd, ADS1015_REG_POINTER_CONVERT);
    raw = __bswap_16 (raw) ;
    raw >>=4;
    
    return raw;
}

// Fast precise delay using busy-wait
void preciseDelay(int microseconds) {
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::microseconds(microseconds);
    
    while (std::chrono::high_resolution_clock::now() < end) {
        // Busy-wait for maximum precision
    }
}

int main() {
    std::cout << "Back-EMF Measurement (C++ with WiringPi)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "DIS Pin: GPIO" << DIS_PIN << " (BCM numbering)" << std::endl;
    std::cout << "Wait time: " << WAIT_TIME_US << "µs" << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;
    
    // Setup signal handler
    signal(SIGINT, signalHandler);
    
    // Initialize WiringPi (use BCM GPIO numbering)
    if (wiringPiSetupGpio() < 0) {
        std::cerr << "ERROR: Failed to setup WiringPi!" << std::endl;
        std::cerr << "Run as root: sudo ./backemf_measurement" << std::endl;
        return 1;
    }
    
    // Setup DIS pin as output
    pinMode(DIS_PIN, OUTPUT);
    digitalWrite(DIS_PIN, LOW);  // Start with motor enabled
    std::cout << "✓ GPIO" << DIS_PIN << " configured as output" << std::endl;
    
    // Setup I2C for ADS1015
    i2cFd = wiringPiI2CSetup(ADS1015_ADDR);
    if (i2cFd < 0) {
        std::cerr << "ERROR: Failed to setup I2C!" << std::endl;
        std::cerr << "Make sure I2C is enabled: sudo raspi-config" << std::endl;
        return 1;
    }
    std::cout << "✓ I2C configured (ADS1015 at 0x" << std::hex << ADS1015_ADDR << std::dec << ")" << std::endl;
    
    // Configure ADS1015
    configureADS1015(i2cFd);
    std::cout << "✓ ADS1015 configured (differential AIN0-AIN1, 1600 SPS)\n" << std::endl;
    
    int measurement_count = 0;
    double timing_sum = 0;
    int timing_count = 0;
    
    while (running) {
        auto start_total = std::chrono::high_resolution_clock::now();
        
        // Enable DIS (motor disabled) - FAST!
        digitalWrite(DIS_PIN, HIGH);
        
        start_readADS1015(i2cFd);
        auto i2c_total = std::chrono::high_resolution_clock::now();
        // Wait for voltage to stabilize
        preciseDelay(WAIT_TIME_US);

        digitalWrite(DIS_PIN, LOW);
        auto dis_total = std::chrono::high_resolution_clock::now();
        // Read back-EMF
        int16_t backemf = readADS1015(i2cFd);
        
        // Disable DIS (motor enabled again) - FAST!
        //digitalWrite(DIS_PIN, LOW);
        auto end_total = std::chrono::high_resolution_clock::now();
        
        double elapsed_i2c_us = std::chrono::duration<double, std::micro>(i2c_total - start_total).count();
        double elapsed_dis_us = std::chrono::duration<double, std::micro>(dis_total - start_total).count();
        double elapsed_us = std::chrono::duration<double, std::micro>(end_total - start_total).count();
        
        // Calculate running average
        timing_sum += elapsed_us;
        timing_count++;
        if (timing_count > 20) {
            timing_sum -= elapsed_us;  // Approximate rolling average
            timing_count = 20;
        }
        double avg_timing = timing_sum / timing_count;
        
        // Display result
        measurement_count++;
        
        std::cout << std::setfill(' ') << std::setw(4) << "[" << measurement_count << "] "
                  << "Back-EMF: " << std::setw(6) << backemf
                  << "  |  I2C time: " << std::fixed << std::setprecision(0) << elapsed_i2c_us << "µs"
                  << "  |  DIS time: " << std::fixed << std::setprecision(0) << elapsed_dis_us << "µs"
                  << "  |  total time: " << std::fixed << std::setprecision(0) << elapsed_us << "µs";
        
        // Show average every 10 measurements
        if (measurement_count % 10 == 0) {
            std::cout << " (avg: " << avg_timing << "µs)";
        }
        
        std::cout << std::endl;
        
        // Wait before next measurement (~1 Hz)
        // Subtract the time we already spent
        int sleep_us = 1000000 - static_cast<int>(elapsed_us);
        if (sleep_us > 0) {
            usleep(sleep_us);
        }
    }
    
    // Cleanup
    std::cout << "\nCleaning up..." << std::endl;
    digitalWrite(DIS_PIN, LOW);  // Ensure motor is enabled
    std::cout << "Exiting..." << std::endl;
    
    return 0;
}
