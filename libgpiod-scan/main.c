#include <gpiod.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void banner(char *progName)
{
    printf("%s: \n", progName);
    printf("     -u/h  : Usage message \n");
    printf("     -n    : Number of GPIO controller to scan, Default range [0,1) \n");
    printf("     -l    : Line number to search in GPIO controllers, Default 0 \n");
}
/* libgpiod client to scan all possible GPIO controller for a given GPIO Line */
int main(int argc, char **argv)
{

    int optIn = 0;
    int numDevices = -1;
    int numLine = -1;

    /* Boilerplate command line arguments processing */
    while((optIn = getopt(argc, argv, "uhn:l:")) != EOF) {
        switch(optIn) {
            case 'u':
            case 'h':
                banner(argv[0]);
                return 0;
            case 'n':
                numDevices = atoi(optarg);
                break;
            case 'l':
                numLine = atoi(optarg);
                break;
            case '?':
                if (optopt == 'c') {
                    fprintf(stderr, "Option -%c supplied but number of devices not given.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unsupported option -%c supplied\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character 0x%x\n", optopt);
                    return -1;
                }
            default:
                banner(argv[0]);
                return -1;
        }
    }

    if (numDevices == -1) {
        printf("Number of controllers not supplied, default value 1 will be used\n");
        numDevices = 1;
    }

    if (numLine == -1) {
        printf("GPIO Line number not specified, default value zero will be used\n");
        numLine = 0;
    }
    printf("%s: started with GPIO controllers=%d to scan for GPIO Line=%d\n", argv[0], numDevices, numLine);

    int chipIndex;
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    bool lineFound = false;
    for (chipIndex = 0; chipIndex < numDevices; chipIndex++) {
        chip = gpiod_chip_open_by_number(chipIndex);
        if (!chip) {
            fprintf(stderr, "Opening GPIO controller=%d failed with error=%d\n", chipIndex, errno);
            continue;
        } else {
            line = gpiod_chip_get_line(chip, numLine);
            if (!line) {
                fprintf(stderr, "GPIO Line=%d not found in controller=%d\n", numLine, chipIndex);
            } else {
                printf("GPIO Line=%d found within controller=%d\n", numLine, chipIndex);
                /* Break as soon as GPIO line within a controller is found */
                lineFound = true;
                gpiod_line_release(line);
                gpiod_chip_close(chip);
                break;
            }

            gpiod_chip_close(chip);
        }
    }

    int retVal = lineFound ? 0 : -1;
    return retVal;
}
