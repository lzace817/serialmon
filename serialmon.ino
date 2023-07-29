/* Serial monitor based on 6502 hex monitor (wozmon)
 */
#include <stdint.h>

#include <stdarg.h>
#define SERIAL_PRINTF_MAX_BUFF 256
void serialPrintf(const char *fmt, ...);
void serialPrintf(const char *fmt, ...)
{
	char buff[SERIAL_PRINTF_MAX_BUFF];

	va_list pargs;
	va_start(pargs, fmt);
	vsnprintf(buff, SERIAL_PRINTF_MAX_BUFF, fmt, pargs);
	va_end(pargs);
	Serial.print(buff);
}

// #define DEBUG_SLOW
#ifdef DEBUG_SLOW
// TODO(proto): make restart or infinite loop on assertion fail?
#define assert(exp) if(!(exp)) serialPrintf("%s:%d: Assertion failed\n", __FILE__, __LINE__)
#else
#define assert(...)
#endif

#define internal static
#define global_variable static

// typedef uint8_t bool8;

typedef enum Command_t {
	C_PRINT,
	C_PRINT_RANGE,
	C_STORE,
} Command;

#define SERIAL_BUFFER_CAP 128
struct Monitor_t {
	int inputBufferSize;
	char inputBuffer[SERIAL_BUFFER_CAP];
};
typedef struct Monitor_t Monitor;

internal inline int parseHexDigit(const char digit)
{
	if ('0' <= digit &&  digit <= '9') {
		return digit - '0';
	} else if ('a' <= digit &&  digit <= 'f') {
		return digit - 'a' + 10;
	} else if ('A' <= digit &&  digit <= 'F') {
		return digit - 'A' + 10;
	}
	return -1;
}

internal inline uint8_t getData(uint16_t addr)
{
	return *((int8_t *) addr);
}

internal inline void setData(uint16_t addr, uint8_t value)
{
	*((uint8_t *) addr) = value;
}

internal void startMonitor(Monitor *mon);
internal void startMonitor(Monitor *mon)
{
	mon->inputBufferSize = 0;
	serialPrintf("\\\n");
}

internal void parseBuffer(Monitor *mon);
internal void parseBuffer(Monitor *mon)
{
	int index = 0;
	char c = 0;

	uint16_t storeAddr = 0;
	uint16_t printAddr = 0;

	uint16_t number = 0;
	Command command = C_PRINT;

	assert(mon->inputBufferSize <= SERIAL_BUFFER_CAP);
	assert(mon->inputBuffer[mon->inputBufferSize-1] == '\n');

	while(index < SERIAL_BUFFER_CAP) {
		assert(index < mon->inputBufferSize);

		c = mon->inputBuffer[index];

		if (c == '\n')
			return;

		// blank
		if (isspace(c)) {
			index++;
			continue;
		}

		if (c == ':' || c == '=') {
			//wirte command
			command = C_STORE;
			index++;
			continue;
		}

		if (c == '.' || c == '-') {
			//print range
			command = C_PRINT_RANGE;
			index++;
			continue;
		}

		int startNumber = index;
		// first number
		number = 0;
		while (isxdigit(c)) {
			number = number << 4 | parseHexDigit(c);
			c = mon->inputBuffer[++index];
		}

		if(index == startNumber) {
			startMonitor(mon);
			return;
		}

		// execute command
		switch (command) {
		case C_PRINT: {
			printAddr = number;
			storeAddr = number;
			serialPrintf("\n%.4x:", printAddr);
			serialPrintf(" %.2x", getData(printAddr));
		} break;

		case C_PRINT_RANGE: {
			command = C_PRINT;
			while(printAddr < number) {
				printAddr++;
				if(printAddr % 8 == 0) {
					serialPrintf("\n%.4x:", printAddr);
				}
				serialPrintf(" %.2x", getData(printAddr));
			}
		} break;

		case C_STORE: {
			setData(storeAddr++, (uint8_t) number);
		} break;

		default: {
			assert(0 && "unreachable");
			startMonitor(mon);
			return;
		} break;
		}
	}

}

global_variable bool ledState;
global_variable Monitor monitor;

void setup() {
	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);
	startMonitor(&monitor);
}

void loop() {
	static long int timestamp;

	// NOTE(proto): volatile to try avoid optimization
	// so we can change it in the monitor
	static volatile uint16_t interval = 400;

	if (millis() - timestamp > interval) {
		digitalWrite(LED_BUILTIN, ledState);
		ledState = !ledState;
		timestamp = millis();
	}

	while(Serial.available()) {
		char c = Serial.read();

		if (monitor.inputBufferSize < SERIAL_BUFFER_CAP) {
			monitor.inputBuffer[monitor.inputBufferSize++] = c;
			if (c == '\n') {
				parseBuffer(&monitor);
				monitor.inputBufferSize = 0;
				serialPrintf("\n");
			}
		} else {
			startMonitor(&monitor);
		}
	}
}
