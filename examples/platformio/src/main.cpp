#include <battery_percent.h>

#ifdef ARDUINO
#include <Arduino.h>

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    uint32_t millivolts = battery_percent_adc_to_millivolts(analogRead(34), 12, 3300, 100000, 100000);
    Serial.println(battery_percent_from_millivolts(&battery_percent_curve_lipo, (uint16_t)millivolts));
    delay(1000);
}
#else
#include <stdio.h>

int main()
{
    uint8_t percent = battery_percent_from_millivolts(&battery_percent_curve_lipo, 3821);
    printf("%u%%\n", (unsigned)percent);
    return percent == 50 ? 0 : 1;
}
#endif
