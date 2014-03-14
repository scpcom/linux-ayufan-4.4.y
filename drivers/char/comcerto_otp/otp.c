#include <linux/delay.h>
#include <linux/io.h>

#include <mach/comcerto-2000/otp.h>

/* Taken from repo://barebox/mindspeed/drivers/otp/c2k_otp.c */
int otp_read(u32 offset, u8 *read_data, int size)
{
	int i;
	u32 read_tmp = 0;

	if (NULL == read_data)
		return 1;

	if (size <= 0)
		return 1;

	/* configure the OTP_DATA_OUT_COUNTER for read operation.
			70 nsec is needed except for blank check test, in which 1.5 usec is needed.*/
	writel(DOUT_COUNTER_VALUE, OTP_DATA_OUT_COUNTER);

	/* Unlock the write protection. */
	writel(0xEBCF0000, OTP_CONFIG_LOCK_0); /* config lock0 */
	writel(0xEBCF1111, OTP_CONFIG_LOCK_1); /* config lock1 */
	writel(0x0, OTP_CEB_INPUT);

	/* rstb drive 0 */
	writel(0x0, OTP_RSTB_INPUT);
	/* Wait for at least 20nsec */
	udelay(OTP_DELAY);
	/* rstb drive 1 to have pulse */
	writel(0x1, OTP_RSTB_INPUT);
	/* Wait for at least 1usec */
	udelay(OTP_DELAY);

	/* Write the desired address to the ADDR register */
	writel(offset, OTP_ADDR_INPUT);
	/* read_enable drive */
	writel(0x1, OTP_READEN_INPUT);
	/* Wait for at least 70nsec/1.5usec depends on operation type */
	udelay(OTP_DELAY);

	/* Read First Byte */
	read_tmp = readl(OTP_DATA_OUTPUT);
	*read_data = read_tmp & 0xFF;

	/* For consecutive read */
	for(i = 1 ; i < size ; i++)
	{
		offset = offset + 8;

		/* start reading from data out register */
		writel(offset, OTP_ADDR_INPUT);
		/* Wait for at least 70nsec/1.5usec depends on operation type */
		udelay(OTP_DELAY);

		read_tmp = readl(OTP_DATA_OUTPUT);
		*(read_data + i) = read_tmp & 0xFF;
	}

	/* reading is done make the read_enable low */
	writel(0x0, OTP_READEN_INPUT);

	/* lock CEB register, return to standby mode */
	writel(0x1, OTP_CEB_INPUT);

	return 1;
}

