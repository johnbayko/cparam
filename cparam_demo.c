#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cparam.h"

enum tempmon_units {
    TEMPMON_KELVIN,
    TEMPMON_CELCIUS,
    TEMPMON_FARENHEIT,
};

enum tempmon_alarm_level {
    TEMPMON_ALARM_LOW,
    TEMPMON_ALARM_MEDIUM,
    TEMPMON_ALARM_HIGH,
};

enum tempmon_enum {
  TEMPMON_ON,
  TEMPMON_OFF,
  TEMPMON_FAN,
  TEMPMON_HEATER,
  TEMPMON_RANGE,
  TEMPMON_ALARM,
};


static bool action_on(
    struct cparam_info * param,
    void * data,
    char * err_msg,
    size_t err_len
) {
    printf("Set temperature monitoring ON.\n");
    return true;
}

static bool action_off(
    struct cparam_info * param,
    void * data,
    char * err_msg,
    size_t err_len
) {
    printf("Set temperature monitoring OFF.\n");
    return true;
}

static bool action_fan(
    struct cparam_info * param,
    void * data,
    char * err_msg,
    size_t err_len
) {
    struct cparam_info *scan_param = param;
    // Skip first param.

    scan_param = cparam_next(scan_param);
    const int temperature = scan_param->int_val;

    scan_param = cparam_next(scan_param);
    const char * const units_name = scan_param->str_val;
    const enum tempmon_units units = scan_param->int_val;

    printf("Set fan on at or above %d degrees %s (%d).\n",
        temperature,
        units_name,
        units
    );
    return true;
}

static bool action_heater(
    struct cparam_info * param,
    void * data,
    char * err_msg,
    size_t err_len
) {
    struct cparam_info *scan_param = param;
    // Skip first param.

    scan_param = cparam_next(scan_param);
    const int temperature = scan_param->int_val;

    scan_param = cparam_next(scan_param);
    const char * const units_name = scan_param->str_val;
    const enum tempmon_units units = scan_param->int_val;

    printf("Set heater on at or below %d degrees %s (%d).\n",
        temperature,
        units_name,
        units
    );
    return true;
}

static bool action_range(
    struct cparam_info * param,
    void * data,
    char * err_msg,
    size_t err_len
) {
    struct cparam_info *scan_param = param;
    // Skip first param.

    scan_param = cparam_next(scan_param);
    const int min_temp = scan_param->int_val;

    scan_param = cparam_next(scan_param);
    const int max_temp = scan_param->int_val;

    scan_param = cparam_next(scan_param);
    const char * const units_name = scan_param->str_val;
    const enum tempmon_units units = scan_param->int_val;

    printf("Signal alarm below %d or above %d degrees %s (%d).\n",
        min_temp,
        max_temp,
        units_name,
        units
    );
    return true;
}

static bool action_alarm(
    struct cparam_info * param,
    void * data,
    char * err_msg,
    size_t err_len
) {
    struct cparam_info *scan_param = param;
    // Skip first param.

    scan_param = cparam_next(scan_param);
    const char * const alarm_level_name = scan_param->str_val;
    const enum tempmon_alarm_level alarm_level_val = scan_param->int_val;

    printf("Set alarm level to %s (%d).\n",
        alarm_level_name,
        alarm_level_val
    );
    return true;
}


struct cparam_info tempmon_on_param =
    CPARAM_INFO_ACTION(
        action_on, NULL
    );

struct cparam_info tempmon_off_param =
    CPARAM_INFO_ACTION(
        action_off, NULL
    );

struct cparam_keyword_info tempmon_units_list[] = {
    {"kelvin", TEMPMON_KELVIN, NULL},
    {"celcius", TEMPMON_CELCIUS, NULL},
    {"farenheit", TEMPMON_FARENHEIT, NULL},
};

struct cparam_info tempmon_action_max_units_param =
    CPARAM_INFO_LAST_KEYWORD(
        "units", "Temperature units.", tempmon_units_list, action_fan, NULL
    );

struct cparam_info tempmon_fan_param =
    CPARAM_INFO_INT(
        "temp",
        "High temperature to activate.",
        &tempmon_action_max_units_param
    );

