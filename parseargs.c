#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DIM
#define DIM(a) (sizeof(a)/sizeof(a[0]))
#endif

enum param_type {
    PARAM_STRING,
    PARAM_INT,
    PARAM_KEYWORD,
};

struct param_info;
typedef bool (*param_action)(
    struct param_info * param,
    void * data,
    char * err_msg,
    size_t err_len
);

struct param_info {
    const enum param_type type;

    // For PARAM_INT.
    // Range is inclusive. To specify list of valid values, use PARAM_KEYWORD
    // with "1"=1, etc.
    const bool has_range;
    const int int_val_min;
    const int int_val_max;

    // For PARAM_KEYWORD.
    const struct keyword_info * const key_list;
    const unsigned int key_lim;

    // NULL if no next, or if depends on keyword.
    struct param_info * const next_param;
    const param_action action;
    void * const action_data;

    // For printing usage.
    const char * const name;
    const char * const desc;

    // If parsing succeeds, these will always have the appropriate value.
    // Will be argv in all cases.
    const char *str_val;
    // For PARAM_INT and PARAM_KEYWORD.
    int int_val;
    // For PARAM_KEYWORD, use as key_list[key_idx].
    int key_idx;
};

struct keyword_info {
    const char * const name;
    const int val;
    struct param_info * const next_param; // NULL if no next.
};

#define PARAM_INFO_STRING(name, desc, next) \
    {PARAM_STRING, false, 0, 0, NULL, 0, next, NULL, NULL, name, desc, NULL, 0, 0}
#define PARAM_INFO_LAST_STRING(name, desc, action, data) \
    {PARAM_STRING, false, 0, 0, NULL, 0, NULL, action, data, name, desc, NULL, 0, 0}
#define PARAM_INFO_INT(name, desc, next) \
    {PARAM_INT, false, 0, 0, NULL, 0, next, NULL, NULL, name, desc, NULL, 0, 0}
#define PARAM_INFO_LAST_INT(name, desc, action, data) \
    {PARAM_INT, false, 0, 0, NULL, 0, NULL, action, data, name, desc, NULL, 0, 0}
#define PARAM_INFO_INT_RANGE(name, desc, min, max, next) \
    {PARAM_INT, true, min, max, NULL, 0, next, NULL, NULL, name, desc, NULL, 0, 0}
#define PARAM_INFO_LAST_INT_RANGE(name, desc, min, max, action, data) \
    {PARAM_INT, true, min, max, NULL, 0, NULL, action, data, name, desc, NULL, 0, 0}
#define PARAM_INFO_KEYWORD(name, desc, key_list, next) \
    {PARAM_KEYWORD, false, 0, 0, key_list, DIM(key_list), next, NULL, NULL, name, desc, NULL, 0, 0}
#define PARAM_INFO_LAST_KEYWORD(name, desc, key_list, action, data) \
    {PARAM_KEYWORD, false, 0, 0, key_list, DIM(key_list), NULL, action, data, name, desc, NULL, 0, 0}

struct param_info *
param_next(struct param_info * const param)
{
    if (NULL == param)
    {
        return NULL;
    }
    if (NULL != param->next_param)
    {
        return param->next_param;
    }
    if (PARAM_KEYWORD == param->type)
    {
        const struct keyword_info * const key =
            &param->key_list[param->key_idx];
        return key->next_param;
    }
    return NULL;
}

