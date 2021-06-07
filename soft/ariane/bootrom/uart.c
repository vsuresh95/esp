#include <uart.h>
#include <00_systest_helper.h>

extern uint64_t amo_swap (volatile uint64_t* handshake, uint64_t value);
extern volatile uint64_t read_dword_fcs (volatile uint64_t* dst, bool dcs_en, bool owner_pred);

void write_reg_u32(uintptr_t addr, uint32_t value)
{
    volatile uint32_t *loc_addr = (volatile uint32_t *)addr;
    *loc_addr = (uint32_t) value;
}

uint32_t read_reg_u32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

int is_transmit_empty()
{
    return read_reg_u32(UART_STATUS) & UART_STATUS_TE;
}

void wait_uart_tx()
{
	while (is_transmit_empty() == 0) {
		/* asm volatile ("nop"); */
		/* asm volatile ("nop"); */
		/* asm volatile ("nop"); */
		/* asm volatile ("nop"); */
		/* asm volatile ("nop"); */
		/* asm volatile ("nop"); */
		/* asm volatile ("nop"); */
		/* asm volatile ("nop"); */
	};

}

void write_serial(char a)
{

	if (a == '\n') {
		/* while (is_transmit_empty() == 0) {}; */
		/* write_reg_u32(UART_DATA, 0x0d); */
		wait_uart_tx();
		write_reg_u32(UART_DATA, 0x0a);
		wait_uart_tx();
		write_reg_u32(UART_DATA, 0x0d);
		wait_uart_tx();
	} else {
		wait_uart_tx();
		write_reg_u32(UART_DATA, a);
		wait_uart_tx();
	}
}

void print_uart(const char *str)
{
	uint64_t old_val;
	volatile uint64_t* lock = (volatile uint64_t*) 0x90080000;

	// acquire the lock
	while (1) {
		// check if lock is set
		if (read_dword_fcs(lock, false, false) != 1) {
			// try to set lock
			old_val = amo_swap (lock, 1); 

			// check if lock was set
			if (old_val != 1){
				break;
			}
		}
	}

    const char *cur = &str[0];
    while (*cur != '\0')
    {
        write_serial((uint8_t)*cur);
        ++cur;
    }

    // release the lock
    old_val = amo_swap (lock, 0);
}

void init_uart()
{
	const unsigned scaler = BASE_FREQ / (38400 * 8 + 7);
	write_reg_u32(UART_SCALER, scaler);
	write_reg_u32(UART_CONTROL, UART_CTRL_FA | UART_CTRL_TE | UART_CTRL_RE);

	print_uart("Hello ESP\n");
}

uint8_t bin_to_hex_table[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void bin_to_hex(uint8_t inp, uint8_t res[2])
{
    res[1] = bin_to_hex_table[inp & 0xf];
    res[0] = bin_to_hex_table[(inp >> 4) & 0xf];
    return;
}

void print_uart_int(uint32_t data)
{
    int i;
    for (i = 3; i > -1; i--)
    {
        uint8_t cur = (data >> (i * 8)) & 0xff;
        uint8_t hex[2];
        bin_to_hex(cur, hex);
        write_serial(hex[0]);
        write_serial(hex[1]);
    }
}

void print_uart_int64(uint64_t data)
{
    int i;
    for (i = 7; i > -1; i--)
    {
        uint8_t cur = (data >> (i * 8)) & 0xff;
        uint8_t hex[2];
        bin_to_hex(cur, hex);
        write_serial(hex[0]);
        write_serial(hex[1]);
    }
}

void print_uart_addr(uint64_t addr)
{
    int i;
    for (i = 7; i > -1; i--)
    {
        uint8_t cur = (addr >> (i * 8)) & 0xff;
        uint8_t hex[2];
        bin_to_hex(cur, hex);
        write_serial(hex[0]);
        write_serial(hex[1]);
    }
}

void print_uart_byte(uint8_t byte)
{
    uint8_t hex[2];
    bin_to_hex(byte, hex);
    write_serial(hex[0]);
    write_serial(hex[1]);
}
