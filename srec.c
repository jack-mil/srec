#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

// ANSI formatting escape codes
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define PURPLE "\033[1;35m"
#define ITALIC "\033[4;3m"
#define RESET "\033[0m"
#define END "\033[0m\n"

typedef unsigned char BYTE;

bool parse_file(FILE *fp);
bool parse_line(unsigned lineNum, const char *line);
bool get_byte(const char **s, BYTE *nb);
bool get_nybble(const char **s, BYTE *nb);
void printArray(BYTE *__buf, size_t size);

int main(int argc, char *argv[]) {
	if ( argc < 2 ) {
		fprintf(stderr, RED "Usage: ./srec FILE" END);
		return 1;
	}
	FILE *fp = fopen(argv[1], "r");
	if ( fp == NULL ) {
		fprintf(stderr, RED "Cannot open input file \"%s\"" END, argv[1]);
		return 1;
	}

	bool status = parse_file(fp);
	fclose(fp);
	return !status;
}

/**
 *  Parses the S-Record file pointed by @a fp.
 *
 *  @param  fp  pointer to file stream
 *
 *  @return  true if parsing was successfully, false otherwise
 */
bool parse_file(FILE *fp) {

	unsigned lnNum = 0;
	char line[100];
	// Read and process each line of the file
	// `line` string will be newline-terminated
	while ( fgets(line, sizeof(line), fp) != NULL ) {
		lnNum++;
		printf(BLUE "-- Line %02d --" RESET "\n", lnNum);
		if ( !parse_line(lnNum, line) ) {
			fprintf(stderr, RED "Error in parsing line" END);
			return false;
		}
	}
	// If we made it this far, record is valid!
	return true;
}

/**
 * Parse a single line of an S record file.
 *
 * @param  lineNum  line being parsed (for error reporting)
 * @param  line  string to be parsed
 *
 * @return  true is parsing was succesful, false otherwise
 */
bool parse_line(unsigned lineNum, const char *line) {
	// data byte array will store the address and data bytes of the record
	// (used for address and checksum calculation)
	BYTE data[50];

	// First check for leading 'S'
	if ( line[0] != 'S' ) {
		fprintf(stderr, RED "Line %d: Not a valid S-Record (first char not 'S')" END, lineNum);
		return false;
	}

	// Check record type is valid
	if ( !isdigit(line[1]) ) {
		fprintf(stderr, RED "Line %d: Unsupported record type, found : '%c'" END, lineNum, line[1]);
		return false;
	}

	// Make a new string skipping the first 2 characters ("S" and type)
	const char *s = &line[2];

	// Get the length of this S-Record line
	BYTE byteCount;
	if ( !get_byte(&s, &byteCount) ) {
		return false;
	}

	// Calculate checksum of all *plain-text* bytes in record
	// Also stores the data array with bytes starting at address record
	BYTE checksumCalc = byteCount;
	for ( int i = 0; i < (byteCount - 1); i++ ) {
		if ( !get_byte(&s, &data[i]) ) {
			return false;
		}
		checksumCalc += data[i];
	}
	// Checksum is the ones compliment of the sum of all 2-char bytes
	checksumCalc = ~checksumCalc;

	// Read checksum from last byte of record and validate
	BYTE checksumRead;
	if ( !get_byte(&s, &checksumRead) ) {
		return false;
	}

	// Parse the record type to determine the correct address byte length
	char rcType = line[1];
	unsigned addr = 0;
	unsigned addrLen;

	switch ( rcType ) {
		case '0':
			addrLen = 2;
			break;
		case '1':
		case '2':
		case '3':
			// Address length can be calculated from ascii codes
			addrLen = rcType - '1' + 2;
			break;
		case '7':
		case '8':
		case '9':
			// Address length can be calculated from ascii codes
			addrLen = '9' - rcType + 2;
			break;
		default:
			printf(RED "Unsupported S record type detected '%c'" END, line[1]);
			return false;
			break;
	}

	// Calculate the length of actual data bytes.
	// value from record minus the address length and checksum byte
	BYTE dataLen = byteCount - addrLen - 1;

	// Use a byte pointer instead of byte array so we can manipulate the size
	BYTE *x = data;
	for ( unsigned addrIndex = 0; addrIndex < addrLen; addrIndex++ ) {
		// Form a integer address value from the address bytes,
		// shifting first (so only MSB(s) get shifted)  by 8 (one byte)
		addr = addr << 8;
		// Add the byte to the address, and move pointer to next byte.
		addr += *x++;
	}

	// Print formated output
	printf("Type: %c; %d byte address\n", rcType, addrLen);
	printf("Address: " PURPLE "0x%0*X", addrLen * 2, addr); // Pad address so length is clear
	if ( dataLen > 0 ) {
		printf(" - 0x%0*X", addrLen * 2, addr + (dataLen - 1)); // print ending address of data location
	}
	puts(RESET); // puts includes a /n newline
	printf("Record checksum " ITALIC "0x%02X" RESET ": ", checksumRead);
	if ( checksumRead != checksumCalc ) {
		printf(RED "❌ Fail (calculated " ITALIC "0x%02X" RESET RED ")" END, checksumCalc);
	} else {
		printf(GREEN "✔ Good" END);
	}
	printf("Data: " YELLOW);
	printArray(x, dataLen);
	puts(END);
	return true;
}

/**
 * Read 2 hex character from a string @a s and convert 
 * to single numerical byte, stored in b.
 *
 * @param  s  pointer to a string to read from
 * @param  b  pointer to a byte to store value in
 *
 * @return  true if reading was succesful, false otherwise
 */
bool get_byte(const char **s, BYTE *b) {
	BYTE low_nibble, hi_nibble;
	// Read 2 nibbles (singe ascii characters '0'-'9', 'A'-'F')
	// Shift the high nibble and concatinate
	if ( get_nybble(s, &hi_nibble) && get_nybble(s, &low_nibble) ) {
		*b = hi_nibble << 4 | low_nibble;
		return true;
	}
	return false;
}

/**
 * Read a single character from a string @a s and convert 
 * to number, stored in b.
 *
 * @param  s  pointer to a string to read from
 * @param  b  pointer to a byte to store value in
 *
 * @return  true if reading was succesful, false otherwise
 */
bool get_nybble(const char **s, BYTE *nb) {
	// Get the first character in the current string
	// by dereferencing the pointer to a pointer to a character
	char ch = **s;

	// Increment the char pointer so the string is consumed
	*s = *s + 1;

	// Convert the hex digit character to numbers, store in byte `b`
	if ( (ch >= '0') && (ch <= '9') ) {
		*nb = ch - '0';
		return true;
	}

	if ( (ch >= 'A') && (ch <= 'F') ) {
		*nb = ch - 'A' + 10;
		return true;
	}

	if ( (ch >= 'a') && (ch <= 'f') ) {
		*nb = ch - 'a' + 10;
		return true;
	}
	fprintf(stderr, RED "Unexpected character found, '%c'" RESET "\n", ch);
	return false;
}

/**
 * Print an array of bytes to screen
 *
 * @param  buf  pointer to array of bytes to print
 * @param  size  number of bytes to print
 */
void printArray(BYTE *buf, size_t size) {

	if ( size == 0 ) {
		printf("--");
	}
	for ( size_t i = 0; i < size; i++ ) {
		printf("0x%02X ", *buf++);
	}
}