bool
param_process(
    const int argc,
    const char * const argv[],
    struct param_info * const start_param,
    char * err_msg,
    size_t err_len,
    int * err_idx  // Which argv index failed.
) {
    int argv_idx = 0;
    if ((NULL == argv) || (NULL == start_param))
    {
        if (NULL != err_idx)
        {
            *err_idx = argv_idx;
        }
        if (NULL != err_msg)
        {
            snprintf(
                err_msg, err_len,
                "%s called with NULL %s%s%s parameter.",
                __func__,
                (NULL == argv) ? "argv" : "",
                ((NULL == argv) && (NULL == start_param)) ? ", " : "",
                (NULL == start_param) ? "start_param" : ""
            );
        }
        return false;
    }
    struct param_info * param = start_param;

    for (;;)
    {
        if (argv_idx >= argc)
        {
            if (NULL != err_idx)
            {
                *err_idx = argc - 1;
            }
            if (NULL != err_msg)
            {
                snprintf(err_msg, err_len, "Missing arguments.");
            }
            return false;
        }
        param->str_val = argv[argv_idx];
        switch (param->type)
        {
        case PARAM_STRING:
            /* Done by default. */
            break;
        case PARAM_INT:
            {
                char *endptr = NULL;
                long int_val = strtol(argv[argv_idx], &endptr, 0);
                if (endptr == argv[argv_idx])
                {
                    /* No integer found. */
                    if (NULL != err_idx)
                    {
                        *err_idx = argv_idx;
                    }
                    if (NULL != err_msg)
                    {
                        snprintf(err_msg, err_len,
                            "Not a valid integer: \"%s\"",
                            argv[argv_idx]
                        );
                    }
                    return false;
                }
                if (param->has_range)
                {
                    if ( (int_val < param->int_val_min)
                      || (int_val > param->int_val_max) )
                    {
                        if (NULL != err_idx)
                        {
                            *err_idx = argv_idx;
                        }
                        if (NULL != err_msg)
                        {
                            snprintf(err_msg, err_len,
                     "Specified value %ld is not between %d and %d (inclusive).",
                                int_val,
                                param->int_val_min,
                                param->int_val_max
                            );
                        }
                        return false;
                    }
                }
                param->int_val = int_val;
            }
            break;
        case PARAM_KEYWORD:
            {
                /* Find which keyword in list matches. */
                int key_idx = 0;
                const int key_lim = param->key_lim;
                const struct keyword_info * matching_key = NULL;

                for (key_idx = 0;key_idx < key_lim;key_idx++)
                {
                    const struct keyword_info * const current_key =
                        &param->key_list[key_idx];
                    if (0 == strcmp(current_key->name, argv[argv_idx]))
                    {
                        matching_key = current_key;
                        break;
                    }
                }
                if (NULL == matching_key)
                {
                    if (NULL != err_idx)
                    {
                        *err_idx = argv_idx;
                    }
                    if (NULL != err_msg)
                    {
                        snprintf(err_msg, err_len,
                            "Keyword \"%s\" is not in the keyword list.",
                            argv[argv_idx]
                        );
                    }
                    return false;
                }
                param->key_idx = key_idx;
                param->int_val = matching_key->val;
            }
            break;
        }
        if (NULL != param->action)
        {
            if ( !param->action(
                    start_param, param->action_data, err_msg, err_len
                )
            ) {
                if (NULL != err_idx)
                {
                    *err_idx = argv_idx;
                }
                return false;
            }
        }
        /* Param is processed, so param_next works now. */
        param = param_next(param);
        if (NULL == param)
        {
            /* End of the line. */
            /* If you wanted to be strict, you could return false if there were
               unused arguments (argv_idx < argc - 1). */
            return true;
        }
        argv_idx++;
    }
}

void
param_indent(const unsigned int indent_level)
{
    for (unsigned int indent_cnt = 0;indent_cnt < indent_level;indent_cnt++)
    {
        printf("    ");
    }
}

void
param_print_main(
    const struct param_info * const start_param, unsigned int indent_level
) {
    bool has_desc_in_list = false;
    bool has_keyword_in_list = false;
    /* First list the paramers. */
    for ( const struct param_info * param = start_param;
        NULL != param;
        param = param->next_param )
    {
        const bool has_name = (NULL != param->name) && ('\0' != param->name[0]);
        const char * const name = has_name ? param->name : "";
        const char * const sep = has_name ? ":" : "";

        has_desc_in_list = (NULL != param->desc) && ('\0' != param->desc[0]);

        switch (param->type)
        {
        case PARAM_STRING:
            printf("<%s%sstring> ", name, sep);
            break;
        case PARAM_INT:
            if (param->has_range)
            {
                printf("<%s%s%d-%d> ",
                    name, sep, param->int_val_min, param->int_val_max
                );
            }
            else
            {
                printf("<%s%sinteger> ", name, sep);
            }
            break;
        case PARAM_KEYWORD:
            has_keyword_in_list = true;
            if (has_name)
            {
                printf("<%s> ", name);
            }
            else
            {
                printf("<keyword> ");
            }
            break;
        }
    }
    printf("\n");
    /* Next, if any parameter has a description, or is a keyword list, print
       those one per line. */
    if (has_desc_in_list || has_keyword_in_list)
    {
        param_indent(indent_level);
        printf("Where:\n");
        param_indent(indent_level + 1);
        for ( const struct param_info * param = start_param;
            NULL != param;
            param = param->next_param )
        {
            const bool has_desc =
                (NULL != param->desc) && ('\0' != param->desc[0]);
            if (has_desc || (PARAM_KEYWORD == param->type))
            {
                const bool has_name =
                    (NULL != param->name) && ('\0' != param->name[0]);
                /* Print name first. */
                const char * name = "";
                if (has_name)
                {
                    name = param->name;
                }
                else
                {
                    switch (param->type)
                    {
                    case PARAM_STRING:
                        name = "string";
                        break;
                    case PARAM_INT:
                        name = "integer";
                        break;
                    case PARAM_KEYWORD:
                        name = "keyword";
                        break;
                    }
                }
                if (param->has_range)
                {
                    if (has_name)
                    {
                        printf("<%s:%d-%d>:",
                            name, param->int_val_min, param->int_val_max
                        );
                    }
                    else
                    {
                        printf("<%d-%d>:",
                            param->int_val_min, param->int_val_max
                        );
                    }
                }
                else
                {
                    printf("<%s>:", name);
                }

                /* Print additional information - description, keyword list. */
                if (has_desc)
                {
                    printf(" %s", param->desc);
                }
                if (PARAM_KEYWORD == param->type)
                {
                    const int key_lim = param->key_lim;
                    bool has_next_param = false;
                    for (int key_idx = 0;key_idx < key_lim;key_idx++)
                    {
                        const struct keyword_info * key =
                            &param->key_list[key_idx];
                        if (NULL != key->next_param)
                        {
                            has_next_param = true;
                        }
                    }
                    printf(" One of: ");
                    /* If any keyword has more parameters, print one per line,
                       otherwise print all on one line. */
                    if (!has_next_param)
                    {
                        for (int key_idx = 0;key_idx < key_lim;key_idx++)
                        {
                            const struct keyword_info * key =
                                &param->key_list[key_idx];
                            printf("%s ", key->name);
                        }
                    }
                    else
                    {
                        printf("\n");
                        for (int key_idx = 0;key_idx < key_lim;key_idx++)
                        {
                            const struct keyword_info * key =
                                &param->key_list[key_idx];
                            param_indent(indent_level + 2);
                            printf("%s ", key->name);
                            param_print_main(key->next_param, indent_level + 2);
                        }
                    }
                }
                printf("\n");
            }
        }
    }
}

