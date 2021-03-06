/**
 * @file
 * @author Matt Miller <matt@matthewjmiller.net>
 *
 * @section LICENSE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 *
 * Unit test for mkavl library.  
 *
 * \verbatim
    Test the mkavl structure
  
    Usage:
    -s <seed>
       The starting seed for the RNG (default=seeded by time()).
    -n <nodes>
       The number of nodes to place in the trees (default=15).
    -b <range beginning>
       The smallest (inclusive ) possible data value in the range of values
       (default=0).
    -e <range ending>
       The largest (exclusive) possible data value in the range of values
       (default=100).
    -r <runs>
       The number of runs to do (default=15).
    -v <verbosity level>
       A higher number gives more output (default=0).
   -h
      Display this help message.
   \endverbatim
 */

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include "../mkavl.h"

/**
 * Display a failure message.
 */
#define LOG_FAIL(fmt, args...) \
do { \
    printf("FAILURE(%s:%u): " fmt "\n", __FUNCTION__, __LINE__, ##args); \
} while (0)

/**
 * Determine the number of elements in an array.
 */
#ifndef NELEMS
#define NELEMS(x) (sizeof(x) / sizeof(x[0]))
#endif

/**
 * Compile time assert macro from:
 * http://www.pixelbeat.org/programming/gcc/static_assert.html 
 */
#ifndef CT_ASSERT
#define CT_ASSERT(e) extern char (*CT_ASSERT(void)) [sizeof(char[1 - 2*!(e)])]
#endif

/**
 * Sanity check for infinite loops.
 */
#define MKAVL_TEST_RUNAWAY_SANITY 100000

/** The default node count for runs */
static const uint32_t default_node_cnt = 15;
/** The default number of test runs */
static const uint32_t default_run_cnt = 15;
/** The default verbosity level of messages displayed */
static const uint8_t default_verbosity = 0;
/** The default start of the range for node data values */
static const uint32_t default_range_start = 0;
/** The default end of the range for node data values */
static const uint32_t default_range_end = 100;

/**
 * State for the current test execution.
 */
typedef struct test_mkavl_opts_st_ {
    /** The max number of nodes for the AVL tree */
    uint32_t node_cnt;
    /** The number of separate runs to do */
    uint32_t run_cnt;
    /** The RNG seed for the first run */
    uint32_t seed;
    /** The verbosity level for the test */
    uint8_t verbosity;
    /** The starting value for the data range */
    uint32_t range_start;
    /** The ending value for the data range */
    uint32_t range_end;
} test_mkavl_opts_st;

/**
 * Display the program's help screen and exit as needed.
 *
 * @param do_exit Whether to exit after the output.
 * @param exit_val If exiting the value with which to exit.
 */
static void
print_usage (bool do_exit, int32_t exit_val)
{
    printf("\nTest the mkavl structure\n\n");
    printf("Usage:\n");
    printf("-s <seed>\n"
           "   The starting seed for the RNG (default=seeded by time()).\n");
    printf("-n <nodes>\n"
           "   The number of nodes to place in the trees (default=%u).\n",
           default_node_cnt);
    printf("-b <range beginning>\n"
           "   The smallest (inclusive ) possible data value in the "
           "range of values (default=%u).\n", default_range_start);
    printf("-e <range ending>\n"
           "   The largest (exclusive) possible data value in the "
           "range of values (default=%u).\n", default_range_end);
    printf("-r <runs>\n"
           "   The number of runs to do (default=%u).\n",
           default_run_cnt);
    printf("-v <verbosity level>\n"
           "   A higher number gives more output (default=%u).\n",
           default_verbosity);
    printf("-h\n"
           "   Display this help message.\n");
    printf("\n");

    if (do_exit) {
        exit(exit_val);
    }
}

/**
 * Utility function to output the value of the options.
 *
 * @param opts The options to output.
 */
static void
print_opts (test_mkavl_opts_st *opts)
{
    if (NULL == opts) {
        return;
    }

    printf("test_mkavl_opts: seed=%u, node_cnt=%u, run_cnt=%u,\n"
           "                 range=[%u,%u) verbosity=%u\n",
           opts->seed, opts->node_cnt, opts->run_cnt, opts->range_start,
           opts->range_end, opts->verbosity);
}

/**
 * Store the command line options into a local structure.
 *
 * @param argc The number of options
 * @param argv The string for the options.
 * @param opts The local structure in which to store the parsed info.
 */
static void
parse_command_line (int argc, char **argv, test_mkavl_opts_st *opts)
{
    int c;
    char *end_ptr;
    uint32_t val;

    if (NULL == opts) {
        return;
    }

    opts->node_cnt = default_node_cnt;
    opts->run_cnt = default_run_cnt;
    opts->verbosity = default_verbosity;
    opts->range_start = default_range_start;
    opts->range_end = default_range_end;
    opts->seed = (uint32_t) time(NULL);

    while ((c = getopt(argc, argv, "n:r:v:s:b:e:h")) != -1) {
        switch (c) {
        case 'n':
            val = strtol(optarg, &end_ptr, 10);
            if ((end_ptr != optarg) && (0 == errno)) {
                opts->node_cnt = val;
            }
            break;
        case 'r':
            val = strtol(optarg, &end_ptr, 10);
            if ((end_ptr != optarg) && (0 == errno)) {
                opts->run_cnt = val;
            }
            break;
        case 'v':
            val = strtol(optarg, &end_ptr, 10);
            if ((end_ptr != optarg) && (0 == errno)) {
                opts->verbosity = val;
            }
            break;
        case 's':
            val = strtol(optarg, &end_ptr, 10);
            if ((end_ptr != optarg) && (0 == errno)) {
                opts->seed = val;
            }
            break;
        case 'b':
            val = strtol(optarg, &end_ptr, 10);
            if ((end_ptr != optarg) && (0 == errno)) {
                opts->range_start = val;
            }
            break;
        case 'e':
            val = strtol(optarg, &end_ptr, 10);
            if ((end_ptr != optarg) && (0 == errno)) {
                opts->range_end = val;
            }
            break;
        case 'h':
        case '?':
        default:
            print_usage(true, EXIT_SUCCESS);
            break;
        }
    }

    if (optind < argc) {
        print_usage(true, EXIT_SUCCESS);
    }

    if (opts->range_start >= opts->range_end) {
        printf("Error: range start(%u) must be strictly less than range "
               "end(%u)\n", opts->range_start, opts->range_end);
        print_usage(true, EXIT_SUCCESS);
    }

    if (0 == opts->node_cnt) {
        printf("Error: node count(%u) must be non-zero\n",
               opts->node_cnt);
        print_usage(true, EXIT_SUCCESS);
    }

    if (opts->verbosity >= 3) {
        print_opts(opts);
    }
}

/**
 * Create a permutation of the given array.
 *
 * @param src_array The array to permute.
 * @param dst_array The output location of the permuted array.
 * @param num_elem The number of array elements in the arrays.
 */
static void
permute_array (const uint32_t *src_array, uint32_t *dst_array, 
               size_t num_elem)
{
    uint32_t i, j, swp_val;

    if ((NULL == src_array) || (NULL == dst_array) || (0 == num_elem)) {
        return;
    }

    memcpy(dst_array, src_array, (num_elem * sizeof(*dst_array)));

    for (i = (num_elem - 1); i > 0; --i) {
        j = (rand() % (i + 1));
        swp_val = dst_array[j];
        dst_array[j] = dst_array[i];
        dst_array[i] = swp_val;
    }
}

/**
 * Callback to compare two pointers to uint32_t's.
 *
 * @param a An item to compare.
 * @param b The other item to compare.
 * @return 0 if equal, -1 if a < b, and 1 if a > b
 */
static int
uint32_t_cmp (const void *a, const void *b)
{
    const uint32_t i1 = *((const uint32_t *) a);
    const uint32_t i2 = *((const uint32_t *) b);

    if (i1 < i2) {
        return (-1);
    } else if (i1 > i2) {
        return (1);
    }

    return (0);
}

/**
 * Get a count of the number of unique items in the array.  This assumes the
 * input array is sorted.
 *
 * @param array The sorted array.
 * @param num_elem The number of elements in the array.
 * @return The unique number of elements in the array.
 */
static uint32_t
get_unique_count (uint32_t *array, size_t num_elem)
{
    uint32_t count = 0;
    uint32_t cur_val;
    uint32_t i;

    if ((NULL == array) || (0 == num_elem)) {
        return (0);
    }

    for (i = 0; i < num_elem; ++i) {
        if ((0 == i) || (array[i] != cur_val)) {
            cur_val = array[i];
            ++count;
        }
    }

    return (count);
}

/**
 * The input structure to pass test parameters to functions.
 */
typedef struct mkavl_test_input_st_ {
    /** The sequence in which items should be inserted */
    uint32_t *insert_seq;
    /** The sequence in which items should be deleted */
    uint32_t *delete_seq;
    /** The sequence in a sorted order */
    uint32_t *sorted_seq;
    /** The count of how many unique items are in the sequence. */
    uint32_t uniq_cnt; 
    /** The count of how many duplicated items are in the sequence. */
    uint32_t dup_cnt; 
    /** The input options for the run */
    const test_mkavl_opts_st *opts;
    /** The tree for the run */
    mkavl_tree_handle tree_h;
    /** A deep copy of the tree (is such a copy has been done) */
    mkavl_tree_handle tree_copy_h;
} mkavl_test_input_st;

/* 
 * Do a forward declaration so we can keep all the AVL setup stuff separate
 * below main().
 */
static bool
run_mkavl_test(mkavl_test_input_st *input);

/**
 * Main function to test objects.
 */
int 
main (int argc, char *argv[])
{
    test_mkavl_opts_st opts;
    bool was_success;
    uint32_t fail_count = 0;
    uint32_t i;
    uint32_t cur_run, cur_seed, uniq_cnt;
    mkavl_test_input_st test_input = {0};

    parse_command_line(argc, argv, &opts);

    printf("\n");
    cur_seed = opts.seed;
    for (cur_run = 0; cur_run < opts.run_cnt; ++cur_run) {
        uint32_t insert_seq[opts.node_cnt];
        uint32_t delete_seq[opts.node_cnt];
        uint32_t sorted_seq[opts.node_cnt];

        printf("Doing run %u with seed %u\n", (cur_run + 1), cur_seed);
        srand(cur_seed);

        for (i = 0; i < opts.node_cnt; ++i) {
            insert_seq[i] = ((rand() % opts.range_end) + opts.range_start);
        }
        permute_array(insert_seq, delete_seq, opts.node_cnt);
        memcpy(sorted_seq, insert_seq, sizeof(sorted_seq));
        qsort(sorted_seq, opts.node_cnt, sizeof(sorted_seq[0]), uint32_t_cmp);
        uniq_cnt = get_unique_count(sorted_seq, opts.node_cnt);

        if (opts.verbosity >= 1) {
            printf("Unique count: %u\n", uniq_cnt);
            printf("Insertion sequence:\n  ");
            for (i = 0; i < opts.node_cnt; ++i) {
                printf(" %u", insert_seq[i]);
            }
            printf("\n");

            printf("Deletion sequence:\n  ");
            for (i = 0; i < opts.node_cnt; ++i) {
                printf(" %u", delete_seq[i]);
            }
            printf("\n");

            printf("Sorted sequence:\n  ");
            for (i = 0; i < opts.node_cnt; ++i) {
                printf(" %u", sorted_seq[i]);
            }
            printf("\n");
        }

        test_input.insert_seq = insert_seq;
        test_input.delete_seq = delete_seq;
        test_input.sorted_seq = sorted_seq;
        test_input.uniq_cnt = uniq_cnt;
        test_input.dup_cnt = (opts.node_cnt - uniq_cnt);
        test_input.opts = &opts;
        test_input.tree_h = NULL;
        test_input.tree_copy_h = NULL;

        was_success = run_mkavl_test(&test_input);
        if (!was_success) {
            printf("FAILURE: the test has failed for seed %u!!!\n", cur_seed);
            ++fail_count;
        }

        ++cur_seed;
    }

    if (0 != fail_count) {
        printf("\n%u/%u TESTS FAILED\n", fail_count, opts.run_cnt);
    } else {
        printf("\nALL TESTS PASSED\n");
    }
    printf("\n");

    return (0);
}

/* AVL operation functions */

/** Magic value for sanity checks */
#define MKAVL_TEST_MAGIC 0x1234ABCD

/**
 * The context storted for a tree.
 */
typedef struct mkavl_test_ctx_st_ {
    /** A sanity check field */
    uint32_t magic;
    /** A count for how many times mkavl_test_copy_fn() was called */
    uint32_t copy_cnt;
    /** A count for how many times mkavl_test_item_fn() was called */
    uint32_t item_fn_cnt;
    /** A count for how many time mkavl_test_copy_malloc() was called */ 
    uint32_t copy_malloc_cnt;
    /** A count for how many time mkavl_test_copy_free() was called */ 
    uint32_t copy_free_cnt;
} mkavl_test_ctx_st;

/**
 * The malloc function for the copied tree.
 *
 * @param size Size of memory to allocate.
 * @param context The tree context.
 * @return A pointer to the memory or NULL if allocation was not possible.
 */
static void *
mkavl_test_copy_malloc (size_t size, void *context)
{
    mkavl_test_ctx_st *ctx = (mkavl_test_ctx_st *) context;

    if ((NULL == ctx) || (MKAVL_TEST_MAGIC != ctx->magic)) {
        abort();
    }

    ++(ctx->copy_malloc_cnt);

    return (malloc(size));
}


/**
 * The free function for the copied tree.
 *
 * @param ptr The memory to free.
 * @param context The tree context.
 */
static void
mkavl_test_copy_free (void *ptr, void *context)
{
    mkavl_test_ctx_st *ctx = (mkavl_test_ctx_st *) context;

    if ((NULL == ctx) || (MKAVL_TEST_MAGIC != ctx->magic)) {
        abort();
    }

    ++(ctx->copy_free_cnt);

    free(ptr);
}

/**
 * Allocators to use for the copied tree.
 */
static mkavl_allocator_st copy_allocator = {
    mkavl_test_copy_malloc,
    mkavl_test_copy_free
};

/**
 * Simply compare the uint32_t values.
 *
 * @param item1 Item to compare
 * @param item2 Item to compare
 * @param context Context for the tree
 * @return Comparison result
 */
static int32_t
mkavl_cmp_fn1 (const void *item1, const void *item2, void *context)
{
    const uint32_t *i1 = item1;
    const uint32_t *i2 = item2;
    mkavl_test_ctx_st *ctx;

    ctx = (mkavl_test_ctx_st *) context;

    if ((NULL == ctx) || (MKAVL_TEST_MAGIC != ctx->magic)) {
        abort();
    }

    if (*i1 < *i2) {
        return (-1);
    } else if (*i1 > *i2) {
        return (1);
    }

    return (0);
}

/**
 * Reverse the values of the items.
 *
 * @param item1 Item to compare
 * @param item2 Item to compare
 * @param context Context for the tree
 * @return Comparison result
 */
static int32_t
mkavl_cmp_fn2 (const void *item1, const void *item2, void *context)
{
    const uint32_t *i1 = item1;
    const uint32_t *i2 = item2;
    mkavl_test_ctx_st *ctx;

    ctx = (mkavl_test_ctx_st *) context;

    if ((NULL == ctx) || (MKAVL_TEST_MAGIC != ctx->magic)) {
        abort();
    }

    if (*i1 > *i2) {
        return (-1);
    } else if (*i1 < *i2) {
        return (1);
    }

    return (0);
}

/**
 * The values for the key ordering.
 */
typedef enum mkavl_test_key_e_ {
    /** Ascending order */
    MKAVL_TEST_KEY_E_ASC,
    /** Descending order */
    MKAVL_TEST_KEY_E_DESC,
    /** Max value for boundary testing */
    MKAVL_TEST_KEY_E_MAX,
} mkavl_test_key_e;

/** The opposite key, used for certain find operations */
static const mkavl_test_key_e mkavl_key_opposite[] = {
    MKAVL_TEST_KEY_E_DESC,
    MKAVL_TEST_KEY_E_ASC,
};

/** The comparison functions to use */
static mkavl_compare_fn cmp_fn_array[] = { mkavl_cmp_fn1 , mkavl_cmp_fn2 };

/** @cond doxygen_suppress */
CT_ASSERT(NELEMS(mkavl_key_opposite) == MKAVL_TEST_KEY_E_MAX);
CT_ASSERT(NELEMS(cmp_fn_array) == MKAVL_TEST_KEY_E_MAX);
/** @endcond */

/**
 * The type to use for find lookups for the key types.
 */
static const mkavl_find_type_e 
mkavl_key_find_type[MKAVL_TEST_KEY_E_MAX][(MKAVL_FIND_TYPE_E_MAX + 1)] = {
    { MKAVL_FIND_TYPE_E_INVALID, MKAVL_FIND_TYPE_E_EQUAL,
      MKAVL_FIND_TYPE_E_GT, MKAVL_FIND_TYPE_E_LT,
      MKAVL_FIND_TYPE_E_GE, MKAVL_FIND_TYPE_E_LE,
      MKAVL_FIND_TYPE_E_MAX },
    { MKAVL_FIND_TYPE_E_INVALID, MKAVL_FIND_TYPE_E_EQUAL,
      MKAVL_FIND_TYPE_E_LT, MKAVL_FIND_TYPE_E_GT,
      MKAVL_FIND_TYPE_E_LE, MKAVL_FIND_TYPE_E_GE,
      MKAVL_FIND_TYPE_E_MAX }
};

/**
 * Test mkavl_new() for error handling.
 * @return True if test passed.
 */
static bool
mkavl_test_new_error (void)
{
    mkavl_rc_e rc;
    mkavl_tree_handle tree_h = NULL;

    if (0 != mkavl_count(NULL)) {
        LOG_FAIL("NULL mkavl_count failed, mkavl_count(%u)", 
                 mkavl_count(NULL));
        return (false);
    }

    rc = mkavl_new(NULL, cmp_fn_array, NELEMS(cmp_fn_array), NULL, NULL);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("NULL tree failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_new(&tree_h, NULL, NELEMS(cmp_fn_array), NULL, NULL);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("NULL function failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_new(&tree_h, cmp_fn_array, 0, NULL, NULL);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("zero size function failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    return (true);
}

/**
 * Test mkavl_new().
 *
 * @param input The input state for the test.
 * @param allocator The allocator to use.
 * @return True if test passed.
 */
static bool
mkavl_test_new (mkavl_test_input_st *input, mkavl_allocator_st *allocator)
{
    mkavl_rc_e rc;
    mkavl_test_ctx_st *ctx;

    ctx = calloc(1, sizeof(*ctx));
    if (NULL == ctx) {
        LOG_FAIL("calloc failed");
        return (false);
    }
    ctx->magic = MKAVL_TEST_MAGIC;

    rc = mkavl_new(&(input->tree_h), cmp_fn_array, NELEMS(cmp_fn_array),
                   ctx, allocator);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("new failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    return (true);
}

/**
 * The callback for freeing the context memory.
 *
 * @param context The context to free.
 * @return The return code
 */
static mkavl_rc_e
mkavl_test_delete_context (void *context)
{
    mkavl_test_ctx_st *ctx = (mkavl_test_ctx_st *) context;

    if ((NULL == ctx) || (MKAVL_TEST_MAGIC != ctx->magic)) {
        return (MKAVL_RC_E_EINVAL);
    }

    ctx->magic = 0;
    free(context);

    return (MKAVL_RC_E_SUCCESS);
}

/**
 * Test mkavl_delete().
 *
 * @param input The input state for the test.
 * @param item_fn The function to apply to deleted items.
 * @param delete_context_fn The function to apply to the tree's context.
 * @param delete_copy_context_fn The function to apply to the copied tree's
 * context.
 * @return True if test passed.
 */
static bool
mkavl_test_delete (mkavl_test_input_st *input, mkavl_item_fn item_fn,
                   mkavl_delete_context_fn delete_context_fn,
                   mkavl_delete_context_fn delete_copy_context_fn)
{
    mkavl_rc_e rc;

    if (NULL != input->tree_h) {
        rc = mkavl_delete(&(input->tree_h), item_fn, delete_context_fn);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("delete failed, rc(%s)", mkavl_rc_e_get_string(rc));
            return (false);
        }
    }

    if (NULL != input->tree_copy_h) {
        rc = mkavl_delete(&(input->tree_copy_h), item_fn, 
                          delete_copy_context_fn);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("delete failed, rc(%s)", mkavl_rc_e_get_string(rc));
            return (false);
        }
    }

    return (true);
}

/**
 * Test mkavl_add() for error handling.
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_add_error (mkavl_test_input_st *input)
{
    uint32_t *existing_item;
    mkavl_rc_e rc;

    rc = mkavl_add(input->tree_h, &(input->insert_seq[0]), (void **) NULL);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("NULL existing item failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_add(input->tree_h, NULL, (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("NULL item failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_add(NULL, &(input->insert_seq[0]), (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("NULL tree failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    return (true);
}

/**
 * Test mkavl_add().
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_add (mkavl_test_input_st *input)
{
    uint32_t i;
    uint32_t non_null_cnt = 0;
    uint32_t *existing_item;
    mkavl_rc_e rc;

    for (i = 0; i < input->opts->node_cnt; ++i) {
        rc = mkavl_add(input->tree_h, &(input->insert_seq[i]), 
                       (void **) &existing_item);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("add failed, rc(%s)", mkavl_rc_e_get_string(rc));
            return (false);
        }

        if (NULL != existing_item) {
            ++non_null_cnt;
        }
    }

    if (non_null_cnt != input->dup_cnt) {
        LOG_FAIL("duplicate check failed, non_null_cnt(%u) dup_cnt(%u)", 
                 non_null_cnt, input->dup_cnt);
        return (false);
    }

    if (mkavl_count(input->tree_h) != input->uniq_cnt) {
        LOG_FAIL("unique check failed, mkavl_count(%u) uniq_cnt(%u)", 
                 mkavl_count(input->tree_h), input->uniq_cnt);
        return (false);
    }

    return (true);
}

/**
 * Test mkavl_find().
 *
 * @param input The input state for the test.
 * @param val The value on which to search.
 * @param type The type of find.
 * @return True if test passed.
 */
static uint32_t *
mkavl_test_find_val (mkavl_test_input_st *input,
                     uint32_t val, mkavl_find_type_e type)
{
    int32_t lo, hi, mid;
    uint32_t runaway_counter = 0;
    bool is_equal_type, is_greater_type, is_less_type;
    size_t num_elems = input->opts->node_cnt;

    if (0 == num_elems) {
        return (NULL);
    }
    
    if (!mkavl_find_type_e_is_valid(type)) {
        return (NULL);
    }

    is_equal_type = ((MKAVL_FIND_TYPE_E_EQUAL == type) ||
                     (MKAVL_FIND_TYPE_E_GE == type) ||
                     (MKAVL_FIND_TYPE_E_LE == type));

    is_greater_type = ((MKAVL_FIND_TYPE_E_GT == type) ||
                       (MKAVL_FIND_TYPE_E_GE == type));

    is_less_type = ((MKAVL_FIND_TYPE_E_LT == type) ||
                    (MKAVL_FIND_TYPE_E_LE == type));

    lo = 0; 
    hi = (num_elems - 1);
    while (lo <= hi) {
        mid = (lo + ((hi - lo) / 2));
        if (input->opts->verbosity >= 6) {
            printf("lo(%d) hi(%d) mid(%d) val(%u) arrayval(%u) type(%s)\n",
                   lo, hi, mid, val, input->sorted_seq[mid],
                   mkavl_find_type_e_get_string(type));
        }
        if (is_equal_type && (input->sorted_seq[mid] == val)) {
            return &(input->sorted_seq[mid]);
        }

        if (is_greater_type && ((val >= input->sorted_seq[mid]) || 
                                (0 == mid))) {
            if ((mid + 1) >= num_elems) {
                return (NULL);
            }

            if ((0 == mid) && (val < input->sorted_seq[mid])) {
                return &(input->sorted_seq[mid]);
            }

            if (val < input->sorted_seq[mid + 1]) {
                return &(input->sorted_seq[mid + 1]);
            }
        }

        if (is_less_type && ((val <= input->sorted_seq[mid]) ||
                             (mid == (num_elems - 1)))) {
            if (0 == mid) {
                return (NULL);
            }

            if ((mid == (num_elems - 1)) && (val > input->sorted_seq[mid])) {
                return &(input->sorted_seq[mid]);
            }

            if (val > input->sorted_seq[mid - 1]) {
                return &(input->sorted_seq[mid - 1]);
            }
        }

        if (input->sorted_seq[mid] == val) {
            if (is_greater_type) {
                lo = (mid + 1);
            }

            if (is_less_type) {
                hi = (mid - 1);
            }
        } else if (val > input->sorted_seq[mid]) {
            lo = (mid + 1);
        } else {
            hi = (mid - 1);
            if (is_greater_type) {
                hi = mid;
            }
        }

        if (++runaway_counter > MKAVL_TEST_RUNAWAY_SANITY) {
            abort();
        }
    }

    if (input->opts->verbosity >= 6) {
        printf("lo(%d) hi(%d)\n", lo, hi);
    }

    return (NULL);
}

/**
 * Test mkavl_find().
 *
 * @param input The input state for the test.
 * @param type The type of find.
 * @return True if test passed.
 */
static bool
mkavl_test_find (mkavl_test_input_st *input, mkavl_find_type_e type)
{
    uint32_t *existing_item, *array_item;
    uint32_t rand_lookup_val;
    uint32_t i, j;
    mkavl_rc_e rc;
    bool is_equal_type;

    is_equal_type = ((MKAVL_FIND_TYPE_E_EQUAL == type) ||
                     (MKAVL_FIND_TYPE_E_GE == type) ||
                     (MKAVL_FIND_TYPE_E_LE == type));

    for (i = 0; i < input->opts->node_cnt; ++i) {
        for (j = 0; j < MKAVL_TEST_KEY_E_MAX; ++j) {
            /* Do the operation on an existing item */
            rc = mkavl_find(input->tree_h, mkavl_key_find_type[j][type], j,
                            &(input->insert_seq[i]), (void **) &existing_item);
            if (mkavl_rc_e_is_notok(rc)) {
                LOG_FAIL("find failed, rc(%s)", mkavl_rc_e_get_string(rc));
                return (false);
            }

            if (is_equal_type) {
                if (NULL == existing_item) {
                    LOG_FAIL("find failed for %u, type %s", 
                             input->insert_seq[i],
                             mkavl_find_type_e_get_string(type));
                    return (false);
                }

                if (*existing_item != input->insert_seq[i]) {
                    LOG_FAIL("find failed for %u, found %u type %s",
                             input->insert_seq[i],
                             *existing_item,
                             mkavl_find_type_e_get_string(type));
                    return (false);
                }
            }

            /* 
             * Make sure what we founds matches a binary search on
             * the sorted array.
             */
            array_item = mkavl_test_find_val(input, input->insert_seq[i], type);
            if (((NULL == existing_item) && (NULL != array_item)) ||
                ((NULL != existing_item) && (NULL == array_item)) ||
                ((NULL != existing_item) && (NULL != array_item) &&
                 (*existing_item != *array_item))) {
                LOG_FAIL("mismatch in array and AVL find for %u, AVL(%p) %u "
                         "array(%p) %u type %s key %u",
                         input->insert_seq[i],
                         existing_item,
                         (NULL == existing_item) ? 0 : *existing_item, 
                         array_item,
                         (NULL == array_item) ? 0 : *array_item, 
                         mkavl_find_type_e_get_string(type), j);
                return (false);
            }

            if (input->opts->verbosity >= 5) {
                printf("find for type %s and key %u for %u, AVL(%p) %u "
                       "array(%p) %u\n",
                       mkavl_find_type_e_get_string(type),
                       j, input->insert_seq[i],
                       existing_item,
                       (NULL == existing_item) ? 0 : *existing_item, 
                       array_item,
                       (NULL == array_item) ? 0 : *array_item);
            }

            /* Do the operation on a (potentially) non-existing item */
            rand_lookup_val = ((rand() % input->opts->range_end) + 
                                input->opts->range_start);
            rc = mkavl_find(input->tree_h, mkavl_key_find_type[j][type], j,
                            &rand_lookup_val, (void **) &existing_item);
            if (mkavl_rc_e_is_notok(rc)) {
                LOG_FAIL("find failed, rc(%s)", mkavl_rc_e_get_string(rc));
                return (false);
            }

            array_item = mkavl_test_find_val(input, rand_lookup_val, type);
            if (((NULL == existing_item) && (NULL != array_item)) ||
                ((NULL != existing_item) && (NULL == array_item)) ||
                ((NULL != existing_item) && (NULL != array_item) &&
                 (*existing_item != *array_item))) {
                LOG_FAIL("mismatch in array and AVL find for %u, AVL(%p) %u "
                         "array(%p) %u type %s key %u",
                         rand_lookup_val,
                         existing_item,
                         (NULL == existing_item) ? 0 : *existing_item, 
                         array_item,
                         (NULL == array_item) ? 0 : *array_item, 
                         mkavl_find_type_e_get_string(type), j);
                return (false);
            }

            if (input->opts->verbosity >= 5) {
                printf("find for type %s and key %u for %u, AVL(%p) %u "
                       "array(%p) %u\n",
                       mkavl_find_type_e_get_string(type),
                       j, rand_lookup_val,
                       existing_item,
                       (NULL == existing_item) ? 0 : *existing_item, 
                       array_item,
                       (NULL == array_item) ? 0 : *array_item);
            }

        }
    }
    
    return (true);
}

/**
 * Test mkavl_find() for error handling.
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_find_error (mkavl_test_input_st *input)
{
    uint32_t *existing_item;
    mkavl_rc_e rc;

    rc = mkavl_find(NULL, MKAVL_FIND_TYPE_E_EQUAL, MKAVL_TEST_KEY_E_ASC,
                    &(input->insert_seq[0]), (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("NULL tree failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_find(input->tree_h, (MKAVL_FIND_TYPE_E_MAX + 1), 
                    MKAVL_TEST_KEY_E_ASC,
                    &(input->insert_seq[0]), (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Invalid type failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_find(input->tree_h, MKAVL_FIND_TYPE_E_EQUAL, 
                    MKAVL_TEST_KEY_E_MAX,
                    &(input->insert_seq[0]), (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Invalid key index failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_find(input->tree_h, MKAVL_FIND_TYPE_E_EQUAL,
                    MKAVL_TEST_KEY_E_ASC, NULL, (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("NULL item failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_find(input->tree_h, MKAVL_FIND_TYPE_E_EQUAL,
                    MKAVL_TEST_KEY_E_ASC, &(input->insert_seq[0]), NULL);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("NULL item failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    return (true);
}

/**
 * Test mkavl_remove_key() and mkavl_add_key().
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_add_remove_key (mkavl_test_input_st *input)
{
    uint32_t i, j;
    uint32_t non_null_cnt, null_cnt;
    uint32_t *existing_item;
    mkavl_rc_e rc;

    for (i = 0; i < MKAVL_TEST_KEY_E_MAX; ++i) {
        /* Take them all out for one key */
        non_null_cnt = 0;
        for (j = 0; j < input->opts->node_cnt; ++j) {
            rc = mkavl_remove_key_idx(input->tree_h, i, 
                                      &(input->delete_seq[j]), 
                                      (void **) &existing_item);
            if (mkavl_rc_e_is_notok(rc)) {
                LOG_FAIL("remove key idx failed, rc(%s)", 
                         mkavl_rc_e_get_string(rc));
                return (false);
            }

            if (NULL != existing_item) {
                ++non_null_cnt;
            }

            rc = mkavl_find(input->tree_h, MKAVL_FIND_TYPE_E_EQUAL, i,
                            &(input->delete_seq[j]), (void **) &existing_item);
            if (mkavl_rc_e_is_notok(rc)) {
                LOG_FAIL("find failed, rc(%s)", mkavl_rc_e_get_string(rc));
                return (false);
            }

            if (NULL != existing_item) {
                LOG_FAIL("found item expected to be deleted, %u", 
                         input->delete_seq[j]);
                return (false);
            }

            rc = mkavl_find(input->tree_h, MKAVL_FIND_TYPE_E_EQUAL, 
                            mkavl_key_opposite[i],
                            &(input->delete_seq[j]), (void **) &existing_item);
            if (mkavl_rc_e_is_notok(rc)) {
                LOG_FAIL("find failed, rc(%s)", mkavl_rc_e_get_string(rc));
                return (false);
            }

            if (NULL == existing_item) {
                LOG_FAIL("did not find item, %u", input->delete_seq[j]);
                return (false);
            }
        }

        if (non_null_cnt != input->uniq_cnt) {
            LOG_FAIL("unique check failed, non_null_cnt(%u) uniq_cnt(%u)", 
                     non_null_cnt, input->uniq_cnt);
            return (false);
        }

        /* Tree count should remain unchanged */
        if (mkavl_count(input->tree_h) != input->uniq_cnt) {
            LOG_FAIL("unique check failed, mkavl_count(%u) uniq_cnt(%u)", 
                     mkavl_count(input->tree_h), input->uniq_cnt);
            return (false);
        }

        /* Put them all back in for the key */
        null_cnt = 0;
        for (j = 0; j < input->opts->node_cnt; ++j) {
            rc = mkavl_add_key_idx(input->tree_h, i, &(input->insert_seq[j]), 
                                   (void **) &existing_item);
            if (mkavl_rc_e_is_notok(rc)) {
                LOG_FAIL("add key idx failed, rc(%s)", 
                         mkavl_rc_e_get_string(rc));
                return (false);
            }

            if (NULL == existing_item) {
                ++null_cnt;
            }
        }

        if (null_cnt != input->uniq_cnt) {
            LOG_FAIL("unique check failed, null_cnt(%u) uniq_cnt(%u)", 
                     null_cnt, input->uniq_cnt);
            return (false);
        }

        /* Tree count should remain unchanged */
        if (mkavl_count(input->tree_h) != input->uniq_cnt) {
            LOG_FAIL("unique check failed, mkavl_count(%u) uniq_cnt(%u)", 
                     mkavl_count(input->tree_h), input->uniq_cnt);
            return (false);
        }
    }

    return (true);
}

