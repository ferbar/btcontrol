#!/usr/bin/env python3
"""
Back-EMF Measurement Script with fast GPIO using libgpiod
Much faster than sysfs GPIO!
"""

import time
import sys
import os

try:
    import gpiod
except ImportError:
    print("ERROR: gpiod not installed!")
    print("Install with: sudo apt-get install python3-libgpiod")
    sys.exit(1)

try:
    import board
    i2c = board.I2C()
    from adafruit_ads1x15 import ADS1015, AnalogIn, ads1x15
    ads = ADS1015(i2c)
except Error:
    print("ERROR: ")
    sys.exit(1)


# Configuration
DIS_PIN = 22  # GPIO22 for motor disable
WAIT_TIME_MS = 0.05  # Wait time in milliseconds after DIS=1
ADS1015_RAW = "/sys/bus/i2c/devices/i2c-1/1-0048/iio:device0/in_voltage0-voltage1_raw"

def precise_sleep(seconds):
    """
    Precise sleep using hybrid approach:
    - time.sleep() for long durations (CPU-friendly)
    - Busy-wait for short durations (precise)
    """
    if seconds > 0.001:
        # Sleep most of it, leave 1ms for busy-wait
        time.sleep(seconds - 0.001)
        seconds = 0.001
    
    # Busy-wait the rest for precision
    end_time = time.perf_counter() + seconds
    while time.perf_counter() < end_time:
        pass

chan=AnalogIn(ads, ads1x15.Pin.A0, ads1x15.Pin.A1)
def adc_start_read():
    #pin = chan._pin_setting if chan.is_differential else chan_pin_setting + 0x04
    #chan._ads._write_config(pin)
    _ADS1X15_POINTER_CONFIG = 0x01
    chan._ads._write_register(_ADS1X15_POINTER_CONFIG, 33667)
#    chan._ads.i2c.write(self.buf)

def adc_wait_complete():
    # Continuously poll conversion complete status bit
    while not chan._ads._conversion_complete():
        pass
    return chan._ads.get_last_result(False)


def read_backemf():
    """Read raw ADC value from ADS1015"""
    try:
        #with open(ADS1015_RAW, 'r') as f:
        #    raw_value = int(f.read().strip())
        #return raw_value
        return chan.value
    except Exception as e:
        print(f"Error reading ADC: {e}")
        return None

def main():
    print("Back-EMF Measurement Script (with fast gpiod)")
    print("=" * 60)
    print(f"DIS Pin: GPIO{DIS_PIN}")
    print(f"Wait time: {WAIT_TIME_MS}ms")
    print(f"ADC Path: {ADS1015_RAW}")
    print("Press Ctrl+C to stop\n")
    
    # Try to set real-time priority for better timing
    try:
        os.sched_setscheduler(0, os.SCHED_FIFO, os.sched_param(50))
        print("✓ Real-time priority enabled (SCHED_FIFO, priority 50)")
    except PermissionError:
        print("⚠ Running without RT priority (run as root for better timing)")
    except Exception as e:
        print(f"⚠ Could not set RT priority: {e}")
    
    # Setup GPIO with gpiod (fast!)
    try:
        chip = gpiod.Chip('gpiochip0')
        dis_line = chip.get_line(DIS_PIN)
        dis_line.request(consumer="backemf", type=gpiod.LINE_REQ_DIR_OUT)
        dis_line.set_value(0)  # Start with motor enabled
        print(f"✓ GPIO{DIS_PIN} configured with gpiod (fast access!)\n")
    except Exception as e:
        print(f"ERROR: Could not setup GPIO with gpiod: {e}")
        print("Make sure you run as root: sudo python3 script.py")
        sys.exit(1)
    
    try:
        measurement_count = 0
        timing_samples = []
        gpio_timing_samples = []
        
        while True:
            # Measure total timing
            start_total = time.perf_counter()
            
            # Measure GPIO timing
            gpio_start = time.perf_counter()
            dis_line.set_value(1)  # Enable DIS (motor disabled) - FAST!
            gpio_time = (time.perf_counter() - gpio_start) * 1000000  # microseconds
            gpio_timing_samples.append(gpio_time)
            
            # Wait for voltage to stabilize
            precise_sleep(WAIT_TIME_MS / 1000.0)
            
            # Read back-EMF
            i2c_start = time.perf_counter()
            adc_start_read()
            i2c_time = (time.perf_counter() - i2c_start) * 1000000  # microseconds
            precise_sleep(WAIT_TIME_MS / 1000.0)
            dis_line.set_value(0)
            #backemf = read_backemf()
            
            # Disable DIS (motor enabled again) - FAST!
            #dis_line.set_value(0)
            
            # Measure elapsed time
            elapsed_ms = (time.perf_counter() - start_total) * 1000.0
            timing_samples.append(elapsed_ms)
            backemf = adc_wait_complete()
            
            # Keep only last 20 samples for averaging
            if len(timing_samples) > 20:
                timing_samples.pop(0)
            if len(gpio_timing_samples) > 20:
                gpio_timing_samples.pop(0)
            
            avg_timing = sum(timing_samples) / len(timing_samples)
            avg_gpio = sum(gpio_timing_samples) / len(gpio_timing_samples)
            
            # Display result
            if backemf is not None:
                measurement_count += 1
                
                # Show detailed timing every 10 measurements
                #if measurement_count % 10 == 0:
                if True:
                    print(f"[{measurement_count:4d}] Back-EMF: {backemf:6d}  |  "
                          f"DIS: {elapsed_ms:.2f}ms (avg: {avg_timing:.2f}ms)  |  "
                          f"GPIO: {gpio_time:.1f}µs (avg: {avg_gpio:.1f}µs) | "
                          f"I2C: {i2c_time:.1f}µs")
                else:
                    print(f"[{measurement_count:4d}] Back-EMF: {backemf:6d}  |  "
                          f"DIS: {elapsed_ms:.2f}ms")
            else:
                print("Failed to read ADC")
            
            # Wait before next measurement (adjust for actual timing)
            remaining_time = max(0.001, 1.0 - elapsed_ms/1000.0)
            time.sleep(remaining_time)
    
    except KeyboardInterrupt:
        print("\n\nStopped by user")
    
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        # Cleanup GPIO
        try:
            dis_line.set_value(0)  # Ensure motor is enabled
            dis_line.release()
            print("GPIO cleaned up")
        except:
            pass
        print("Exiting...")

if __name__ == "__main__":
    main()
