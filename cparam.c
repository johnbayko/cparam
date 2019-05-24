#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cparam.h"

struct cparam_info *
cparam_next(struct cparam_info * const param)
{
    if (NULL == param)
    {
        return NULL;
    }
    if (CPARAM_KEYWORD == param->type)
    {
        const struct cparam_keyword_info * const key =
            &param->key_list[param->key_idx];
        if (NULL != key->next_param) {
            return key->next_param;
        }
    }
    if (NULL != param->next_param)
    {
        return param->next_param;
    }
    return NULL;
}

static bool
cparam_process_arg(
    const int argc,
    const char * const argv[],
    const int argv_current,
    struct cparam_info * const param,
    char * const err_msg,
    const size_t err_msg_size
) {
    if (CPARAM_ACTION == param->type) {
        // Action parameter does not process an argument.
        return true;
    }
    if (argv_current >= argc)
    {
        if (NULL != err_msg)
        {
            snprintf(err_msg, err_msg_size, "Missing arguments.");
        }
        return false;
    }
    switch (param->type)
    {
    case CPARAM_STRING:
        param->str_val = argv[argv_current];
        break;
    case CPARAM_INT:
        {
            param->str_val = argv[argv_current];

            char *endptr = NULL;
            long int_val = strtol(argv[argv_current], &endptr, 0);
            if (endptr == argv[argv_current])
            {
                /* No integer found. */
                if (NULL != err_msg)
                {
                    snprintf(err_msg, err_msg_size,
                        "Not a valid integer: \"%s\"",
                        argv[argv_current]
                    );
                }
                return false;
            }
            if (param->has_range)
            {
                if ( (int_val < param->int_val_min)
                  || (int_val > param->int_val_max) )
                {
                    if (NULL != err_msg)
                    {
                        snprintf(err_msg, err_msg_size,
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
    case CPARAM_KEYWORD:
        {
            param->str_val = argv[argv_current];

            /* Find which keyword in list matches. */
            int key_idx = 0;
            const int key_lim = param->key_lim;
            const struct cparam_keyword_info * matching_key = NULL;

            // Partial matches okay if unique. Count number of matches, if
            // one then found the keyword.
            int num_matches = 0;
            for (key_idx = 0;key_idx < key_lim;key_idx++)
            {
                const struct cparam_keyword_info * const current_key =
                    &param->key_list[key_idx];
                // Only compare to length of parameter for partial matches.
                if (0 == strncmp(
                        current_key->name,
                        argv[argv_current],
                        strlen(argv[argv_current])
                    )
                ) {
                    num_matches++;
                    matching_key = current_key;
                    break;
                }
            }
            if (1 != num_matches)
            {
                if (NULL != err_msg)
                {
                    snprintf(err_msg, err_msg_size,
                        "Keyword \"%s\" %s.",
                        argv[argv_current],
                        (0 == num_matches)
                            ? "is not in the keyword list"
                            : "matches too many keywords"
                    );
                }
                return false;
            }
            param->key_idx = key_idx;
            param->int_val = matching_key->val;
        }
        break;
    case CPARAM_ACTION:
        // Doesn't parse anything.
        break;
    }
    return true;
}

bool
cparam_process(
    const int argc,
    const char * const argv[],
    int * argv_idx, // argv position to start, updated to last one parsed.
    struct cparam_info * const start_param,
    char * const err_msg,
    const size_t err_msg_size
) {
    int argv_current = (NULL != argv_idx) ? *argv_idx : 0;
    if ((NULL == argv) || (NULL == start_param))
    {
        if (NULL != argv_idx)
        {
            *argv_idx = argv_current;
        }
        if (NULL != err_msg)
        {
            snprintf(
                err_msg, err_msg_size,
                "%s called with NULL %s%s%s parameter.",
                __func__,
                (NULL == argv) ? "argv" : "",
                ((NULL == argv) && (NULL == start_param)) ? ", " : "",
                (NULL == start_param) ? "start_param" : ""
            );
        }
        return false;
    }
    struct cparam_info * param = start_param;

    for (;;)
    {
        if ( !cparam_process_arg(
                argc, argv, argv_current, param, err_msg, err_msg_size
            )
        ) {
            if (NULL != argv_idx)
            {
                *argv_idx = argv_current;
            }
            return false;
        }
        if (NULL != param->action)
        {
            if ( !param->action(
                    start_param, param->action_data, err_msg, err_msg_size
                )
            ) {
                if (NULL != argv_idx)
                {
                    *argv_idx = argv_current;
                }
                return false;
            }
        }
        /* Param is processed, so cparam_next works now. */
        param = cparam_next(param);
        if (NULL == param)
        {
            /* End of the line. */
            /* If you wanted to be strict, you could return false if there were
               unused arguments (argv_current < argc - 1). */
            if (NULL != argv_idx)
            {
                *argv_idx = argv_current;
            }
            return true;
        }
        argv_current++;
    }
}

static void
cparam_indent(const unsigned int indent_level)
{
    for (unsigned int indent_cnt = 0;indent_cnt < indent_level;indent_cnt++)
    {
        printf("  ");
    }
}

static bool
cparam_has_more_lines(
    const struct cparam_info * const start_param
) {
    for ( const struct cparam_info * param = start_param;
        NULL != param;
        param = param->next_param )
    {
        if ((NULL != param->desc) && ('\0' != param->desc[0])) {
            // Has a description.
            return true;
        }
        if (CPARAM_KEYWORD == param->type)
        {
            // Has keyword in list.
            return true;
        }
    }
    return false;
}

static void
cparam_print_parameters(
    const struct cparam_info * const start_param,
    unsigned int indent_level
) {
    /* First list the paramers. */
    for ( const struct cparam_info * param = start_param;
        NULL != param;
        param = param->next_param )
    {
        const bool has_name = (NULL != param->name) && ('\0' != param->name[0]);
        const char * const name = has_name ? param->name : "";
        const char * const sep = has_name ? ":" : "";

        switch (param->type)
        {
        case CPARAM_STRING:
            printf("<%s%sstring> ", name, sep);
            break;
        case CPARAM_INT:
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
        case CPARAM_KEYWORD:
            if (has_name)
            {
                printf("<%s> ", name);
            }
            else
            {
                printf("<keyword> ");
            }
            break;
        case CPARAM_ACTION:
            // No corresponding argument.
            break;
        }
    }
}

static void
cparam_print_main(
    const struct cparam_info * const start_param,
    unsigned int indent_level
) {
    if (!cparam_has_more_lines(start_param)) {
        return;
    }
    for ( const struct cparam_info * param = start_param;
        NULL != param;
        param = param->next_param )
    {
        const bool has_desc =
            (NULL != param->desc) && ('\0' != param->desc[0]);
        if (has_desc || (CPARAM_KEYWORD == param->type))
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
                case CPARAM_STRING:
                    name = "string";
                    break;
                case CPARAM_INT:
                    name = "integer";
                    break;
                case CPARAM_KEYWORD:
                    name = "keyword";
                    break;
                case CPARAM_ACTION:
                    // Shouldn't get here
                    break;
                }
            }
            cparam_indent(indent_level + 1);
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
            if (CPARAM_KEYWORD == param->type)
            {
                const int key_lim = param->key_lim;
                bool has_next_param = false;
                for (int key_idx = 0;key_idx < key_lim;key_idx++)
                {
                    const struct cparam_keyword_info * key =
                        &param->key_list[key_idx];
                    if (NULL != key->next_param)
                    {
                        has_next_param = true;
                    }
                }
                printf(" One of: \n");
                /* If any keyword has more parameters, print one per line,
                   otherwise print all on one line. */
                if (!has_next_param)
                {
                    cparam_indent(indent_level + 2);
                    for (int key_idx = 0;key_idx < key_lim;key_idx++)
                    {
                        const struct cparam_keyword_info * key =
                            &param->key_list[key_idx];
                        printf("%s ", key->name);
                    }
                }
                else
                {
                    for (int key_idx = 0;key_idx < key_lim;key_idx++)
                    {
                        const struct cparam_keyword_info * key =
                            &param->key_list[key_idx];
                        const struct cparam_info * const next_param =
                            key->next_param;
                        cparam_indent(indent_level + 2);
                        printf("%s ", key->name);
                        cparam_print_parameters(next_param, indent_level);
                        printf("\n");
                        cparam_print_main(next_param, indent_level + 2);
                    }
                }
            }
            printf("\n");
        }
    }
}

void
cparam_print_param_names(const struct cparam_info * const start_param) {
    cparam_print_parameters(start_param, 0);
}

void
cparam_print(const struct cparam_info * const start_param) {
    cparam_print_main(start_param, 1);
}

