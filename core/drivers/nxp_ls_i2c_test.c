// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright 2020 NXP
 * Test suite for I2C Controller
 */

#include <kernel/tee_time.h>
#include <mm/core_memprot.h>
#include <drivers/nxp_ls_i2c.h>
#include <kernel/delay.h>
#include <string.h>

#define OFFSET_OF(type, field)		((uintptr_t)&(((type *)0)->field))
#define PCF2129_CTRL3_BIT_BLF		BIT(2)    /* Battery Low Flag*/
#define PCF2129_SLAVE_ADDRESS		0x51

#pragma pack(1)
typedef struct {
	uint8_t  control[3];
	uint8_t  seconds;
	uint8_t  minutes;
	uint8_t  hours;
	uint8_t  days;
	uint8_t  weekdays;
	uint8_t  months;
	uint8_t  years;
	uint8_t  second_alarm;
	uint8_t  minute_alarm;
	uint8_t  hour_alarm;
	uint8_t  day_alarm;
	uint8_t  weekday_alarm;
} pcf2129_regs;
#pragma pack()

// I2c clock based on 750Mhz platform clock
#define I2C_CLOCK	93750000
#define I2C_SPEED	100000

static TEE_Result i2c_test_suite(void)
{
	nxp_i2c_data i2c_data;

	TEE_Result status = TEE_ERROR_GENERIC;
	i2c_reg_request req;
	uint8_t                                rtc_reg_adr;
	pcf2129_regs	*pcf_regs;
	uint8_t retries = 5;

	pcf_regs = (pcf2129_regs *)malloc(sizeof(pcf2129_regs));

	EMSG("I2C RTC TEST Starts, will get time from RTC for 5 times after every 2 secs");

	/* set slave info */
	i2c_data.i2c_controller = CFG_I2C_CONTROLLER;
	i2c_data.i2c_bus_clock	= I2C_CLOCK;
	i2c_data.speed		= I2C_SPEED;

	/* Initialise DSPI driver */
	status = i2c_init(&i2c_data);
	if (status != TEE_SUCCESS) {
		EMSG("Unable to init I2C driver");
		goto exit;
	}

	while (retries--) {
		rtc_reg_adr = OFFSET_OF(pcf2129_regs, control[2]);

		req.operation_count = 1;
		req.operation[0].flags = 0;
		req.operation[0].length_in_bytes = sizeof(rtc_reg_adr);
		req.operation[0].buffer = &rtc_reg_adr;

		status = i2c_bus_xfer(i2c_data.base, PCF2129_SLAVE_ADDRESS,
					(void *)&req);
		if (status) {
			EMSG("RTC write error at Addr, Status = %x\n", status);
			goto exit;
		}

		req.operation_count = 1;
		req.operation[0].flags = I2C_FLAG_READ;
		req.operation[0].length_in_bytes = OFFSET_OF(pcf2129_regs, second_alarm) -
							OFFSET_OF(pcf2129_regs, control[2]);
		req.operation[0].buffer = &pcf_regs->control[2];

		status = i2c_bus_xfer(i2c_data.base, PCF2129_SLAVE_ADDRESS, (void *)&req);
		if (status) {
			EMSG("RTC read error at Addr, Status = %x\n", status);
			goto exit;
		}

		EMSG("Second = %u, minutes = %u, Hours = %u, days = %u weekdays = %u months = %u, years = %u\n",
				pcf_regs->seconds, pcf_regs->minutes, pcf_regs->hours, pcf_regs->days,
				pcf_regs->weekdays, pcf_regs->months, pcf_regs->years);

		if (pcf_regs->control[2] & PCF2129_CTRL3_BIT_BLF)
			EMSG("### Warning: RTC battery status low, check/replace RTC battery.\n");

		/* Add delay of 2 secs and then again get time from RTC */
		udelay(2000000);
		memset(pcf_regs, 0, sizeof(pcf2129_regs));
	}
exit:
	if (pcf_regs)
		free(pcf_regs);

	return status;
}

void i2c_test(void)
{
	TEE_Result status = TEE_ERROR_GENERIC;

	status = i2c_test_suite();
	if (status == TEE_SUCCESS)
		EMSG("Test passed");
	else
		EMSG("Test failed");
}
