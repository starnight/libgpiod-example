#include <atomic>
#include <iostream>
#include <chrono>
#include <future>
#include <gpiod.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <unistd.h>


/* constants */
static constexpr size_t PROG_DURATION_IN_SECONDS = 10;

/* Helpful banner for program usage */
void banner(char *progName)
{
    std::cout << std::string(progName) << std::endl;
    std::cout << "     -u/h  : Usage message " << std::endl;
    std::cout << "     -n    : Number of GPIO controller to scan, Default range [0,1)" << std::endl;
    std::cout << "     -l    : Line number to search in GPIO controllers, Default 0" << std::endl;
}

/* Asynchronous operator managing callbacks from libgpiod */
int async_monitor(const std::string devName, const unsigned int inLine, std::atomic<bool>& endFlag)
{

    bool eventTransition = false;
    const struct timespec monitorTimeout = {1, 0};
    auto poll_callback = [](unsigned int numLines, struct gpiod_ctxless_event_poll_fd* desc,
                            const struct timespec* timeOut, void* userData) -> int
    {
        if (numLines != 1) {
            std::cerr << "Invalid number of GPIO lines received" << numLines << std::endl;
            return GPIOD_CTXLESS_EVENT_POLL_RET_ERR;
        }

        if (not desc) {
            std::cerr << "Invalid file descriptor structure passed" << std::endl;
            return GPIOD_CTXLESS_EVENT_POLL_RET_ERR;
        }
        if (not timeOut) {
            std::cerr << "Invalid timeout structure passed" << std::endl;
            return GPIOD_CTXLESS_EVENT_POLL_RET_ERR;
        }

        if (timeOut->tv_sec != 1U or timeOut->tv_nsec != static_cast<uint64_t>(0)) {
            std::cerr << "Unexpected polling callback timestamp sec="
                      <<  static_cast<uint64_t>(timeOut->tv_sec)
                      << " nanosec=" << static_cast<uint64_t>(timeOut->tv_nsec) << std::endl;
            return GPIOD_CTXLESS_EVENT_POLL_RET_ERR;
        } else {
            /* No need to set file descriptor and corresponding event if not controlling GPIO */
            return GPIOD_CTXLESS_EVENT_POLL_RET_TIMEOUT;
        }
    };

    auto event_callback = [inLine, &eventTransition](int event, unsigned int lineIn, const struct timespec* timeOut,
                            void* data) -> int
    {
        if (event != GPIOD_CTXLESS_EVENT_RISING_EDGE and event != GPIOD_CTXLESS_EVENT_FALLING_EDGE) {
            std::cerr << "Unexpected GPIO event=" << event << " received" << std::endl;
            return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
        }
        if (not timeOut) {
            std::cerr << "Invalid timeout structure passed" << std::endl;
            return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
        }
        std::cout << "EventCallback timestamp: sec" << static_cast<uint64_t>(timeOut->tv_sec) <<
                     " nanosec=" << static_cast<uint64_t>(timeOut->tv_nsec) << std::endl;
        if (lineIn != inLine) {
            std::cerr << "Event received for unwanted GPIO line=" << lineIn << std::endl;
            return GPIOD_CTXLESS_EVENT_CB_RET_STOP;
        }

        eventTransition = event == GPIOD_CTXLESS_EVENT_RISING_EDGE ? true : false;
        return GPIOD_CTXLESS_EVENT_CB_RET_OK;
    };

    auto event_callback_wrapper = [](int event, unsigned int lineIn, const struct timespec* timeOut, void* data)-> int {
        return (*static_cast<decltype(event_callback)*>(data))(event, lineIn, timeOut, NULL);
    };

    auto retVal = gpiod_ctxless_event_monitor(devName.c_str(),
                                              GPIOD_CTXLESS_EVENT_RISING_EDGE,
                                              inLine,
                                              false,
                                              "LSMF",
                                              &monitorTimeout,
                                              poll_callback,
                                              event_callback_wrapper,
                                              &event_callback);
    if (retVal != 0) {
        std::cerr << "Event monitoring for GPIO Line=" << inLine << " Failed! with error=" << errno << std::endl;
        return retVal;
    } else {
        std::cout << "Callbacks successfully registered, this thread will just wait now for callbacks to take over" << std::endl;
        while(not endFlag) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (eventTransition) {
                std::cout << "GPIO event registered!" << std::endl;
            }
        }
        return 0;
    }
}

/* libgpiod client driver for demonstrating context less event loop */
int main(int argc, char **argv)
{

    int optIn = 0;
    int numDevices = -1;
    unsigned int numLine = 0;

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
                    std::cerr << "Option -" << optopt << " supplied but number of devices not given" << std::endl;
                } else if (isprint(optopt)) {
                    std::cerr << "Unsupported option -"<< optopt << " supplied" << std::endl;
                } else {
                    std::cerr << "Unknown option character 0x" << std::hex << optopt << std::endl;
                    return -1;
                }
            default:
                banner(argv[0]);
                return -1;
        }
    }

    if (numDevices == -1) {
        std::cout << "Number of controllers not supplied, default value 1 will be used" << std::endl;
        numDevices = 1;
    }

    if (numLine == 0) {
        std::cout << "GPIO Line number not specified, default value zero will be used" << std::endl;
    }
    std::cout << std::string(argv[0]) << ": GPIO controllers=" << numDevices <<" GPIO Line=" << numLine << std::endl;

    int chipIndex;
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    std::string gpio_device;
    bool lineFound = false;
    for (chipIndex = 0; chipIndex < numDevices; chipIndex++) {
        chip = gpiod_chip_open_by_number(chipIndex);
        if (!chip) {
            std::cerr << "Opening GPIO controller=" << chipIndex << " failed with error=" << errno << std::endl;
            continue;
        } else {
            gpio_device = "/dev/" + std::string(gpiod_chip_name(chip));
            line = gpiod_chip_get_line(chip, numLine);
            if (!line) {
                std::cerr << "GPIO Line=" << numLine << " not found in controller=" << gpio_device << std::endl;
            } else {
                std::cout << "GPIO Line=" << numLine << " found within controller=" << gpio_device << std::endl;
                lineFound = true;
                gpiod_line_release(line);
                gpiod_chip_close(chip);
                break;
            }

            gpiod_chip_close(chip);
        }
    }

    if (lineFound) {
        if (not gpiod_line_is_free(line)) {
            std::cerr << "GPIO line with num=" << numLine << " is already under use!" << std::endl;
            return -1;
        }

        std::atomic<bool> endFlag {false};
        std::future<int> monitorF = std::async(std::launch::async, &async_monitor, gpio_device.c_str(), numLine, std::ref(endFlag));
        if (monitorF.valid()) {
            auto status = monitorF.wait_for(std::chrono::seconds(PROG_DURATION_IN_SECONDS));
            if (status == std::future_status::timeout) {
                std::cout << "Time to end the program successfully!" << std::endl;
                endFlag = true;
                return 0;
            }
        }

        /* Purposely not retrieving future.get() since the normal case is ending the program after
         a defined duration */

    } else {
       std::cerr << "GPIO Line=" << numLine << " not found in any GPIO controller" << std::endl;
    }

    return -1;
}
