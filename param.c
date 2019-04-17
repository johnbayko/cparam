#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shvar_set_param.h"

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

static void
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