/**
 * Test mkavl_add_key() for error handling.
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_add_key_error (mkavl_test_input_st *input)
{
    uint32_t *existing_item;
    mkavl_rc_e rc;

    rc = mkavl_add_key_idx(NULL, MKAVL_TEST_KEY_E_ASC,
                           &(input->insert_seq[0]), (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Key index operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_add_key_idx(input->tree_h, MKAVL_TEST_KEY_E_MAX,
                           &(input->insert_seq[0]), (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Key index operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_add_key_idx(input->tree_h, MKAVL_TEST_KEY_E_ASC,
                           NULL, (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Key index operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_add_key_idx(input->tree_h, MKAVL_TEST_KEY_E_ASC,
                           &(input->insert_seq[0]), NULL);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Key index operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    return (true);
}

/**
 * Test mkavl_remove_key() for error handling.
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_remove_key_error (mkavl_test_input_st *input)
{
    uint32_t *existing_item;
    mkavl_rc_e rc;

    rc = mkavl_remove_key_idx(NULL, MKAVL_TEST_KEY_E_ASC,
                           &(input->insert_seq[0]), (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Key index operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_remove_key_idx(input->tree_h, MKAVL_TEST_KEY_E_MAX,
                           &(input->insert_seq[0]), (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Key index operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_remove_key_idx(input->tree_h, MKAVL_TEST_KEY_E_ASC,
                           NULL, (void **) &existing_item);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Key index operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    rc = mkavl_remove_key_idx(input->tree_h, MKAVL_TEST_KEY_E_ASC,
                           &(input->insert_seq[0]), NULL);
    if (mkavl_rc_e_is_ok(rc)) {
        LOG_FAIL("Key index operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        return (false);
    }

    return (true);
}

/**
 * The callback for mkavl_copy().
 *
 * @param item The original item being copied.
 * @param context The context for the tree.
 * @return The item to place in the copied tree.
 */
