#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\yacheSTS.h"
#ifndef yacheSTS_h
#define yacheSTS_h

#include <Arduino.h>
#include <SCServo.h>

class yacheSTS {
    public:
        yacheSTS();

        // STS motor can be initialized as name.begin(dklfj); 
        // delay(500);
        // setWHeelMode();
        void begin(HardwareSerial &serialPort, int enablePin, unsigned long baud = 1000000);
        void setWheelMode();
        void setWheelMode(bool enable);
        void power(int8_t lf, int8_t rf, int8_t lb, int8_t rb);
        void stop();

    private:
        SMS_STS _sts;
        int _enPin;
        uint8_t _ids[4] = {4, 1, 2, 3};
        int16_t _speeds[4] = {0, 0, 0, 0};   // Must be int16_t (s16)
        uint8_t _accs[4] = {150, 150, 150, 150};
};

#endif