void
param_print(
    const char * const prog_name, const struct param_info * const start_param
) {
    printf("%s ", prog_name);
    param_print_main(start_param, 0);
}

bool
test_action_ok(
    struct param_info * start_param,
    void * data,
    char * err_msg,
    size_t err_len
) {
    printf("true\n");
    for ( struct param_info * param = start_param;
        NULL != param;
        param = param_next(param) )
    {
        printf("type %d str_val \"%s\"\n", param->type, param->str_val);
    }
    return true;
}

bool
test_action_fail(
    struct param_info * start_param,
    void * data,
    char * err_msg,
    size_t err_len
) {
    printf("false\n");
    for ( struct param_info * param = start_param;
        NULL != param;
        param = param_next(param) )
    {
        printf("type %d str_val \"%s\"\n", param->type, param->str_val);
    }
    if (NULL != err_msg)
    {
        snprintf(err_msg, err_len, "Error message");
    }
    return false;
}

struct param_info status_lo_info_mute_lo_param =
    PARAM_INFO_INT_RANGE("", "1=mute, 0=not mute.", 0, 1, NULL);
struct param_info status_lo_info_dac_ref_param =
    PARAM_INFO_INT_RANGE("", "0 to 5 or 255 for not used.", 0, 255, NULL);
enum status_lo_info_param {
    STATUS_LO_INFO_MUTE_LO,
    STATUS_LO_INFO_DAC_REF,
};
struct keyword_info status_lo_info_list[] = {
    {"mute_lo", STATUS_LO_INFO_MUTE_LO, &status_lo_info_mute_lo_param},
    {"dac_ref", STATUS_LO_INFO_DAC_REF, &status_lo_info_dac_ref_param},
};
struct param_info status_lo_info_type_param =
    PARAM_INFO_KEYWORD("lo_info type", "", status_lo_info_list, NULL);

struct param_info status_ipv4_param =
    PARAM_INFO_STRING("", "", NULL);
struct param_info status_shv_lo_synth_refresh_param =
    PARAM_INFO_LAST_INT_RANGE("", "0=not refreshing, 1=refreshing rf_settings.txt.", 0, 1, test_action_fail, NULL);
struct param_info status_tc_atten_enabled_param =
    PARAM_INFO_INT_RANGE("", "Enable temperature controlled attenuation.", 0, 1, NULL);
struct param_info status_lo_info_param =
    PARAM_INFO_INT_RANGE("lo #", "", 1, 6, &status_lo_info_type_param);

struct param_info status_system_proc_brd_serial_param =
    PARAM_INFO_INT("serial num", "", NULL);
struct param_info status_system_proc_brd_minor_param =
    PARAM_INFO_INT("minor rev", "", &status_system_proc_brd_serial_param);