static void *
mkavl_test_copy_fn (void *item, void *context)
{
    mkavl_test_ctx_st *ctx;

    ctx = (mkavl_test_ctx_st *) context;

    if ((NULL == ctx) || (MKAVL_TEST_MAGIC != ctx->magic)) {
        abort();
    }
    ++(ctx->copy_cnt);

    return (item);
}

/**
 * Test mkavl_copy().
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_copy (mkavl_test_input_st *input)
{
    mkavl_rc_e rc;
    mkavl_test_ctx_st *ctx, *src_ctx;

    src_ctx = mkavl_get_tree_context(input->tree_h);
    if (NULL == src_ctx) {
        LOG_FAIL("NULL context pointer");
        return (false);
    }

    ctx = calloc(1, sizeof(*ctx));
    if (NULL == ctx) {
        LOG_FAIL("calloc failed");
        return (false);
    }
    ctx->magic = MKAVL_TEST_MAGIC;

    rc = mkavl_copy(input->tree_h,
                    &(input->tree_copy_h),
                    mkavl_test_copy_fn, NULL, false, ctx,
                    mkavl_test_delete_context,
                    &copy_allocator);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("copy failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    if (src_ctx->copy_cnt != input->uniq_cnt) {
        LOG_FAIL("unexpected copy count, copy count %u "
                 "unique count %u)", ctx->copy_cnt, input->uniq_cnt);
        return (false);
    }

    if (mkavl_count(input->tree_h) != mkavl_count(input->tree_copy_h)) {
        LOG_FAIL("unequal count after copy, original %u copy %u)", 
                 mkavl_count(input->tree_h), mkavl_count(input->tree_copy_h));
        return (false);
    }

    return (true);
}

/**
 * Test mkavl iterators.
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_iterator (mkavl_test_input_st *input)
{
    mkavl_iterator_handle iter1_h = NULL, iter2_h = NULL;
    mkavl_iterator_handle copy_iter1_h = NULL;
    uint32_t last_idx = (input->opts->node_cnt - 1);
    uint32_t *item, *copy_item, *cur_item, *prev_item, *found_item;
    uint32_t idx;
    mkavl_rc_e rc;
    bool retval = true;

    /* 
     * Iterate over both trees, make sure order is the same, everything is less
     * than in one iterator and greater than in the other.
     */
    rc = mkavl_iter_new(&iter1_h, input->tree_h, MKAVL_TEST_KEY_E_ASC);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("new iterator failed, rc(%s)", mkavl_rc_e_get_string(rc));
        retval = false;
        goto cleanup;
    }

    rc = mkavl_iter_new(&iter2_h, input->tree_h, MKAVL_TEST_KEY_E_DESC);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("new iterator failed, rc(%s)", mkavl_rc_e_get_string(rc));
        retval = false;
        goto cleanup;
    }

    rc = mkavl_iter_new(&copy_iter1_h, input->tree_copy_h,
                        MKAVL_TEST_KEY_E_ASC);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("new iterator failed, rc(%s)", mkavl_rc_e_get_string(rc));
        retval = false;
        goto cleanup;
    }

    rc = mkavl_iter_last(iter1_h, (void **) &item);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("iterator operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        retval = false;
        goto cleanup;
    }

    if (*item != input->sorted_seq[last_idx]) {
        LOG_FAIL("iterator item value mismatch, item %u array val %u",
                 *item, input->sorted_seq[last_idx]);
        retval = false;
        goto cleanup;
    }

    rc = mkavl_iter_last(iter2_h, (void **) &item);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("iterator operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        retval = false;
        goto cleanup;
    }

    if (*item != input->sorted_seq[0]) {
        LOG_FAIL("iterator item value mismatch, item %u array val %u",
                 *item, input->sorted_seq[0]);
        retval = false;
        goto cleanup;
    }

    rc = mkavl_iter_first(iter2_h, (void **) &item);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("iterator operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        retval = false;
        goto cleanup;
    }

    if (*item != input->sorted_seq[last_idx]) {
        LOG_FAIL("iterator item value mismatch, item %u array val %u",
                 *item, input->sorted_seq[last_idx]);
        retval = false;
        goto cleanup;
    }

    rc = mkavl_iter_first(iter1_h, (void **) &item);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("iterator operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        retval = false;
        goto cleanup;
    }

    if (*item != input->sorted_seq[0]) {
        LOG_FAIL("iterator item value mismatch, item %u array val %u",
                 *item, input->sorted_seq[0]);
        retval = false;
        goto cleanup;
    }

    rc = mkavl_iter_first(copy_iter1_h, (void **) &copy_item);
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("iterator operation failed, rc(%s)",
                 mkavl_rc_e_get_string(rc));
        retval = false;
        goto cleanup;
    }

    idx = 0;
    prev_item = NULL;
    while ((NULL != item) && (NULL != copy_item)) {
        if (idx >= input->opts->node_cnt) {
            LOG_FAIL("invalid idx(%u), node_cnt(%u)", idx,
                     input->opts->node_cnt);
            retval = false;
            goto cleanup;
        }

        if (*item != *copy_item) {
            LOG_FAIL("iterator has mismatch, item %u copy_item %u", *item,
                     *copy_item);
            retval = false;
            goto cleanup;
        }

        if (*item != input->sorted_seq[idx]) {
            LOG_FAIL("iterator has mismatch, item %u sorted_seq %u", *item,
                     input->sorted_seq[idx]);
            retval = false;
            goto cleanup;
        }

        /* Go to the next unique value in the sorted array */
        while ((idx < input->opts->node_cnt) &&
               (*item == input->sorted_seq[idx])) {
            ++idx;
        }

        /* Test the current function */
        rc = mkavl_iter_cur(iter1_h, (void **) &cur_item);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("iterator operation failed, rc(%s)",
                     mkavl_rc_e_get_string(rc));
            retval = false;
            goto cleanup;
        }

        if (item != cur_item) {
            LOG_FAIL("iterator has mismatch, item %p cur_item %p", item,
                     cur_item);
            retval = false;
            goto cleanup;
        }

        /* Test previous */
        rc = mkavl_iter_prev(iter1_h, (void **) &item);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("iterator operation failed, rc(%s)",
                     mkavl_rc_e_get_string(rc));
            retval = false;
            goto cleanup;
        }

        if (prev_item != item) {
            LOG_FAIL("iterator has mismatch, item %p prev_item %p", item,
                     prev_item);
            retval = false;
            goto cleanup;
        }

        /* Test find */
        rc = mkavl_iter_find(iter1_h, cur_item, (void **) &found_item);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("iterator operation failed, rc(%s)",
                     mkavl_rc_e_get_string(rc));
            retval = false;
            goto cleanup;
        }

        if (found_item != cur_item) {
            LOG_FAIL("iterator has mismatch, found_item %p cur_item %p", 
                     found_item, cur_item);
            retval = false;
            goto cleanup;
        }

        rc = mkavl_iter_next(iter1_h, (void **) &item);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("iterator operation failed, rc(%s)",
                     mkavl_rc_e_get_string(rc));
            retval = false;
            goto cleanup;
        }

        rc = mkavl_iter_next(copy_iter1_h, (void **) &copy_item);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("iterator operation failed, rc(%s)",
                     mkavl_rc_e_get_string(rc));
            retval = false;
            goto cleanup;
        }

        prev_item = cur_item;
    }

    if (item != copy_item) {
        LOG_FAIL("iterator has mismatch, item %p copy_item %p", item,
                 copy_item);
        retval = false;
        goto cleanup;
    }

