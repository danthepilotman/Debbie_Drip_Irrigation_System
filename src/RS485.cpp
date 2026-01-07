#include <RS485.h>


 RS485_STATUS read_Registers( HardwareSerial &serial, uint8_t deviceAddress, uint16_t startAddress, uint8_t sampleCount, uint16_t *results )
{
    if ( sampleCount == 0 || sampleCount > MAX_SAMPLES )
        return INVALID_PARAM;

    uint16_t samples[MAX_SAMPLES][NUM_REGISTERS];

    // -------------------------------
    // Collect samples
    // -------------------------------
    for ( uint8_t s = 0; s < sampleCount; s++ )
    {

        RS485_STATUS status = read_Registers_raw( serial, deviceAddress, startAddress, NUM_REGISTERS, samples[s] );

        if ( status != RS485_GOOD )
            return status;

        delay( 50 );   // settling time between reads
    }

    // -------------------------------
    // Filter and average per register
    // -------------------------------
    for ( uint8_t r = 0; r < NUM_REGISTERS; r++ )
    {

        float sum = 0.0f;

        for ( uint8_t s = 0; s < sampleCount; s++ )
            sum += samples[s][r];

        float avg = sum / sampleCount;
        float lower = avg * ( 1.0f - FILTER_PCT / 100.0f );
        float upper = avg * ( 1.0f + FILTER_PCT / 100.0f );

        float filteredSum = 0.0f;
        uint8_t count = 0;

        for ( uint8_t s = 0; s < sampleCount; s++ )
        {
            if ( samples[s][r] >= lower && samples[s][r] <= upper )
            {
                filteredSum += samples[s][r];
                count++;
            }
        }

        // Fallback if all rejected
        if ( count == 0 )
            results[r] = uint16_t(avg);
        else
            results[r] = uint16_t( filteredSum / count );
    }

    return RS485_GOOD;
}



// Function to read registers from the RS485 soil sensor
RS485_STATUS read_Registers_raw( HardwareSerial &serial, uint8_t deviceAddress, uint16_t startAddress, uint16_t numRegisters, uint16_t *results )
{
    
    // Modbus RTU request: [Device Addr][Function][Start Addr Hi][Start Addr Lo][Qty Hi][Qty Lo][CRC Lo][CRC Hi]

    uint8_t request[FRAME_LEN];

    request[ADDR] = deviceAddress;
    request[FUNC] = 0x03; // Function code for Read Holding Registers
    request[STRT_ADDR_HI] = highByte(startAddress);
    request[STRT_ADDR_LO] = lowByte(startAddress);
    request[DATA_LEN_HI] = highByte(numRegisters);
    request[DATA_LEN_LO] = lowByte(numRegisters);

    // Calculate CRC
    uint16_t crc = calc_crc( request, CRC_HI );
    request[CRC_HI] = crc & 0xFF;        // CRC Low
    request[CRC_LO] = (crc >> 8) & 0xFF; // CRC High

    // Send the request
    serial.write( request, FRAME_LEN );  // Seond interrogation frame
    serial.flush();

    // Get reply frame response
    uint8_t response[5 + numRegisters * 2]; // Address + Function + ByteCount + Data(2 / value) + CRC(2 hi/lo)

    uint8_t index = 0;

    unsigned long start = millis();  // Capture time when reply frame capture begins

    while ( ( millis() - start ) < 1000 && index < sizeof( response ) ) // One seconds timeout to receive all responses
    {

        if ( serial.available()  )
            response[index++] = serial.read();
        
    }

    // Validate response length
    if ( index < (5 + numRegisters * 2 ) )  // 5 reply frame registers are addr, func data len, crc hi crc lo
        return RESPONSE_INCOMPLETE;
    

    // Check CRC
    uint16_t response_crc = calc_crc( response, index - 2 );  // CEC is two bytes (hi & lo bytes)

    uint16_t received_crc = ( response[index - 1] << 8 ) | response[index - 2];  // hi byte shifted 8 to the left ORed with lo byte

    if ( response_crc != received_crc ) // Compare CRCs
        return CRC_MISMATCH;
    

    // Parse register values
    for ( uint16_t i = 0; i < numRegisters; i++ )
        results[i] = ( response[3 + i * 2] << 8 ) | response[4 + i * 2];  // Index 3 is the hi byte of the moisture value
    

    return RS485_GOOD;
}


// Example CRC calculation function for Modbus RTU
uint16_t calc_crc( uint8_t *data, uint8_t length )
{

    uint16_t crc = 0xFFFF;
    
    for ( uint8_t i = 0; i < length; i++ )
    {
        crc ^= data[i];
        
        for ( uint8_t j = 0; j < 8; j++ )
        {
            if ( crc & 0x0001 )
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
                crc >>= 1;
            
        }
    }

    return crc;
}