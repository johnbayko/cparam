#ifndef SHVAR_SET_PARAM_H
#define SHVAR_SET_PARAM_H
#include <stdbool.h>

#ifndef DIM
#define DIM(a) (sizeof(a)/sizeof(a[0]))
#endif

enum param_type {
    PARAM_STRING,
    PARAM_INT,
    PARAM_KEYWORD,
};

struct param_info; /* Forward declaration. */
typedef bool (*param_action)(
    struct param_info * param,
    void * data,    // May be NULL.
    char * err_msg, // May be NULL.
    size_t err_len
);

/*
    If parameter is a keyword type, then next parameter might depend on which
    keyword was specified. In that case, next_param will be NULL and the next
    parameter will be found in the matching keyword_info struct.

    If the keyword is just a flag, then you can set next_param here.
 */
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
    struct param_info * const next_param; // NULL if none for this keyword.
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

struct param_info * param_next(struct param_info * const param);
bool param_process(
    const int argc,
    const char * const argv[],
    struct param_info * const start_param,
    char * err_msg,
    size_t err_len,
    int * err_idx  // Which argv index failed.
);
void param_print(
    const char * const prog_name, const struct param_info * const start_param
);
#endif