struct param_info status_system_proc_brd_major_param =
    PARAM_INFO_INT("major rev", "", &status_system_proc_brd_minor_param);

struct param_info status_shvar_uenv_uboot_ver_param =
    PARAM_INFO_STRING("uboot_ver", "", NULL);
struct param_info status_shvar_uenv_vendor_name_param =
    PARAM_INFO_STRING("vendor_name", "", &status_shvar_uenv_uboot_ver_param);
struct param_info status_shvar_uenv_system_serial_param =
    PARAM_INFO_INT("system_serial", "", &status_shvar_uenv_vendor_name_param);
struct param_info status_shvar_uenv_system_hw_ver_param =
    PARAM_INFO_INT("system_hw_ver", "", &status_shvar_uenv_system_serial_param);
struct param_info status_shvar_uenv_system_bom_param =
    PARAM_INFO_STRING("system_bom", "", &status_shvar_uenv_system_hw_ver_param);
struct param_info status_shvar_uenv_secrelnum_param =
    PARAM_INFO_STRING("secrelnum", "", &status_shvar_uenv_system_bom_param);
struct param_info status_shvar_uenv_prirelnum_param =
    PARAM_INFO_STRING("prirelnum", "", &status_shvar_uenv_secrelnum_param);
struct param_info status_shvar_uenv_ppc_serial_param =
    PARAM_INFO_INT("ppc_serial", "", &status_shvar_uenv_prirelnum_param);
struct param_info status_shvar_uenv_ppc_hw_ver_param =
    PARAM_INFO_INT("ppc_hw_ver", "", &status_shvar_uenv_ppc_serial_param);
struct param_info status_shvar_uenv_ppc_bom_param =
    PARAM_INFO_STRING("ppc_bom", "", &status_shvar_uenv_ppc_hw_ver_param);
struct param_info status_shvar_uenv_oui_param =
    PARAM_INFO_STRING("oui", "", &status_shvar_uenv_ppc_bom_param);
struct param_info status_shvar_uenv_no_eCMM_param =
    PARAM_INFO_INT("no_eCMM", "", &status_shvar_uenv_oui_param);
struct param_info status_shvar_uenv_model_num_param =
    PARAM_INFO_STRING("model_num", "", &status_shvar_uenv_no_eCMM_param);
struct param_info status_shvar_uenv_max_lcl_high_param =
    PARAM_INFO_INT("max_lcl_high", "", &status_shvar_uenv_model_num_param);
struct param_info status_shvar_uenv_hush_param =
    PARAM_INFO_INT("hush", "", &status_shvar_uenv_max_lcl_high_param);
struct param_info status_shvar_uenv_feature_group_param =
    PARAM_INFO_STRING("feature_group", "", &status_shvar_uenv_hush_param);
struct param_info status_shvar_uenv_ethaddr_param =
    PARAM_INFO_STRING("ethaddr", "", &status_shvar_uenv_feature_group_param);
struct param_info status_shvar_uenv_console_shell_access_param =
    PARAM_INFO_INT("console_shell_access", "", &status_shvar_uenv_ethaddr_param);
struct param_info status_shvar_uenv_bootargs_param =
    PARAM_INFO_STRING("bootargs", "", &status_shvar_uenv_console_shell_access_param);

struct param_info status_ecm_info_param =
    PARAM_INFO_STRING("", "", NULL);

enum status_param {
    STATUS_PARAM_IPV4,
    STATUS_PARAM_SHV_LO_SYNTH_REFRESH,
    STATUS_TC_ATTEN_ENABLED,
    STATUS_LO_INFO,
    STATUS_PARAM_SYSTEM_PROC_BRD,
    STATUS_SHVAR_UENV,
    STATUS_PARAM_ECM_INFO,
};
struct keyword_info status_field_list[] = {
    {"ipv4", STATUS_PARAM_IPV4, &status_ipv4_param},
    {"shv_lo_synth_refresh", STATUS_PARAM_SHV_LO_SYNTH_REFRESH, &status_shv_lo_synth_refresh_param},
    {"tc_atten_enabled", STATUS_TC_ATTEN_ENABLED, &status_tc_atten_enabled_param},
    {"lo_info", STATUS_LO_INFO, &status_lo_info_param},
    {"system_proc_brd", STATUS_PARAM_SYSTEM_PROC_BRD, &status_system_proc_brd_major_param},
    {"shvar_uenv", STATUS_SHVAR_UENV, &status_shvar_uenv_bootargs_param},
    {"ecm_info", STATUS_PARAM_ECM_INFO, &status_ecm_info_param},
};
struct param_info status_param =
    PARAM_INFO_KEYWORD("status field", "Description of status field.", status_field_list, NULL);