cleanup:

    if (NULL != iter1_h) {
        mkavl_iter_delete(&iter1_h);
    }

    if (NULL != iter2_h) {
        mkavl_iter_delete(&iter2_h);
    }

    if (NULL != copy_iter1_h) {
        mkavl_iter_delete(&copy_iter1_h);
    }

    return (retval);
}

/** The context for mkavl_walk() */
typedef struct mkavl_test_walk_ctx_st_ {
    /** Magic value for sanity check */
    uint32_t magic;
    /** A count of the number of nodes walked */
    uint32_t walk_node_cnt;
    /** Tells when the walk should be stopped */
    uint32_t walk_stop_cnt;
} mkavl_test_walk_ctx_st;

/**
 * The callback for mkavl_walk().
 *
 * @param item The current item in the walk.
 * @param tree_context The tree's context.
 * @param walk_context The walks' context.
 * @param stop_walk Can be set true to stop the walk upon return.
 * @return The return code
 */
static mkavl_rc_e
mkavl_test_walk_cb (void *item, void *tree_context, void *walk_context,
                    bool *stop_walk)
{
    mkavl_test_walk_ctx_st *walk_ctx = (mkavl_test_walk_ctx_st *) walk_context;
    mkavl_test_ctx_st *tree_ctx = (mkavl_test_ctx_st *) tree_context;

    if ((NULL == item) || (NULL == walk_ctx) || (NULL == tree_ctx) ||
        (NULL == stop_walk) || (MKAVL_TEST_MAGIC != walk_ctx->magic) ||
        (MKAVL_TEST_MAGIC != tree_ctx->magic)) {
        abort();
    }

    if (walk_ctx->walk_stop_cnt == walk_ctx->walk_node_cnt) {
        *stop_walk = true;
    } else {
        ++(walk_ctx->walk_node_cnt);
    }

    return (MKAVL_RC_E_SUCCESS);
}