struct cparam_info tempmon_action_min_units_param =
    CPARAM_INFO_LAST_KEYWORD(
        "units", "Temperature units.", tempmon_units_list, action_heater, NULL
    );

struct cparam_info tempmon_heater_param =
    CPARAM_INFO_INT(
        "temp",
        "Low temperature to activate.",
        &tempmon_action_min_units_param
    );

struct cparam_info tempmon_range_units_param =
    CPARAM_INFO_LAST_KEYWORD(
        "units", "Temperature units.", tempmon_units_list, action_range, NULL
    );

struct cparam_info tempmon_range_param =
    CPARAM_INFO_INT(
        "temp",
        "Maximum temperature to trigger alarm.",
        &tempmon_range_units_param
    );

struct cparam_info tempmon_range_min_param =
    CPARAM_INFO_INT(
        "temp",
        "Minimum temperature to trigger alarm.",
        &tempmon_range_param
    );

struct cparam_keyword_info tempmon_alarm_list[] = {
    {"low", TEMPMON_ALARM_LOW, NULL},
    {"medium", TEMPMON_ALARM_MEDIUM, NULL},
    {"high", TEMPMON_ALARM_HIGH, NULL},
};

struct cparam_info tempmon_alarm_param =
    CPARAM_INFO_LAST_KEYWORD(
        "level",
        "Alarm level when temperature exceeds range.",
        tempmon_alarm_list,
        action_alarm,
        NULL
    );

struct cparam_keyword_info tempmon_list[] = {
    {"on", TEMPMON_ON, &tempmon_on_param},
    {"off", TEMPMON_OFF, &tempmon_off_param},
    {"fan", TEMPMON_FAN, &tempmon_fan_param},
    {"heater", TEMPMON_HEATER, &tempmon_heater_param},
    {"range", TEMPMON_RANGE, &tempmon_range_min_param},
    {"alarm", TEMPMON_ALARM, &tempmon_alarm_param},
};

struct cparam_info tempmon_param =
    CPARAM_INFO_KEYWORD(
        "oper", "Temperature monitor operation.", tempmon_list, NULL
    );


static void print_usage(const char * cmd_name) {
    printf("%s <options> [<options> ...]\n", cmd_name);
    printf("Where <options> are:\n");

    printf("  [-t | --tempmon] ");
    cparam_print_param_names(&tempmon_param);
    printf("\n");
    cparam_print(&tempmon_param);

    printf("  [-? | --help]: Print this message.\n");
}

int main(const int argc, const char * const argv[]) {
    for (int argi = 1;argi < argc;argi++) {
        char err_msg[256];
        bool cparam_process_success = false;

        if ( (0 == strcmp("-t", argv[argi]))
          || (0 == strcmp("--tempmon", argv[argi]))
        ) {
            argi++;
            cparam_process_success = 
                cparam_process(
                    argc, argv, &argi, &tempmon_param, err_msg, sizeof(err_msg)
                );
        } else if ( (0 == strcmp("-s", argv[argi]))
          || (0 == strcmp("--stringtest", argv[argi]))
        ) {
            print_usage(argv[0]);
        } else {
            printf("Unrecognized option: %s\n\n", argv[argi]);
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
        if (cparam_process_success) {
            struct cparam_info *param = &tempmon_param;
            while (NULL != param)
            {
                switch (param->type)
                {
                case CPARAM_STRING:
                    printf("string: \"%s\"\n", param->str_val);
                    break;
                case CPARAM_INT:
                    printf("int: \"%s\" = %d\n",
                        param->str_val, param->int_val
                    );
                    break;
                case CPARAM_KEYWORD:
                    printf("keyword: \"%s\" = %d [%d]\n",
                        param->str_val,
                        param->int_val,
                        param->key_idx
                    );
                    break;
                case CPARAM_ACTION:
                    printf("action: \n");
                    break;
                }
                param = cparam_next(param);
            }
        } else {
            printf("Incorrect --tempmon parameters: %s\n", err_msg);
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}
