#include <RS485.h>  // RS485 definitions and types


// Read multiple registers with sampling, filtering and averaging
 RS485_STATUS read_Registers( HardwareSerial &serial, uint8_t deviceAddress, uint16_t startAddress, uint8_t sampleCount, uint16_t *results ) // read registers; sample count controls repeated reads
{
    if ( sampleCount == 0 || sampleCount > MAX_SAMPLES ) // validate sample count
        return INVALID_PARAM; // invalid parameter

    uint16_t samples[MAX_SAMPLES][NUM_REGISTERS];  // store raw samples per register

    // -------------------------------
    // Collect samples
    // -------------------------------
    for ( uint8_t s = 0; s < sampleCount; s++ )  // collect each sample
    {

        RS485_STATUS status = read_Registers_raw( serial, deviceAddress, startAddress, NUM_REGISTERS, samples[s] );  // raw read into sample slot

        if ( status != RS485_GOOD ) // check read status
            return status; // propagate error

        delay( 50 );   // settling time between reads
    } // end sample collection

    // -------------------------------
    // Filter and average per register
    // -------------------------------
    for ( uint8_t r = 0; r < NUM_REGISTERS; r++ )  // per-register processing
    {

        float sum = 0.0f;  // accumulated sum for register

        for ( uint8_t s = 0; s < sampleCount; s++ ) // sum samples for this register
            sum += samples[s][r];  // accumulate sample

        float avg = sum / sampleCount;  // compute average
        float lower = avg * ( 1.0f - FILTER_PCT / 100.0f );  // lower threshold after filter
        float upper = avg * ( 1.0f + FILTER_PCT / 100.0f );  // upper threshold after filter

        float filteredSum = 0.0f;  // sum after filtering
        uint8_t count = 0;  // count of values within threshold

        for ( uint8_t s = 0; s < sampleCount; s++ ) // apply threshold filter
        {
            if ( samples[s][r] >= lower && samples[s][r] <= upper ) // within accepted range
            {
                filteredSum += samples[s][r]; // include value in filtered sum
                count++; // increment accepted count
            }
        } // end filtering loop

        // Fallback if all rejected
        if ( count == 0 )
            results[r] = uint16_t(avg);  // use average as fallback
        else
            results[r] = uint16_t( filteredSum / count );  // averaged filtered value
    } // end per-register processing

    return RS485_GOOD; // all registers read successfully
}



// Function to read registers from the RS485 soil sensor (raw Modbus RTU frame)
RS485_STATUS read_Registers_raw( HardwareSerial &serial, uint8_t deviceAddress, uint16_t startAddress, uint16_t numRegisters, uint16_t *results ) // low-level Modbus read
{
    
    // Modbus RTU request: [Device Addr][Function][Start Addr Hi][Start Addr Lo][Qty Hi][Qty Lo][CRC Lo][CRC Hi]

    uint8_t request[FRAME_LEN];  // build request frame

    request[ADDR] = deviceAddress;  // device address byte
    request[FUNC] = 0x03; // Function code for Read Holding Registers
    request[STRT_ADDR_HI] = highByte(startAddress);  // start addr hi byte
    request[STRT_ADDR_LO] = lowByte(startAddress);  // start addr lo byte
    request[DATA_LEN_HI] = highByte(numRegisters);  // quantity hi byte
    request[DATA_LEN_LO] = lowByte(numRegisters);  // quantity lo byte

    // Calculate CRC
    uint16_t crc = calc_crc( request, CRC_HI );  // compute CRC over frame (excluding CRC bytes)
    request[CRC_HI] = crc & 0xFF;        // CRC Low byte
    request[CRC_LO] = (crc >> 8) & 0xFF; // CRC High byte

    // Send the request
    serial.write( request, FRAME_LEN );  // send interrogation frame over serial
    serial.flush();  // ensure all bytes sent

    // Get reply frame response
    uint8_t response[5 + numRegisters * 2]; // response buffer: addr + func + bytecount + data + crc

    uint8_t index = 0;  // response index

    unsigned long start = millis();  // Capture time when reply frame capture begins

    while ( ( millis() - start ) < 1000 && index < sizeof( response ) ) // One second timeout to receive all responses
    {

        if ( serial.available()  ) // check for available bytes
            response[index++] = serial.read();  // read next byte into buffer
        
    } // end receive loop

    // Validate response length
    if ( index < (5 + numRegisters * 2 ) )  // insufficient reply length
        return RESPONSE_INCOMPLETE; // reply didn't include expected bytes
    

    // Check CRC: compute CRC over all bytes except the last two CRC bytes
    uint16_t response_crc = calc_crc( response, index - 2 );  // CRC computed over message (excluding last two CRC bytes)

    uint16_t received_crc = ( response[index - 1] << 8 ) | response[index - 2];  // Received CRC assembled as (Hi<<8) | Lo

    if ( response_crc != received_crc ) // Compare CRCs
        return CRC_MISMATCH; // CRC mismatch indicates corrupted frame
    

    // Parse register values: assemble high/low bytes to 16-bit registers
    for ( uint16_t i = 0; i < numRegisters; i++ ) // for each register in response
        results[i] = ( response[3 + i * 2] << 8 ) | response[4 + i * 2];  // Assemble register value from two bytes (Hi<<8 | Lo)
    

    return RS485_GOOD; // raw read succeeded
}


// Compute Modbus RTU CRC16 (polynomial 0xA001), return 16-bit CRC value
uint16_t calc_crc( uint8_t *data, uint8_t length ) // CRC-16 (modbus) calculation
{

    uint16_t crc = 0xFFFF; // initial CRC value
    
    for ( uint8_t i = 0; i < length; i++ ) // iterate bytes
    {
        crc ^= data[i]; // incorporate byte into CRC
        
        for ( uint8_t j = 0; j < 8; j++ ) // iterate bits
        {
            if ( crc & 0x0001 ) // LSB set
            {
                crc >>= 1; // shift right
                crc ^= 0xA001; // apply polynomial
            }
            else
                crc >>= 1; // just shift right
            
        } // end bit loop
    } // end byte loop

    return crc; // return computed CRC
}