/**
 * Test mkavl_walk().
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_walk (mkavl_test_input_st *input)
{
    mkavl_test_walk_ctx_st walk_ctx = {0};
    mkavl_rc_e rc;
    
    walk_ctx.magic = MKAVL_TEST_MAGIC;
    /* Set it high enough that this walk will go all the way */
    walk_ctx.walk_stop_cnt = input->uniq_cnt;
    rc = mkavl_walk(input->tree_h, mkavl_test_walk_cb, &walk_ctx); 
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("walk failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    if (walk_ctx.walk_node_cnt != walk_ctx.walk_stop_cnt) {
        LOG_FAIL("unexpected walk count, walk_node_cnt(%u) stop_cnt(%u)", 
                 walk_ctx.walk_node_cnt, walk_ctx.walk_stop_cnt);
        return (false);
    }

    walk_ctx.walk_node_cnt = 0;
    /* Set it high enough that this walk will go all the way */
    walk_ctx.walk_stop_cnt = (rand() %  input->uniq_cnt);
    rc = mkavl_walk(input->tree_copy_h, mkavl_test_walk_cb, &walk_ctx); 
    if (mkavl_rc_e_is_notok(rc)) {
        LOG_FAIL("walk failed, rc(%s)", mkavl_rc_e_get_string(rc));
        return (false);
    }

    if (walk_ctx.walk_node_cnt != walk_ctx.walk_stop_cnt) {
        LOG_FAIL("unexpected walk count, walk_node_cnt(%u) stop_cnt(%u)", 
                 walk_ctx.walk_node_cnt, walk_ctx.walk_stop_cnt);
        return (false);
    }

    walk_ctx.magic = 0;

    return (true);
}

