#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

#ifndef	CONSUMER
#define	CONSUMER	"Consumer"
#endif

int main(int argc, char **argv)
{
	char *chipname = "gpiochip0";
	unsigned int line_num = 24;	// GPIO Pin #24
	int val;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int i, ret;

	chip = gpiod_chip_open_by_name(chipname);
	if (!chip) {
		perror("Open chip failed\n");
		goto end;
	}

	line = gpiod_chip_get_line(chip, line_num);
	if (!line) {
		perror("Get line failed\n");
		goto close_chip;
	}

	ret = gpiod_line_request_input(line, CONSUMER);
	if (ret < 0) {
		perror("Request line as input failed\n");
		goto release_line;
	}

	/* Read input 20 times */
	for (i = 20; i > 0; i--) {
		val = gpiod_line_get_value(line);
		if (val < 0) {
			perror("Read line input failed\n");
			goto release_line;
		}
		printf("Intput %d on line #%u\n", val, line_num);
		sleep(1);
	}

release_line:
	gpiod_line_release(line);
close_chip:
	gpiod_chip_close(chip);
end:
	return 0;
}
