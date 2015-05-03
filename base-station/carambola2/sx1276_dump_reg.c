#define _BSD_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/types.h>
#include <linux/spi/spidev.h>

static void dumpstat(const char *name, int fd)
{
	__u8	lsb, bits;
	__u32	mode, speed;

	if (ioctl(fd, SPI_IOC_RD_MODE32, &mode) < 0) {
		perror("SPI rd_mode");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
		perror("SPI rd_lsb_fist");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
		perror("SPI bits_per_word");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
		perror("SPI max_speed_hz");
		return;
	}

	printf("%s: spi mode 0x%x, %d bits %sper word, %d Hz max\n",
		name, mode, bits, lsb ? "(lsb first) " : "", speed);
}

static void dumpregs(int fd)
{
	struct spi_ioc_transfer	xfer[2];
	uint8_t regval[0x71];
	memset(regval, 0x5a, sizeof(regval));

	for (int r=0; r <= 0x70; r++ ) {
		uint8_t buf[4];
		memset(xfer, 0, sizeof(xfer));
		memset(buf, 0, sizeof(buf));

		buf[0] = r;
		xfer[0].tx_buf = (unsigned long)buf;
		xfer[0].len = 1;

		xfer[1].rx_buf = (unsigned long)buf;
		xfer[1].len = 1;

		int	status = ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
		if (status < 0) { perror("SPI_IOC_MESSAGE"); return; }
		if (status != 2) { fprintf(stderr, "SPI status: %d at register %d\n", status, r); }
		regval[r] = buf[0];

		usleep(1000);
	}
  FILE *f = popen("od -Ax -tx1z -v -w8", "w"); // needs coreutils-od
  if (f) { fwrite(regval, 0x71, 1, f); pclose(f); }
	fflush(stdout);
	printf("\n");
}


int main(int argc, char **argv)
{
	if (argc < 2) { fprintf(stderr, "Need spi device\n"); return 1; }

	int fd = open(argv[1], O_RDWR);
	if (fd < 0) { perror("open"); return 1;	}

	dumpstat(argv[1], fd);

	dumpregs(fd);

	close(fd);
}