/**
 * Test mkavl_remove().
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
mkavl_test_remove (mkavl_test_input_st *input)
{
    uint32_t i, null_cnt = 0;
    mkavl_rc_e rc;
    uint32_t *found_item;

    for (i = 0; i < input->opts->node_cnt; ++i) {
        rc = mkavl_remove(input->tree_h, &(input->delete_seq[i]), 
                          (void **) &found_item);
        if (mkavl_rc_e_is_notok(rc)) {
            LOG_FAIL("remove failed, rc(%s)",
                     mkavl_rc_e_get_string(rc));
            return (false);
        }

        if (NULL == found_item) {
            ++null_cnt;
        }
    }

    if (null_cnt != input->dup_cnt) {
        LOG_FAIL("duplicate check failed, null_cnt(%u) dup_cnt(%u)", 
                 null_cnt, input->dup_cnt);
        return (false);
    }

    if (0 != mkavl_count(input->tree_h)) {
        LOG_FAIL("remove count check failed, count(%u)",
                 mkavl_count(input->tree_h));
        return (false);
    }

    return (true);
}

/**
 * The callback for per-item functions.
 *
 * @param item The current item.
 * @param context The tree's context.
 * @return The return code
 */
static mkavl_rc_e
mkavl_test_item_fn (void *item, void *context)
{
    mkavl_test_ctx_st *ctx = (mkavl_test_ctx_st *) context;

    if ((NULL == item) || (NULL == ctx) || 
        (MKAVL_TEST_MAGIC != ctx->magic)) {
        abort();
    }

    ++(ctx->item_fn_cnt);

    return (MKAVL_RC_E_SUCCESS);
}

