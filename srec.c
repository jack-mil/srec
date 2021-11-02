#include <stdio.h>
// #include <stdlib.h>

typedef unsigned char BYTE;

BYTE read_ascii_hex(FILE *__stream);
int read_address(FILE *__stream, int size);
void read_data(BYTE *__buf, FILE *__stream, size_t size);
// _Bool verify_checksum(BYTE *data, size_t size, int byte_count, int address, BYTE sx_sum);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: ./srec FILE\n");
		return 1;
	}
	FILE *fp = fopen(argv[1], "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open input file \"%s\"", argv[1]);
		return 1;
	}
	if (fgetc(fp) != 'S') {
		fprintf(stderr, "Line not a valid S-Record (first char not 'S')\n");
		return 1;
	}
	char mode = fgetc(fp);
	printf("Type: %c\n", mode);
	int byte_len = read_ascii_hex(fp);
	int data_len;
	printf("Length: %d (0x%02X)\n", byte_len, byte_len);

	int address;
	BYTE data[64];
	switch (mode) {
	// 16-bit address modes
	case '0':
		// Skip to next line
		// while (fgetc(fp) != 0x0A) {
		// }
	case '1':
	case '9':
		printf("16 bit address\n");
		address = read_address(fp, 2);
		data_len = byte_len - 3; // Adjust to length of data (len - address - checksum byte)
		break;
	// 24-bit address modes
	case '2':
	case '8':
		printf("24 bit address\n");
		address = read_address(fp, 3);
		data_len = byte_len - 4;
		break;
	// 32-bit address modes
	case '3':
	case '7':
		printf("32 bit address\n");
		address = read_address(fp, 4);
		data_len = byte_len - 5;
		break;
	default:
		printf("Unsupported S record type detected %c\n", mode);
		return 1;
		break;
	}

	printf("Address: 0x%X\n", address);
	read_data(data, fp, data_len);
	BYTE chk_sum = read_ascii_hex(fp);
	printf("Record checksum 0x%02X\n", chk_sum);
	// if (!verify_checksum(data, data_len, byte_len, address, chk_sum)) {
	// 	printf("Checksum Failed!\n");
	// }
	fclose(fp);
}

// _Bool verify_checksum(BYTE *data, size_t size, int byte_count, int address, BYTE sx_sum) {
// 	unsigned int sum = 0;
// 	for (size_t i = 0; i < size; i++) {
// 		sum += data[i];
// 	}
// 	BYTE x_sum = ~sum & 0xFF;
// 	printf("Calculated Checksum 0x%02X\n", x_sum);
// 	printf("Record checksum 0x%02X\n", sx_sum);
// 	return x_sum == sx_sum;
// }

BYTE read_ascii_hex(FILE *__stream) {
	unsigned char sixteens;

	unsigned char c = fgetc(__stream);
	if (c == EOF) {
		return EOF;
	}
	c = (c <= '9') ? (c - '0') : (c - 'A' + 10);
	sixteens = (c << 4);

	c = fgetc(__stream);
	if (c == EOF) {
		return EOF;
	}
	c = (c <= '9') ? (c - '0') : (c - 'A' + 10);
	return (sixteens + c);
}

int read_address(FILE *__stream, int size) {
	int address = 0;
	for (int i = size - 1; i >= 0; i--) {
		BYTE byte = read_ascii_hex(__stream);
		address += (byte << (8 << 8 * i));
	}
	return address;
}

void read_data(BYTE *__buf, FILE *__stream, size_t size) {
	printf("Data: ");
	for (size_t i = 0; i < size; i++) {
		BYTE byte = read_ascii_hex(__stream);
		printf("%02X ", byte);
		__buf[i] = byte;
	}
	printf("\n");
}