#define F_CPU 8000000UL  // 8 MHz internal oscillator
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>  // For itoa (integer to ASCII conversion)
#include <stdio.h>   // For sprintf

#define F_SCL 100000UL  // I2C SCL frequency (100 kHz)
#define BMP280_ADDR 0x76  // BMP280 I2C address
char buffer[30];  // Increased buffer size

void float_to_string(float num, char* buffer, int precision) {
	int whole_part = (int)num;
	float fractional_part = num - whole_part;

	// Convert whole part to string
	itoa(whole_part, buffer, 10);  // Convert the integer part
	char* ptr = buffer + strlen(buffer);  // Move pointer to the end of the string

	*ptr++ = '.';  // Add decimal point

	// Convert fractional part to string
	for (int i = 0; i < precision; i++) {
		fractional_part *= 10;
		whole_part = (int)fractional_part;
		*ptr++ = whole_part + '0';  // Convert to character
		fractional_part -= whole_part;
	}
	*ptr = '\0';  // Null-terminate the string
}

// UART functions
void uart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8 data bits, 1 stop bit
}

void uart_transmit(unsigned char data) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

void uart_send_string(const char* str) {
	while (*str) {
		uart_transmit(*str++);
	}
}

char uart_receive(void) {
    while (!(UCSR0A & (1 << RXC0)));  // Wait for data to be received
    return UDR0;  // Return received data
}

// I2C functions
void i2c_init(void) {
	TWSR = 0x00;  // Prescaler = 1
	TWBR = ((F_CPU / F_SCL) - 16) / 2;  // Set I2C clock rate to 100kHz
}

void i2c_start(void) {
	TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));  // Wait for TWINT to be set
}

void i2c_stop(void) {
	TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);
}

void i2c_write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));  // Wait for TWINT to be set
}

