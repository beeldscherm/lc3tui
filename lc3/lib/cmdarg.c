#include "cmdarg.h"
#include "va_template.h"


// Returns an allocated copy of str, or NULL if str is NULL
static char *strdup(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    int l = strlen(str) + 1;

    char *ret = malloc(l);
    memcpy(ret, str, l);

    return ret;
}


// Free if nonnull
static void _free_nn(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}


/*
 * Struct representing a flag and some additional information
 * Used in both ca_config and ca_info, hence the union
 * For ca_config, the "st" struct is used
 * For ca_info, the "val" pointer is used
*/
typedef struct _ca_flag_entry {
    char *name;                 // Name of the flag
    union {
        char *val;              // Value for flag
        struct {
            uint64_t flag_bit;  // Which bits this flag should set
            bool has_bit;       // Whether this flags has flag_bit(s) defined
            bool expects_value; // Whether this flag should have a value
        } st;
    } un;
} _ca_flag_entry;


// Boilerplate functions for _ca_flag_entry and string arrays
vaTypedef(_ca_flag_entry, _ca_fe_arr);
vaAllocFunction(_ca_fe_arr, _ca_flag_entry, _ca_fe_arr_alloc,,)
vaAppendFunction(_ca_fe_arr, _ca_flag_entry, _ca_fe_arr_add,,)
vaFreeFunction(_ca_fe_arr, _ca_flag_entry, _ca_fe_arr_free_config, _free_nn(el.name),,)
vaFreeFunction(_ca_fe_arr, _ca_flag_entry, _ca_fe_arr_free_info, _free_nn(el.name); _free_nn(el.un.val),,)

vaTypedef(char *, _ca_str_arr);
vaAllocFunction(_ca_str_arr, char *, _ca_str_arr_alloc,,)
vaAppendFunction(_ca_str_arr, char *, _ca_str_arr_add,,)
vaFreeFunction(_ca_str_arr, char *, _ca_str_arr_free, free(el),,)


// ca_config stores a list of flags
struct ca_config {
    _ca_fe_arr entries;
};


// ca_info stores a list of flags, a list of literals, and the flag bits
struct ca_info {
    _ca_fe_arr flags;
    _ca_str_arr literals;
    uint64_t flag_bits;
};


ca_config *ca_alloc_config() {
    ca_config *ret = malloc(sizeof(ca_config));
    ret->entries = _ca_fe_arr_alloc();
    return ret;
}


void ca_free_config(ca_config *cfg) {
    _ca_fe_arr_free_config(cfg->entries);
    free(cfg);
}


void ca_free_info(ca_info *info) {
    _ca_fe_arr_free_info(info->flags);
    _ca_str_arr_free(info->literals);
    free(info);
}


// Compare function for qsort on a _ca_flag_entry array
static int _e_cmp_fn(const void *s1, const void *s2) {
    return strcmp(((_ca_flag_entry *)s1)->name, ((_ca_flag_entry *)s2)->name);
}


// Binary search on a sorted _ca_flag_entry array
static int _get_flag_index(_ca_flag_entry *arr, int sz, const char *flag) {
    int low = 0, high = sz - 1;

    while (low <= high) {
        int mid = (low + high + 1) / 2;
        int cmp = strcmp(flag, arr[mid].name);

        if (cmp == 0) {
            return mid;
        } else if (cmp > 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return -1;
}


// Splits flag into before and after '='
static void _ca_split(const char *flag, char **name, char **val) {
    const char *tf = flag + (flag[0] == '=');
    char *eq = strchr(tf, '=');

    int len = strlen(flag);

    int name_len = eq ? (eq - flag) : len;
    int val_len  = len - name_len - 1;

    if (eq == NULL || val_len == 0) {
        (*name) = strdup(flag);
        (*val)  = NULL;
        return;
    }

    (*name) = malloc(name_len + 1);
    (*val)  = malloc(val_len  + 1);

    memcpy(*name, flag, name_len);
    memcpy(*val, eq + 1, val_len + 1);
    (*name)[name_len] = '\0';
}


void ca_bind_flag(ca_config *cfg, const char *flag_name, uint64_t flag_val) {
    _ca_flag_entry entry = {
        .name = strdup(flag_name),
        .un.st.has_bit = true,
        .un.st.flag_bit = flag_val,
        .un.st.expects_value = false,
    };

    _ca_fe_arr_add(&cfg->entries, entry);
}


void ca_set_hasv(ca_config *cfg, const char *flag_name) {
    _ca_flag_entry entry = {
        .name = strdup(flag_name),
        .un.st.has_bit = 0,
        .un.st.expects_value = true,
    };
    
    _ca_fe_arr_add(&cfg->entries, entry);
}


ca_info *ca_parse(ca_config *cfg, int argc, char **argv) {
    qsort(cfg->entries.ptr, cfg->entries.sz, sizeof(_ca_flag_entry), _e_cmp_fn);

    ca_info *ret = malloc(sizeof(ca_info));
    ret->flags = _ca_fe_arr_alloc();
    ret->literals = _ca_str_arr_alloc();
    ret->flag_bits = 0L;

    _ca_flag_entry tmp = {
        .name = NULL, 
        .un.val = NULL
    };

    char *flag_str, *val_str; // for splitting

    for (int i = 0; i < argc; i++) {
        _ca_split(argv[i], &flag_str, &val_str);
        int idx = _get_flag_index(cfg->entries.ptr, cfg->entries.sz, flag_str);

        if (idx < 0) {
            // Not a flag, so we add it as a literal
            _ca_str_arr_add(&ret->literals, strdup(argv[i]));
            _free_nn(flag_str);
            _free_nn(val_str);
            continue;
        }

        // Check what kind of flag this is
        tmp.name = flag_str;
        tmp.un.val  = val_str;

        // Potentially use flag bits
        if (cfg->entries.ptr[idx].un.st.has_bit) {
            ret->flag_bits |= cfg->entries.ptr[idx].un.st.flag_bit;
        }

        // Potentially get value
        if (val_str == NULL && cfg->entries.ptr[idx].un.st.expects_value) {
            i++;
            tmp.un.val = strdup((i == argc) ? NULL : argv[i]);
        }

        _ca_fe_arr_add(&ret->flags, tmp);
    }

    qsort(ret->flags.ptr, ret->flags.sz, sizeof(_ca_flag_entry), _e_cmp_fn);

    return ret;
}


// Get list of literals
const char **ca_literals(ca_info *info, size_t *count) {
    (*count) = info->literals.sz;
    return (const char **)info->literals.ptr;
}


// Get OR'ed values of flags
uint64_t ca_flags(ca_info *info) {
    return info->flag_bits;
}


// Check if flag is set or has a value
bool ca_is_set(ca_info *info, const char *flag_name) {
    return (_get_flag_index(info->flags.ptr, info->flags.sz, flag_name) >= 0);
}


// Get flag value, might return NULL
const char *ca_flag_value(ca_info *info, const char *flag_name) {
    int idx = _get_flag_index(info->flags.ptr, info->flags.sz, flag_name);
    return (idx >= 0) ? info->flags.ptr[idx].un.val : NULL;
}
