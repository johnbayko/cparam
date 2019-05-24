#ifndef CPARAM_H
#define CPARAM_H
#include <stdbool.h>

#ifndef DIM
#define DIM(a) (sizeof(a)/sizeof(a[0]))
#endif

enum cparam_type {
    CPARAM_STRING,
    CPARAM_INT,
    CPARAM_KEYWORD,
    CPARAM_ACTION,
};

struct cparam_info; /* Forward declaration. */
typedef bool (*cparam_action)(
    struct cparam_info * param,
    void * data,    // May be NULL.
    char * err_msg, // May be NULL.
    size_t err_len
);

/*
    If parameter is a keyword type, then next parameter might depend on which
    keyword was specified. In that case, next_param will be NULL and the next
    parameter will be found in the matching cparam_keyword_info struct.

    If the keyword is just a flag, then you can set next_param here.
 */
struct cparam_info {
    const enum cparam_type type;

    // For CPARAM_INT.
    // Range is inclusive. To specify list of valid values, use CPARAM_KEYWORD
    // with "1"=1, etc.
    const bool has_range;
    const int int_val_min;
    const int int_val_max;

    // For CPARAM_KEYWORD.
    const struct cparam_keyword_info * const key_list;
    const unsigned int key_lim;

    // NULL if no next, or if depends on keyword.
    struct cparam_info * const next_param;
    const cparam_action action;
    void * const action_data;

    // For printing usage.
    const char * const name;
    const char * const desc;

    // If parsing succeeds, these will always have the appropriate value.
    // Will be argv in all cases.
    const char *str_val;
    // For CPARAM_INT and CPARAM_KEYWORD.
    int int_val;
    // For CPARAM_KEYWORD, use as key_list[key_idx].
    int key_idx;
};


struct cparam_keyword_info {
    const char * const name;
    const int val;
    struct cparam_info * const next_param; // NULL if none for this keyword.
};

#define CPARAM_INFO_STRING(name, desc, next) \
    {CPARAM_STRING, false, 0, 0, NULL, 0, next, NULL, NULL, name, desc, NULL, 0, 0}

#define CPARAM_INFO_LAST_STRING(name, desc, action, data) \
    {CPARAM_STRING, false, 0, 0, NULL, 0, NULL, action, data, name, desc, NULL, 0, 0}

#define CPARAM_INFO_INT(name, desc, next) \
    {CPARAM_INT, false, 0, 0, NULL, 0, next, NULL, NULL, name, desc, NULL, 0, 0}

#define CPARAM_INFO_LAST_INT(name, desc, action, data) \
    {CPARAM_INT, false, 0, 0, NULL, 0, NULL, action, data, name, desc, NULL, 0, 0}

#define CPARAM_INFO_INT_RANGE(name, desc, min, max, next) \
    {CPARAM_INT, true, min, max, NULL, 0, next, NULL, NULL, name, desc, NULL, 0, 0}

#define CPARAM_INFO_LAST_INT_RANGE(name, desc, min, max, action, data) \
    {CPARAM_INT, true, min, max, NULL, 0, NULL, action, data, name, desc, NULL, 0, 0}

#define CPARAM_INFO_KEYWORD(name, desc, key_list, next) \
    {CPARAM_KEYWORD, false, 0, 0, key_list, DIM(key_list), next, NULL, NULL, name, desc, NULL, 0, 0}

#define CPARAM_INFO_LAST_KEYWORD(name, desc, key_list, action, data) \
    {CPARAM_KEYWORD, false, 0, 0, key_list, DIM(key_list), NULL, action, data, name, desc, NULL, 0, 0}

#define CPARAM_INFO_ACTION(action, data) \
    {CPARAM_ACTION, false, 0, 0, NULL, 0, NULL, action, data, NULL, NULL, NULL, 0, 0}

struct cparam_info * cparam_next(struct cparam_info * const param);
bool cparam_process(
    const int argc,
    const char * const argv[],
    int * argv_idx,  // argv position to start, updated to last one parsed.
    struct cparam_info * const start_param,
    char * const err_msg,
    size_t err_msg_size
);
void cparam_print_param_names(const struct cparam_info * const start_param);
void cparam_print(const struct cparam_info * const start_param);
#endif  // CPARAM_H
