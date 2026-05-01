#!/usr/bin/env python3
"""
Back-EMF Measurement Script
Reads motor back-EMF using ADS1015 via sysfs
"""

import time
import sys
import os

# Configuration
DIS_PIN = 22  # GPIO22 for motor disable
WAIT_TIME_MS = 0.3  # Wait time in milliseconds after DIS=1
ADS1015_RAW = "/sys/bus/i2c/devices/i2c-1/1-0048/iio:device0/in_voltage0-voltage1_raw"
GPIO_EXPORT = "/sys/class/gpio/export"
GPIO_BASE = "/sys/class/gpio/gpio{}"

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

def setup_gpio(pin):
    """Setup GPIO pin as output"""
    try:
        # Export GPIO if not already exported
        try:
            with open(GPIO_EXPORT, 'w') as f:
                f.write(str(pin))
        except OSError:
            # Already exported, that's ok
            pass
        
        # Set direction to output
        direction_path = GPIO_BASE.format(pin) + "/direction"
        with open(direction_path, 'w') as f:
            f.write("out")
        
        # Set initial value to 0 (motor enabled)
        value_path = GPIO_BASE.format(pin) + "/value"
        with open(value_path, 'w') as f:
            f.write("0")
        
        print(f"GPIO{pin} configured as output")
        return True
        
    except Exception as e:
        print(f"Error setting up GPIO{pin}: {e}")
        return False

def set_gpio(pin, value):
    """Set GPIO pin value (0 or 1)"""
    value_path = GPIO_BASE.format(pin) + "/value"
    with open(value_path, 'w') as f:
        f.write(str(value))

def read_backemf():
    """Read raw ADC value from ADS1015"""
    try:
        with open(ADS1015_RAW, 'r') as f:
            raw_value = int(f.read().strip())
        return raw_value
    except Exception as e:
        print(f"Error reading ADC: {e}")
        return None

def cleanup_gpio(pin):
    """Unexport GPIO on exit"""
    try:
        # Set to 0 before cleanup
        set_gpio(pin, 0)
        
        # Unexport
        unexport_path = "/sys/class/gpio/unexport"
        with open(unexport_path, 'w') as f:
            f.write(str(pin))
        print(f"\nGPIO{pin} cleaned up")
    except:
        pass

def main():
    print("Back-EMF Measurement Script")
    print("=" * 50)
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
    
    print()
    
    # Setup GPIO
    if not setup_gpio(DIS_PIN):
        print("Failed to setup GPIO. Run as root: sudo python3 script.py")
        sys.exit(1)
    
    try:
        measurement_count = 0
        timing_samples = []
        
        while True:
            # Measure actual timing
            start = time.perf_counter()
            
            # Enable DIS (motor disabled)
            set_gpio(DIS_PIN, 1)
            
            # Wait for voltage to stabilize
            precise_sleep(WAIT_TIME_MS / 1000.0)
            
            # Read back-EMF
            backemf = read_backemf()
            
            # Disable DIS (motor enabled again)
            set_gpio(DIS_PIN, 0)
            
            # Measure elapsed time
            elapsed_ms = (time.perf_counter() - start) * 1000.0
            timing_samples.append(elapsed_ms)
            
            # Keep only last 10 samples for averaging
            if len(timing_samples) > 10:
                timing_samples.pop(0)
            avg_timing = sum(timing_samples) / len(timing_samples)
            
            # Display result
            if backemf is not None:
                measurement_count += 1
                print(f"[{measurement_count:4d}] Back-EMF: {backemf:6d}  |  DIS time: {elapsed_ms:.2f}ms (avg: {avg_timing:.2f}ms)")
            else:
                print("Failed to read ADC")
            
            # Wait before next measurement (adjust for actual timing)
            remaining_time = max(0.001, 1.0 - elapsed_ms/1000.0)
            time.sleep(remaining_time)
    
    except KeyboardInterrupt:
        print("\n\nStopped by user")
    
    except Exception as e:
        print(f"\nError: {e}")
    
    finally:
        cleanup_gpio(DIS_PIN)
        print("Exiting...")

if __name__ == "__main__":
    main()
