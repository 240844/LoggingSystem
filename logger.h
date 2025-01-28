#ifndef LOGGER_H
#define LOGGER_H

typedef enum logger_level {
    min_log,
    standard_log,
    max_log
}logger_level_t;

#define FILENAME "log.txt"

//public functions for users

void save_log(logger_level_t level, const char* message, ...);
int set_message(char* message);
void logger_init(void);
void logger_destroy(void);

#endif //LOGGER_H
