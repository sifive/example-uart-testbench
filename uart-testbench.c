/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>

#include <metal/cpu.h>
#include <metal/interrupt.h>
#include <metal/uart.h>

/* This example assumes that the UART tx and rx are cross-connected
 *
 * It then performs the following steps:
 *   1. Register an interrupt handler to trigger when the receive FIFO is nonempty
 *   2. Write a character
 *   3. Check that the character read by the interrupt handler matches the written character
 */

volatile int c = -1;

void uart_int_handler(int id, void *priv) {
	struct metal_uart *uart = (struct metal_uart *)priv;
	metal_uart_getc(uart, (int *)&c);
}

int main(void)
{
	int rc = 0;

	/* Initialize CPU interrupt controller */
	struct metal_cpu *cpu = metal_cpu_get(metal_cpu_get_current_hartid());
	if (cpu == NULL) {
		return 1;
	}
	struct metal_interrupt *cpu_int = metal_cpu_interrupt_controller(cpu);
	if (cpu_int == NULL) {
		return 2;
	}
	metal_interrupt_init(cpu_int);

	/* Initialize UART device and interrupt controller */
	struct metal_uart *uart = metal_uart_get_device(0);
	if (uart == NULL) {
		return 1;
	}
	struct metal_interrupt *uart_int = metal_uart_interrupt_controller(uart);
	metal_interrupt_init(uart_int);
	int uart_id = metal_uart_get_interrupt_id(uart);

	/* Set the watermark to zero so that the interrupt triggers when the
	 * number of entries is 1 (strictly greater than 0) */
	rc = metal_uart_set_receive_watermark(uart, 0);
	if (rc != 0) {
		return 2;
	}
	rc = metal_uart_receive_interrupt_enable(uart);
	if (rc != 0) {
		return 3;
	}
	rc = metal_interrupt_register_handler(uart_int, uart_id, uart_int_handler, uart);
	if (rc != 0) {
		return 4;
	}

	/* Enable Interrupts */
	rc = metal_interrupt_enable(uart_int, uart_id);
	if (rc != 0) {
		return 5;
	}
	rc = metal_interrupt_enable(cpu_int, 0);
	if (rc != 0) {
		return 6;
	}

	rc = metal_uart_putc(uart, 'A');
	if (rc != 0) {
		return 7;
	}

	/* Wait for the interrupt handler to fire */
	while(c == -1) ;

	if (c != 'A') {
		return 8;
	}

	/* Disable interrupt and print out success message */
	rc = metal_uart_receive_interrupt_disable(uart);
	if (rc != 0) {
		return 9;
	}
	puts("UART testbench example PASSED");

	return 0;
}