/**
 * Runs all of the tests.
 *
 * @param input The input state for the test.
 * @return True if test passed.
 */
static bool
run_mkavl_test (mkavl_test_input_st *input)
{
    mkavl_find_type_e find_type;
    mkavl_test_ctx_st *ctx;
    bool test_rc;

    if (NULL == input) {
        LOG_FAIL("invalid input");
        return (false);
    }

    test_rc = mkavl_test_new(input, NULL);
    if (!test_rc) {
        goto err_exit;
    }

    /* Destroy an empty tree */
    test_rc = mkavl_test_delete(input, mkavl_test_item_fn,
                                mkavl_test_delete_context,
                                mkavl_test_delete_context);
    if (!test_rc) {
        goto err_exit;
    }

    test_rc = mkavl_test_new(input, NULL);
    if (!test_rc) {
        goto err_exit;
    }

    /* Test new error input */
    test_rc = mkavl_test_new_error();
    if (!test_rc) {
        goto err_exit;
    }

    /* Add in all the items */
    test_rc = mkavl_test_add(input);
    if (!test_rc) {
        goto err_exit;
    }

    /* Test add error input */
    test_rc = mkavl_test_add_error(input);
    if (!test_rc) {
        goto err_exit;
    }

    /* Test all types of find */
    for (find_type = MKAVL_FIND_TYPE_E_FIRST; find_type < MKAVL_FIND_TYPE_E_MAX;
         ++find_type) {
        test_rc = mkavl_test_find(input, find_type);
        if (!test_rc) {
            goto err_exit;
        }
    }

    /* Test find error input */
    test_rc = mkavl_test_find_error(input);
    if (!test_rc) {
        goto err_exit;
    }

    /* Test find and add/remove from key */
    test_rc = mkavl_test_add_remove_key(input);
    if (!test_rc) {
        goto err_exit;
    }

    /* Test add/remove idx error conditions */
    test_rc = mkavl_test_add_key_error(input);
    if (!test_rc) {
        goto err_exit;
    }

    test_rc = mkavl_test_remove_key_error(input);
    if (!test_rc) {
        goto err_exit;
    }

    /* Test copying a tree */
    test_rc = mkavl_test_copy(input);
    if (!test_rc) {
        goto err_exit;
    }

    /* Test iterators */
    test_rc = mkavl_test_iterator(input);
    if (!test_rc) {
        goto err_exit;
    }

    /* Do walk over trees */
    test_rc = mkavl_test_walk(input);
    if (!test_rc) {
        goto err_exit;
    }

    /* 
     * Remove items from the original tree, let the items remain in the copied
     * tree so mkavl_delete handles them.
     */
    test_rc = mkavl_test_remove(input);
    if (!test_rc) {
        goto err_exit;
    }

    ctx = mkavl_get_tree_context(input->tree_copy_h);
    if (NULL == ctx) {
        LOG_FAIL("NULL context");
        goto err_exit;
    }

    /* 
     * Destroy both trees: make sure the delete function is called as expected
     * for the copied tree.
     */
    test_rc = mkavl_test_delete(input, mkavl_test_item_fn,
                                mkavl_test_delete_context, NULL);
    if (!test_rc) {
        goto err_exit;
    }

    if (ctx->item_fn_cnt != input->uniq_cnt) {
        LOG_FAIL("item fn count(%u) != uniq count(%u)", ctx->item_fn_cnt,
                 input->uniq_cnt);
        goto err_exit;
    }

    if (ctx->copy_malloc_cnt != ctx->copy_free_cnt) {
        LOG_FAIL("malloc count(%u) != free count(%u)", 
                 ctx->copy_malloc_cnt, ctx->copy_free_cnt);
        goto err_exit;
    }

    free(ctx);
    ctx = NULL;

    return (true);

err_exit:

    mkavl_test_delete(input, mkavl_test_item_fn, mkavl_test_delete_context,
                      mkavl_test_delete_context);

    return (false);
}