struct param_info config_fan_high_param =
    PARAM_INFO_INT("fan high", "", NULL);
struct param_info config_fan_low_param =
    PARAM_INFO_INT("fan low", "", &config_fan_high_param);

struct param_info config_snmp_debugdump_status_param =
    PARAM_INFO_INT("", "", NULL);
struct param_info config_snmp_debugdump_starttime_param =
    PARAM_INFO_STRING("", "", NULL);

enum config_param {
    CONFIG_PARAM_FAN_CONTROL,
    CONFIG_PARAM_SNMP_DEBUGDUMP_STATUS,
    CONFIG_PARAM_SNMP_DEBUGDUMP_STARTTIME,
};
struct keyword_info config_field_list[] = {
    {"fan_control", CONFIG_PARAM_FAN_CONTROL, &config_fan_low_param},
    {"snmp_debugdump_status", CONFIG_PARAM_SNMP_DEBUGDUMP_STATUS, &config_snmp_debugdump_status_param},
    {"snmp_debugdump_starttime", CONFIG_PARAM_SNMP_DEBUGDUMP_STARTTIME, &config_snmp_debugdump_starttime_param},
};
struct param_info config_param =
    PARAM_INFO_KEYWORD("config field", "", config_field_list, NULL);

enum set_mode {
    MODE_STATUS,
    MODE_CONFIG,
};
struct keyword_info mode_list[] = {
    {"status", MODE_STATUS, &status_param},
    {"config", MODE_CONFIG, &config_param},
};
struct param_info mode_param =
    PARAM_INFO_KEYWORD("mode", "", mode_list, NULL);

void
test_param_process(const int argc, const char * const argv[])
{
    char error_msg[256] = "";
    int failed_arg = 0;
    if (param_process(
            argc, argv, &mode_param, error_msg, sizeof(error_msg), &failed_arg
        ))
    {
printf("success\n");
#if 0
        struct param_info *param = &mode_param;
        while (NULL != param)
        {
            switch (param->type)
            {
            case PARAM_STRING:
                printf("string: \"%s\"\n", param->str_val);
                break;
            case PARAM_INT:
                printf("int: \"%s\" = %d\n",
                    param->str_val, param->int_val
                );
                break;
            case PARAM_KEYWORD:
                printf("keyword: \"%s\" = %d [%d]\n",
                    param->str_val,
                    param->int_val,
                    param->key_idx
                );
                break;
            }
            param = param_next(param);
        }
#endif
    }
    else
    {
        printf("Failed at: ");
        for (int argv_idx = 0; argv_idx <= failed_arg;argv_idx++)
        {
            printf("%s ", argv[argv_idx]);
        }
        printf("\n  %s\n", error_msg);
    }
}

int
main(const int argc, const char * const argv[])
{
#if 0
    param_print(argv[0], &mode_param);
    {
        const char * const test_argv[] = { "status", "ipv4", "1.2.3.4" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
#endif
    {
        const char * const test_argv[] = { "status", "shv_lo_synth_refresh", "1" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
#if 0
    {
        const char * const test_argv[] = { "status", "tc_atten_enabled", "1" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] =
            { "status", "lo_info", "1", "mute_lo", "1" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] =
            { "status", "lo_info", "6", "dac_ref", "5" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] =
            { "status", "system_proc_brd", "1", "2", "3" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "status", "shvar_uenv",
            "a",
            "2",
            "c",
            "d",
            "5",
            "6",
            "g",
            "8",
            "i",
            "j",
            "11",
            "12",
            "m",
            "n",
            "o",
            "16",
            "17",
            "r",
            "s",
        };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "status", "ecm_info", "a" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }

    {
        const char * const test_argv[] = { "config", "fan_control", "1", "2" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "config", "snmp_debugdump_status", "1" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "config", "snmp_debugdump_starttime", "a" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    /* Should fail. */
    {
        const char * const test_argv[] =
            { "status", "lo_info", "1", "mute_lo", "2" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] =
            { "status", "lo_info", "6", "dac_ref", "256" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] =
            { "status", "lo_info", "7", "dac_ref", "0" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "status", "shv_lo_synth_refresh", "2" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "status", "tc_atten_enabled", "2" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "status", "tc_atten_enabled", "a" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "status", "tc_atten_enabled" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "a", "tc_atten_enabled", "1" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
    {
        const char * const test_argv[] = { "status", "a", "1" };
        const int test_argc = DIM(test_argv);
        test_param_process(test_argc, test_argv);
    }
#endif
}
