/*
I2C test routine
Sidney McHarg
September 28, 2021
*/

#define VERSION "Vrs 0.1d"

#define MODE_PIN        3       // tie to ground if device is slave 
#define SLAVE_ADDRESS   0    // default slave address, if 0 will be assigned dynamically
byte slaveAddress = SLAVE_ADDRESS;

#include "stdio.h"
#include <Wire.h>

// Wire.h includes a define for BUFFER_LENGTH, the size of the buffer, and hence maximum data
// payload which can be handled.  MAX_LEN is defined to be twice that in order to test for
// boundary conditions
#define MAX_LEN         BUFFER_LENGTH*2
byte buffer[MAX_LEN];

// Wire.endTransmission return values
#define WIRE_NOERROR        0   // no error
#define WIRE_TOOLONG        1   // too much data for buffer
#define WIRE_NAKADDRESS     2   // nak on address transmit
#define WIRE_NAKDATA        3   // nak on data transmit
#define WIRE_OTHER          4   // some other error

boolean mode;                   // master or slave

void setup()
{
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\nI2C Test " VERSION);
    Serial.println("Wire BUFFER_LENGTH = " + String(BUFFER_LENGTH));

    // determine role
    pinMode(MODE_PIN, INPUT_PULLUP);
    mode = digitalRead(MODE_PIN);
    Serial.print("Operating as ");
    Serial.println((mode ? "Master" : "Slave"));

    if (!mode)
    {
        // slave
        if (slaveAddress == 0)
        {
            slaveAddress = random(1,126);
        }
        Serial.println("Slave address set to 0x" + hexString(slaveAddress));
        Wire.begin(slaveAddress);
        // slave work all performed in loop()
        Wire.onRequest(wireRequest);
        Wire.onReceive(wireReceive);
        return;
    }
    else
    {   
        // master
        Wire.begin();
        scanBus();
        performTest();
        Serial.println("Tests completed\nReset master to repeat");
    }

}

void loop()
{
    // slave role
    ;
    
}

void wireRequest()
{
    // slave has received a request
    Serial.print("Request received, replying with ");
    /*
    size_t reqlen = Wire.available();
    Serial.print(reqlen);
    Serial.println(" bytes");
    if (reqlen > MAX_LEN)
        reqlen = MAX_LEN;
    size_t rslt = Wire.readBytes(buffer,reqlen);
    */
    size_t rslt = Wire.write(buffer, 32);
    Serial.println(String(rslt) + " bytes");
    if (Wire.getWriteError())
    {
        Serial.println("WriteError reported");
        Wire.clearWriteError();
    }
    printBuffer(buffer,rslt);
}

void wireReceive(int len)
{
    // slave has received data
    Serial.print("Data received, ");
    Serial.print("length = " + String(len));
    size_t reclen = Wire.available();
    Serial.print(", available = " + String(reclen));
    Serial.println(" bytes");
    if (reclen > MAX_LEN)
        reclen = MAX_LEN;
    size_t rslt = Wire.readBytes(buffer,reclen);
    printBuffer(buffer,rslt);
}

void printBuffer(byte buffer[],size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
    {
        Serial.print(hexString(buffer[i]));
        if (((i%16) == 15))
            Serial.print("\n");
        else    
            Serial.print(" ");
    }
    if  ((i%16) != 0)
        Serial.println();
}

void scanBus()
{
    // scans for i2c devices
    int deviceCount = 0;
    for (byte address = 0x1; address < 0x7f; address++)
    {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        // check status
        switch (error)
        {
        case WIRE_NOERROR:
            // found a responsive device
            if (deviceCount == 0)
                Serial.println("Found devices at addresses:");
            Serial.println(hexString(address));
            slaveAddress = address;
            deviceCount++;
            break;
        case WIRE_OTHER:
            Serial.print("Uknown error at ");
            Serial.println(hexString(address));
            break;
        default:
            // not present
            break;
        }
    }
    if (deviceCount == 0)
    {
        Serial.println("No devices found");
    }
}

void performTest()
{
    size_t sz;

    Serial.println("Will use 0x" + hexString(slaveAddress) + " as slave device");
    // initialize buffer
    for (sz = 0; sz < MAX_LEN; sz++)
        buffer[sz] = sz;
    
    // perform transmission tests
    for (sz = 1; sz <= MAX_LEN; sz *= 2)
    {
        Wire.beginTransmission(slaveAddress);
        //Wire.write(buffer,sz);
        for (size_t i = 0; i < sz; i++)
            Wire.write(buffer[i]);
        byte error = Wire.endTransmission();
        Serial.print("Write of " + String(sz) + " ");
        switch (error)
        {
        case WIRE_NOERROR:
            Serial.println("OK");
            break;
        case WIRE_TOOLONG:
            Serial.println("Write too long");
            break;
        case WIRE_NAKADDRESS:
            Serial.println("NAK on address");
            break;
        case WIRE_NAKDATA:
            Serial.println("NAK on data");
            break;
        default:
            Serial.println("Unknown error: " + hexString(error));
            break;
        }

        if (Wire.getWriteError())
        {
            Serial.println("WriteError reported");
            Wire.clearWriteError();
        }
    }

    // perform requestFrom test
    for (sz = 1; sz <= MAX_LEN; sz *= 2)
    {
        size_t rslt = Wire.requestFrom(slaveAddress,sz);
        Serial.println("Requestfrom for " + String(sz) + " = " + String(rslt));
        Wire.readBytes(buffer,rslt);
        printBuffer(buffer,rslt);
    }
}

static char hexDigits[] = 
    {"0123456789abcdef"};

String hexString(byte b)
{
    return(String(hexDigits[b>>4]) + String(hexDigits[b&0xf]));
}