uint8_t i2c_read_ack(void) {
	TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

uint8_t i2c_read_nack(void) {
	TWCR = (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

void i2c_start_transmission(uint8_t address) {
	i2c_start();
	i2c_write(address);
}

// BMP280 functions
void bmp280_init(void) {
	i2c_start_transmission(BMP280_ADDR << 1);  // Write mode
	i2c_write(0xF4);  // Control register
	i2c_write(0x27);  // Normal mode, temp and pressure oversampling 1x
	i2c_stop();
}

float bmp280_read_temperature(void) {
	uint16_t temp = 0;

	i2c_start_transmission(BMP280_ADDR << 1);  // Write mode
	i2c_write(0xFA);  // Temperature MSB register
	i2c_stop();

	i2c_start_transmission((BMP280_ADDR << 1) | 1);  // Read mode
	temp = (i2c_read_ack() << 8);  // Read MSB
	temp |= i2c_read_nack();  // Read LSB
	i2c_stop();

	float tempx = temp / 1200.0; 
	
	float_to_string(tempx, buffer, 2);  // Convert with 2 decimal places
	uart_send_string("Temperature:");
	uart_send_string(buffer);
	uart_send_string(" \r\n");
	
	return tempx;
}

float bmp280_read_pressure(void) {
	
	uint16_t pressure = 0;

	i2c_start_transmission(BMP280_ADDR << 1);  // Write mode
	i2c_write(0xF7);  // Pressure MSB register
	i2c_stop();

	i2c_start_transmission((BMP280_ADDR << 1) | 1);  // Read mode
	pressure = (i2c_read_ack() << 8);  // Read MSB
	pressure |= i2c_read_nack();  // Read LSB
	i2c_stop();
	
	float real_pressure = 1013 + ((pressure / 25.3 )/ 1013 ) ;
	
	// Send pressure
	float_to_string(real_pressure, buffer, 2);
	uart_send_string("humidity_pressure_sensor:");
	uart_send_string(buffer);
	uart_send_string("\r\n");

	return real_pressure;
}

void adc_init(void) {
	// Set the ADC reference to AVcc (5V)
	ADMUX = (1 << REFS0);  // AVcc as reference

	// Enable the ADC, set the prescaler to 64 for 125kHz ADC clock at 8 MHz CPU
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint16_t adc_read(uint8_t channel) {
	// Select the appropriate ADC channel (0 for LDR, 1 for Vibration sensor, 2 for MQ135)
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);  // Set the lower bits of ADMUX to the channel number

	// Start the ADC conversion
	ADCSRA |= (1 << ADSC);

	// Wait for the conversion to complete
	while (ADCSRA & (1 << ADSC));

	// Return the ADC value (10-bit result)
	return ADC;
}
uint16_t adc_get_data(){
			adc_init();
			char buffer[10];
			
	 		// Read LDR from PC0 (ADC0)
	 		uint16_t ldr_value = adc_read(0);  // Select ADC0 (channel 0)
	 		itoa(ldr_value, buffer, 10);
	 		uart_send_string("light_sensor:");
	 		uart_send_string(buffer);
	 		uart_send_string("\r\n");

	 		// Read Vibration Sensor from PC1 (ADC1)
	 		uint16_t vibration_value = adc_read(1);  // Select ADC1 (channel 1)
	 		itoa(vibration_value, buffer, 10);
	 		uart_send_string("vibration_sensor:");
	 		uart_send_string(buffer);
	 		uart_send_string("\r\n");

	 		// Read MQ135 Gas Sensor from PC2 (ADC2)
	 		uint16_t gas_value = adc_read(2);  // Select ADC2 (channel 2)
	 		itoa(gas_value, buffer, 10);
	 		uart_send_string("gas_sensor_1:");
	 		uart_send_string(buffer);
	 		uart_send_string("\r\n");
			 
			 return ldr_value ,vibration_value, gas_value;
 }

// Function to handle UART commands
void handle_uart_command(const char* command) {
    if (strcmp(command, "ACTIVATE_PD3") == 0) {
        PORTD |= (1 << PD3);  // Set PD3 high
        uart_send_string("PD3 activated\r\n");
    } else if (strcmp(command, "DEACTIVATE_PD3") == 0) {
        PORTD &= ~(1 << PD3);  // Set PD3 low
        uart_send_string("PD3 deactivated\r\n");
    } else if (strcmp(command, "ACTIVATE_PD4") == 0) {
        PORTD |= (1 << PD4);  // Set PD4 high
        uart_send_string("PD4 activated\r\n");
    } else if (strcmp(command, "DEACTIVATE_PD4") == 0) {
        PORTD &= ~(1 << PD4);  // Set PD4 low
        uart_send_string("PD4 deactivated\r\n");
    } else if (strcmp(command, "ACTIVATE_PD5") == 0) {
        PORTD |= (1 << PD5);  // Set PD5 high
        uart_send_string("PD5 activated\r\n");
    } else if (strcmp(command, "DEACTIVATE_PD5") == 0) {
        PORTD &= ~(1 << PD5);  // Set PD5 low
        uart_send_string("PD5 deactivated\r\n");
    } else if (strcmp(command, "ACTIVATE_PD6") == 0) {
        PORTD |= (1 << PD6);  // Set PD6 high
        uart_send_string("PD6 activated\r\n");
    } else if (strcmp(command, "DEACTIVATE_PD6") == 0) {
        PORTD &= ~(1 << PD6);  // Set PD6 low
        uart_send_string("PD6 deactivated\r\n");
    } else if (strcmp(command, "ACTIVATE_PD7") == 0) {
        PORTD |= (1 << PD7);  // Set PD7 high
        uart_send_string("PD7 activated\r\n");
    } else if (strcmp(command, "DEACTIVATE_PD7") == 0) {
        PORTD &= ~(1 << PD7);  // Set PD7 low
        uart_send_string("PD7 deactivated\r\n");
    } else {
        uart_send_string("Unknown command\r\n");
    }
}

int main(void) {
    // Initialize UART, I2C, and ADC
    uart_init(MYUBRR);
    i2c_init();
    bmp280_init();

    // Set PD3 to PD7 as output
    DDRD |= (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7);

    char command_buffer[30];
    uint8_t command_index = 0;

    while (1) {
        // Read and store UART input into a buffer
        char received_char = uart_receive();
        if (received_char == '\n' || received_char == '\r') {
            command_buffer[command_index] = '\0';  // Null-terminate the string
            handle_uart_command(command_buffer);  // Process the command
            command_index = 0;  // Reset buffer index
        } else if (command_index < sizeof(command_buffer) - 1) {
            command_buffer[command_index++] = received_char;
        }

        // Continue with other tasks (e.g., sensor readings)
        float temp = bmp280_read_temperature();
        float pressure = bmp280_read_pressure();
        adc_get_data();
        _delay_ms(1000);  // Delay 1 second between readings
    }
}
