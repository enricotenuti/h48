#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "cube.h"

#ifdef DEBUG

#include <stdio.h>
#define _static
#define _static_inline
#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)
#define DBG_WARN(condition, ...) if (!(condition)) DBG_LOG(__VA_ARGS__);
#define DBG_ASSERT(condition, retval, ...) \
	if (!(condition)) {                \
		DBG_LOG(__VA_ARGS__);      \
		return retval;             \
	}

#else

#define _static static
#define _static_inline static inline
#define DBG_LOG(...)
#define DBG_WARN(condition, ...)
#define DBG_ASSERT(condition, retval, ...)

#endif

/******************************************************************************
Section: mathematical constants
******************************************************************************/

#define _2p11  2048U
#define _2p12  4096U
#define _3p7   2187U
#define _3p8   6561U
#define _12c4  495U
#define _8c4   70U

#define COCSEP_CLASSES 3393U

_static int64_t binomial[12][12] = {
	{1,  0,  0,   0,   0,   0,   0,   0,   0,  0,  0, 0},
	{1,  1,  0,   0,   0,   0,   0,   0,   0,  0,  0, 0},
	{1,  2,  1,   0,   0,   0,   0,   0,   0,  0,  0, 0},
	{1,  3,  3,   1,   0,   0,   0,   0,   0,  0,  0, 0},
	{1,  4,  6,   4,   1,   0,   0,   0,   0,  0,  0, 0},
	{1,  5, 10,  10,   5,   1,   0,   0,   0,  0,  0, 0},
	{1,  6, 15,  20,  15,   6,   1,   0,   0,  0,  0, 0},
	{1,  7, 21,  35,  35,  21,   7,   1,   0,  0,  0, 0},
	{1,  8, 28,  56,  70,  56,  28,   8,   1,  0,  0, 0},
	{1,  9, 36,  84, 126, 126,  84,  36,   9,  1,  0, 0},
	{1, 10, 45, 120, 210, 252, 210, 120,  45, 10,  1, 0},
	{1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1},
};

/******************************************************************************
Section: moves definitions and tables
******************************************************************************/

#define _move_U  0U
#define _move_U2 1U
#define _move_U3 2U
#define _move_D  3U
#define _move_D2 4U
#define _move_D3 5U
#define _move_R  6U
#define _move_R2 7U
#define _move_R3 8U
#define _move_L  9U
#define _move_L2 10U
#define _move_L3 11U
#define _move_F  12U
#define _move_F2 13U
#define _move_F3 14U
#define _move_B  15U
#define _move_B2 16U
#define _move_B3 17U

#define _trans_UFr 0
#define _trans_ULr 1
#define _trans_UBr 2
#define _trans_URr 3
#define _trans_DFr 4
#define _trans_DLr 5
#define _trans_DBr 6
#define _trans_DRr 7
#define _trans_RUr 8
#define _trans_RFr 9
#define _trans_RDr 10
#define _trans_RBr 11
#define _trans_LUr 12
#define _trans_LFr 13
#define _trans_LDr 14
#define _trans_LBr 15
#define _trans_FUr 16
#define _trans_FRr 17
#define _trans_FDr 18
#define _trans_FLr 19
#define _trans_BUr 20
#define _trans_BRr 21
#define _trans_BDr 22
#define _trans_BLr 23

#define _trans_UFm 24
#define _trans_ULm 25
#define _trans_UBm 26
#define _trans_URm 27
#define _trans_DFm 28
#define _trans_DLm 29
#define _trans_DBm 30
#define _trans_DRm 31
#define _trans_RUm 32
#define _trans_RFm 33
#define _trans_RDm 34
#define _trans_RBm 35
#define _trans_LUm 36
#define _trans_LFm 37
#define _trans_LDm 38
#define _trans_LBm 39
#define _trans_FUm 40
#define _trans_FRm 41
#define _trans_FDm 42
#define _trans_FLm 43
#define _trans_BUm 44
#define _trans_BRm 45
#define _trans_BDm 46
#define _trans_BLm 47

#define _c_ufr      0U
#define _c_ubl      1U
#define _c_dfl      2U
#define _c_dbr      3U
#define _c_ufl      4U
#define _c_ubr      5U
#define _c_dfr      6U
#define _c_dbl      7U

#define _e_uf       0U
#define _e_ub       1U
#define _e_db       2U
#define _e_df       3U
#define _e_ur       4U
#define _e_ul       5U
#define _e_dl       6U
#define _e_dr       7U
#define _e_fr       8U
#define _e_fl       9U
#define _e_bl       10U
#define _e_br       11U

#define _eoshift    4U
#define _coshift    5U

#define _pbits      0xFU
#define _esepbit1   0x4U
#define _esepbit2   0x8U
#define _csepbit    0x4U
#define _eobit      0x10U
#define _cobits     0xF0U
#define _cobits2    0x60U
#define _ctwist_cw  0x20U
#define _ctwist_ccw 0x40U
#define _eflip      0x10U
#define _error      0xFFU

_static cube_t zero = { .corner = {0}, .edge = {0} };
_static cube_t solved = {
	.corner = {0, 1, 2, 3, 4, 5, 6, 7},
	.edge = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}
};

#define zero_fast fastcube( \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
#define solved_fast fastcube( \
    0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)

#define _move_cube_U fastcube( \
    5, 4, 2, 3, 0, 1, 6, 7, 4, 5, 2, 3, 1, 0, 6, 7, 8, 9, 10, 11)
#define _move_cube_U2 fastcube( \
    1, 0, 2, 3, 5, 4, 6, 7, 1, 0, 2, 3, 5, 4, 6, 7, 8, 9, 10, 11)
#define _move_cube_U3 fastcube( \
    4, 5, 2, 3, 1, 0, 6, 7, 5, 4, 2, 3, 0, 1, 6, 7, 8, 9, 10, 11)
#define _move_cube_D fastcube( \
    0, 1, 7, 6, 4, 5, 2, 3, 0, 1, 7, 6, 4, 5, 2, 3, 8, 9, 10, 11)
#define _move_cube_D2 fastcube( \
    0, 1, 3, 2, 4, 5, 7, 6, 0, 1, 3, 2, 4, 5, 7, 6, 8, 9, 10, 11)
#define _move_cube_D3 fastcube( \
    0, 1, 6, 7, 4, 5, 3, 2, 0, 1, 6, 7, 4, 5, 3, 2, 8, 9, 10, 11)
#define _move_cube_R fastcube( \
    70, 1, 2, 69, 4, 32, 35, 7, 0, 1, 2, 3, 8, 5, 6, 11, 7, 9, 10, 4)
#define _move_cube_R2 fastcube( \
    3, 1, 2, 0, 4, 6, 5, 7, 0, 1, 2, 3, 7, 5, 6, 4, 11, 9, 10, 8)
#define _move_cube_R3 fastcube( \
    69, 1, 2, 70, 4, 35, 32, 7, 0, 1, 2, 3, 11, 5, 6, 8, 4, 9, 10, 7)
#define _move_cube_L fastcube( \
    0, 71, 68, 3, 33, 5, 6, 34, 0, 1, 2, 3, 4, 10, 9, 7, 8, 5, 6, 11)
#define _move_cube_L2 fastcube( \
    0, 2, 1, 3, 7, 5, 6, 4, 0, 1, 2, 3, 4, 6, 5, 7, 8, 10, 9, 11)
#define _move_cube_L3 fastcube( \
    0, 68, 71, 3, 34, 5, 6, 33, 0, 1, 2, 3, 4, 9, 10, 7, 8, 6, 5, 11)
#define _move_cube_F fastcube( \
    36, 1, 38, 3, 66, 5, 64, 7, 25, 1, 2, 24, 4, 5, 6, 7, 16, 19, 10, 11)
#define _move_cube_F2 fastcube( \
    2, 1, 0, 3, 6, 5, 4, 7, 3, 1, 2, 0, 4, 5, 6, 7, 9, 8, 10, 11)
#define _move_cube_F3 fastcube( \
    38, 1, 36, 3, 64, 5, 66, 7, 24, 1, 2, 25, 4, 5, 6, 7, 19, 16, 10, 11)
#define _move_cube_B fastcube( \
    0, 37, 2, 39, 4, 67, 6, 65, 0, 27, 26, 3, 4, 5, 6, 7, 8, 9, 17, 18)
#define _move_cube_B2 fastcube( \
    0, 3, 2, 1, 4, 7, 6, 5, 0, 2, 1, 3, 4, 5, 6, 7, 8, 9, 11, 10)
#define _move_cube_B3 fastcube( \
    0, 39, 2, 37, 4, 65, 6, 67, 0, 26, 27, 3, 4, 5, 6, 7, 8, 9, 18, 17)

#define _trans_cube_UFr fastcube( \
    0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)
#define _trans_cube_UFr_inverse fastcube( \
    0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)
#define _trans_cube_ULr fastcube( \
    4, 5, 7, 6, 1, 0, 2, 3, 5, 4, 7, 6, 0, 1, 2, 3, 25, 26, 27, 24)
#define _trans_cube_ULr_inverse fastcube( \
    5, 4, 6, 7, 0, 1, 3, 2, 4, 5, 6, 7, 1, 0, 3, 2, 27, 24, 25, 26)
#define _trans_cube_UBr fastcube( \
    1, 0, 3, 2, 5, 4, 7, 6, 1, 0, 3, 2, 5, 4, 7, 6, 10, 11, 8, 9)
#define _trans_cube_UBr_inverse fastcube( \
    1, 0, 3, 2, 5, 4, 7, 6, 1, 0, 3, 2, 5, 4, 7, 6, 10, 11, 8, 9)
#define _trans_cube_URr fastcube( \
    5, 4, 6, 7, 0, 1, 3, 2, 4, 5, 6, 7, 1, 0, 3, 2, 27, 24, 25, 26)
#define _trans_cube_URr_inverse fastcube( \
    4, 5, 7, 6, 1, 0, 2, 3, 5, 4, 7, 6, 0, 1, 2, 3, 25, 26, 27, 24)
#define _trans_cube_DFr fastcube( \
    2, 3, 0, 1, 6, 7, 4, 5, 3, 2, 1, 0, 6, 7, 4, 5, 9, 8, 11, 10)
#define _trans_cube_DFr_inverse fastcube( \
    2, 3, 0, 1, 6, 7, 4, 5, 3, 2, 1, 0, 6, 7, 4, 5, 9, 8, 11, 10)
#define _trans_cube_DLr fastcube( \
    7, 6, 4, 5, 2, 3, 1, 0, 6, 7, 4, 5, 2, 3, 0, 1, 26, 25, 24, 27)
#define _trans_cube_DLr_inverse fastcube( \
    7, 6, 4, 5, 2, 3, 1, 0, 6, 7, 4, 5, 2, 3, 0, 1, 26, 25, 24, 27)
#define _trans_cube_DBr fastcube( \
    3, 2, 1, 0, 7, 6, 5, 4, 2, 3, 0, 1, 7, 6, 5, 4, 11, 10, 9, 8)
#define _trans_cube_DBr_inverse fastcube( \
    3, 2, 1, 0, 7, 6, 5, 4, 2, 3, 0, 1, 7, 6, 5, 4, 11, 10, 9, 8)
#define _trans_cube_DRr fastcube( \
    6, 7, 5, 4, 3, 2, 0, 1, 7, 6, 5, 4, 3, 2, 1, 0, 24, 27, 26, 25)
#define _trans_cube_DRr_inverse fastcube( \
    6, 7, 5, 4, 3, 2, 0, 1, 7, 6, 5, 4, 3, 2, 1, 0, 24, 27, 26, 25)
#define _trans_cube_RUr fastcube( \
    64, 67, 65, 66, 37, 38, 36, 39, 20, 23, 22, 21, 24, 27, 26, 25, 0, 1, 2, 3)
#define _trans_cube_RUr_inverse fastcube( \
    32, 34, 35, 33, 70, 68, 69, 71, 8, 9, 10, 11, 16, 19, 18, 17, 20, 23, 22, 21)
#define _trans_cube_RFr fastcube( \
    38, 37, 36, 39, 64, 67, 66, 65, 24, 27, 26, 25, 23, 20, 21, 22, 19, 16, 17, 18)
#define _trans_cube_RFr_inverse fastcube( \
    36, 39, 38, 37, 66, 65, 64, 67, 25, 26, 27, 24, 21, 22, 23, 20, 16, 19, 18, 17)
#define _trans_cube_RDr fastcube( \
    67, 64, 66, 65, 38, 37, 39, 36, 23, 20, 21, 22, 27, 24, 25, 26, 2, 3, 0, 1)
#define _trans_cube_RDr_inverse fastcube( \
    33, 35, 34, 32, 71, 69, 68, 70, 10, 11, 8, 9, 17, 18, 19, 16, 21, 22, 23, 20)
#define _trans_cube_RBr fastcube( \
    37, 38, 39, 36, 67, 64, 65, 66, 27, 24, 25, 26, 20, 23, 22, 21, 17, 18, 19, 16)
#define _trans_cube_RBr_inverse fastcube( \
    37, 38, 39, 36, 67, 64, 65, 66, 27, 24, 25, 26, 20, 23, 22, 21, 17, 18, 19, 16)
#define _trans_cube_LUr fastcube( \
    65, 66, 64, 67, 36, 39, 37, 38, 21, 22, 23, 20, 26, 25, 24, 27, 1, 0, 3, 2)
#define _trans_cube_LUr_inverse fastcube( \
    34, 32, 33, 35, 68, 70, 71, 69, 9, 8, 11, 10, 19, 16, 17, 18, 22, 21, 20, 23)
#define _trans_cube_LFr fastcube( \
    36, 39, 38, 37, 66, 65, 64, 67, 25, 26, 27, 24, 21, 22, 23, 20, 16, 19, 18, 17)
#define _trans_cube_LFr_inverse fastcube( \
    38, 37, 36, 39, 64, 67, 66, 65, 24, 27, 26, 25, 23, 20, 21, 22, 19, 16, 17, 18)
#define _trans_cube_LDr fastcube( \
    66, 65, 67, 64, 39, 36, 38, 37, 22, 21, 20, 23, 25, 26, 27, 24, 3, 2, 1, 0)
#define _trans_cube_LDr_inverse fastcube( \
    35, 33, 32, 34, 69, 71, 70, 68, 11, 10, 9, 8, 18, 17, 16, 19, 23, 20, 21, 22)
#define _trans_cube_LBr fastcube( \
    39, 36, 37, 38, 65, 66, 67, 64, 26, 25, 24, 27, 22, 21, 20, 23, 18, 17, 16, 19)
#define _trans_cube_LBr_inverse fastcube( \
    39, 36, 37, 38, 65, 66, 67, 64, 26, 25, 24, 27, 22, 21, 20, 23, 18, 17, 16, 19)
#define _trans_cube_FUr fastcube( \
    68, 70, 69, 71, 32, 34, 33, 35, 16, 19, 18, 17, 9, 8, 11, 10, 5, 4, 7, 6)
#define _trans_cube_FUr_inverse fastcube( \
    68, 70, 69, 71, 32, 34, 33, 35, 16, 19, 18, 17, 9, 8, 11, 10, 5, 4, 7, 6)
#define _trans_cube_FRr fastcube( \
    32, 34, 35, 33, 70, 68, 69, 71, 8, 9, 10, 11, 16, 19, 18, 17, 20, 23, 22, 21)
#define _trans_cube_FRr_inverse fastcube( \
    64, 67, 65, 66, 37, 38, 36, 39, 20, 23, 22, 21, 24, 27, 26, 25, 0, 1, 2, 3)
#define _trans_cube_FDr fastcube( \
    70, 68, 71, 69, 34, 32, 35, 33, 19, 16, 17, 18, 8, 9, 10, 11, 7, 6, 5, 4)
#define _trans_cube_FDr_inverse fastcube( \
    69, 71, 68, 70, 33, 35, 32, 34, 17, 18, 19, 16, 11, 10, 9, 8, 4, 5, 6, 7)
#define _trans_cube_FLr fastcube( \
    34, 32, 33, 35, 68, 70, 71, 69, 9, 8, 11, 10, 19, 16, 17, 18, 22, 21, 20, 23)
#define _trans_cube_FLr_inverse fastcube( \
    65, 66, 64, 67, 36, 39, 37, 38, 21, 22, 23, 20, 26, 25, 24, 27, 1, 0, 3, 2)
#define _trans_cube_BUr fastcube( \
    69, 71, 68, 70, 33, 35, 32, 34, 17, 18, 19, 16, 11, 10, 9, 8, 4, 5, 6, 7)
#define _trans_cube_BUr_inverse fastcube( \
    70, 68, 71, 69, 34, 32, 35, 33, 19, 16, 17, 18, 8, 9, 10, 11, 7, 6, 5, 4)
#define _trans_cube_BRr fastcube( \
    35, 33, 32, 34, 69, 71, 70, 68, 11, 10, 9, 8, 18, 17, 16, 19, 23, 20, 21, 22)
#define _trans_cube_BRr_inverse fastcube( \
    66, 65, 67, 64, 39, 36, 38, 37, 22, 21, 20, 23, 25, 26, 27, 24, 3, 2, 1, 0)
#define _trans_cube_BDr fastcube( \
    71, 69, 70, 68, 35, 33, 34, 32, 18, 17, 16, 19, 10, 11, 8, 9, 6, 7, 4, 5)
#define _trans_cube_BDr_inverse fastcube( \
    71, 69, 70, 68, 35, 33, 34, 32, 18, 17, 16, 19, 10, 11, 8, 9, 6, 7, 4, 5)
#define _trans_cube_BLr fastcube( \
    33, 35, 34, 32, 71, 69, 68, 70, 10, 11, 8, 9, 17, 18, 19, 16, 21, 22, 23, 20)
#define _trans_cube_BLr_inverse fastcube( \
    67, 64, 66, 65, 38, 37, 39, 36, 23, 20, 21, 22, 27, 24, 25, 26, 2, 3, 0, 1)
#define _trans_cube_UFm fastcube( \
    4, 5, 6, 7, 0, 1, 2, 3, 0, 1, 2, 3, 5, 4, 7, 6, 9, 8, 11, 10)
#define _trans_cube_UFm_inverse fastcube( \
    4, 5, 6, 7, 0, 1, 2, 3, 0, 1, 2, 3, 5, 4, 7, 6, 9, 8, 11, 10)
#define _trans_cube_ULm fastcube( \
    0, 1, 3, 2, 5, 4, 6, 7, 4, 5, 6, 7, 0, 1, 2, 3, 24, 27, 26, 25)
#define _trans_cube_ULm_inverse fastcube( \
    0, 1, 3, 2, 5, 4, 6, 7, 4, 5, 6, 7, 0, 1, 2, 3, 24, 27, 26, 25)
#define _trans_cube_UBm fastcube( \
    5, 4, 7, 6, 1, 0, 3, 2, 1, 0, 3, 2, 4, 5, 6, 7, 11, 10, 9, 8)
#define _trans_cube_UBm_inverse fastcube( \
    5, 4, 7, 6, 1, 0, 3, 2, 1, 0, 3, 2, 4, 5, 6, 7, 11, 10, 9, 8)
#define _trans_cube_URm fastcube( \
    1, 0, 2, 3, 4, 5, 7, 6, 5, 4, 7, 6, 1, 0, 3, 2, 26, 25, 24, 27)
#define _trans_cube_URm_inverse fastcube( \
    1, 0, 2, 3, 4, 5, 7, 6, 5, 4, 7, 6, 1, 0, 3, 2, 26, 25, 24, 27)
#define _trans_cube_DFm fastcube( \
    6, 7, 4, 5, 2, 3, 0, 1, 3, 2, 1, 0, 7, 6, 5, 4, 8, 9, 10, 11)
#define _trans_cube_DFm_inverse fastcube( \
    6, 7, 4, 5, 2, 3, 0, 1, 3, 2, 1, 0, 7, 6, 5, 4, 8, 9, 10, 11)
#define _trans_cube_DLm fastcube( \
    3, 2, 0, 1, 6, 7, 5, 4, 7, 6, 5, 4, 2, 3, 0, 1, 27, 24, 25, 26)
#define _trans_cube_DLm_inverse fastcube( \
    2, 3, 1, 0, 7, 6, 4, 5, 6, 7, 4, 5, 3, 2, 1, 0, 25, 26, 27, 24)
#define _trans_cube_DBm fastcube( \
    7, 6, 5, 4, 3, 2, 1, 0, 2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9)
#define _trans_cube_DBm_inverse fastcube( \
    7, 6, 5, 4, 3, 2, 1, 0, 2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9)
#define _trans_cube_DRm fastcube( \
    2, 3, 1, 0, 7, 6, 4, 5, 6, 7, 4, 5, 3, 2, 1, 0, 25, 26, 27, 24)
#define _trans_cube_DRm_inverse fastcube( \
    3, 2, 0, 1, 6, 7, 5, 4, 7, 6, 5, 4, 2, 3, 0, 1, 27, 24, 25, 26)
#define _trans_cube_RUm fastcube( \
    68, 71, 69, 70, 33, 34, 32, 35, 21, 22, 23, 20, 25, 26, 27, 24, 0, 1, 2, 3)
#define _trans_cube_RUm_inverse fastcube( \
    70, 68, 69, 71, 32, 34, 35, 33, 8, 9, 10, 11, 19, 16, 17, 18, 23, 20, 21, 22)
#define _trans_cube_RFm fastcube( \
    34, 33, 32, 35, 68, 71, 70, 69, 25, 26, 27, 24, 22, 21, 20, 23, 19, 16, 17, 18)
#define _trans_cube_RFm_inverse fastcube( \
    66, 65, 64, 67, 36, 39, 38, 37, 25, 26, 27, 24, 22, 21, 20, 23, 19, 16, 17, 18)
#define _trans_cube_RDm fastcube( \
    71, 68, 70, 69, 34, 33, 35, 32, 22, 21, 20, 23, 26, 25, 24, 27, 2, 3, 0, 1)
#define _trans_cube_RDm_inverse fastcube( \
    71, 69, 68, 70, 33, 35, 34, 32, 10, 11, 8, 9, 18, 17, 16, 19, 22, 21, 20, 23)
#define _trans_cube_RBm fastcube( \
    33, 34, 35, 32, 71, 68, 69, 70, 26, 25, 24, 27, 21, 22, 23, 20, 17, 18, 19, 16)
#define _trans_cube_RBm_inverse fastcube( \
    67, 64, 65, 66, 37, 38, 39, 36, 27, 24, 25, 26, 23, 20, 21, 22, 18, 17, 16, 19)
#define _trans_cube_LUm fastcube( \
    69, 70, 68, 71, 32, 35, 33, 34, 20, 23, 22, 21, 27, 24, 25, 26, 1, 0, 3, 2)
#define _trans_cube_LUm_inverse fastcube( \
    68, 70, 71, 69, 34, 32, 33, 35, 9, 8, 11, 10, 16, 19, 18, 17, 21, 22, 23, 20)
#define _trans_cube_LFm fastcube( \
    32, 35, 34, 33, 70, 69, 68, 71, 24, 27, 26, 25, 20, 23, 22, 21, 16, 19, 18, 17)
#define _trans_cube_LFm_inverse fastcube( \
    64, 67, 66, 65, 38, 37, 36, 39, 24, 27, 26, 25, 20, 23, 22, 21, 16, 19, 18, 17)
#define _trans_cube_LDm fastcube( \
    70, 69, 71, 68, 35, 32, 34, 33, 23, 20, 21, 22, 24, 27, 26, 25, 3, 2, 1, 0)
#define _trans_cube_LDm_inverse fastcube( \
    69, 71, 70, 68, 35, 33, 32, 34, 11, 10, 9, 8, 17, 18, 19, 16, 20, 23, 22, 21)
#define _trans_cube_LBm fastcube( \
    35, 32, 33, 34, 69, 70, 71, 68, 27, 24, 25, 26, 23, 20, 21, 22, 18, 17, 16, 19)
#define _trans_cube_LBm_inverse fastcube( \
    65, 66, 67, 64, 39, 36, 37, 38, 26, 25, 24, 27, 21, 22, 23, 20, 17, 18, 19, 16)
#define _trans_cube_FUm fastcube( \
    64, 66, 65, 67, 36, 38, 37, 39, 16, 19, 18, 17, 8, 9, 10, 11, 4, 5, 6, 7)
#define _trans_cube_FUm_inverse fastcube( \
    32, 34, 33, 35, 68, 70, 69, 71, 16, 19, 18, 17, 8, 9, 10, 11, 4, 5, 6, 7)
#define _trans_cube_FRm fastcube( \
    36, 38, 39, 37, 66, 64, 65, 67, 9, 8, 11, 10, 16, 19, 18, 17, 21, 22, 23, 20)
#define _trans_cube_FRm_inverse fastcube( \
    37, 38, 36, 39, 64, 67, 65, 66, 20, 23, 22, 21, 27, 24, 25, 26, 1, 0, 3, 2)
#define _trans_cube_FDm fastcube( \
    66, 64, 67, 65, 38, 36, 39, 37, 19, 16, 17, 18, 9, 8, 11, 10, 6, 7, 4, 5)
#define _trans_cube_FDm_inverse fastcube( \
    33, 35, 32, 34, 69, 71, 68, 70, 17, 18, 19, 16, 10, 11, 8, 9, 5, 4, 7, 6)
#define _trans_cube_FLm fastcube( \
    38, 36, 37, 39, 64, 66, 67, 65, 8, 9, 10, 11, 19, 16, 17, 18, 23, 20, 21, 22)
#define _trans_cube_FLm_inverse fastcube( \
    36, 39, 37, 38, 65, 66, 64, 67, 21, 22, 23, 20, 25, 26, 27, 24, 0, 1, 2, 3)
#define _trans_cube_BUm fastcube( \
    65, 67, 64, 66, 37, 39, 36, 38, 17, 18, 19, 16, 10, 11, 8, 9, 5, 4, 7, 6)
#define _trans_cube_BUm_inverse fastcube( \
    34, 32, 35, 33, 70, 68, 71, 69, 19, 16, 17, 18, 9, 8, 11, 10, 6, 7, 4, 5)
#define _trans_cube_BRm fastcube( \
    39, 37, 36, 38, 65, 67, 66, 64, 10, 11, 8, 9, 18, 17, 16, 19, 22, 21, 20, 23)
#define _trans_cube_BRm_inverse fastcube( \
    39, 36, 38, 37, 66, 65, 67, 64, 22, 21, 20, 23, 26, 25, 24, 27, 2, 3, 0, 1)
#define _trans_cube_BDm fastcube( \
    67, 65, 66, 64, 39, 37, 38, 36, 18, 17, 16, 19, 11, 10, 9, 8, 7, 6, 5, 4)
#define _trans_cube_BDm_inverse fastcube( \
    35, 33, 34, 32, 71, 69, 70, 68, 18, 17, 16, 19, 11, 10, 9, 8, 7, 6, 5, 4)
#define _trans_cube_BLm fastcube( \
    37, 39, 38, 36, 67, 65, 64, 66, 11, 10, 9, 8, 17, 18, 19, 16, 20, 23, 22, 21)
#define _trans_cube_BLm_inverse fastcube( \
    38, 37, 39, 36, 67, 64, 66, 65, 23, 20, 21, 22, 24, 27, 26, 25, 3, 2, 1, 0)

_static const char *cornerstr[] = {
	[_c_ufr] = "UFR",
	[_c_ubl] = "UBL",
	[_c_dfl] = "DFL",
	[_c_dbr] = "DBR",
	[_c_ufl] = "UFL",
	[_c_ubr] = "UBR",
	[_c_dfr] = "DFR",
	[_c_dbl] = "DBL"
};

_static const char *cornerstralt[] = {
	[_c_ufr] = "URF",
	[_c_ubl] = "ULB",
	[_c_dfl] = "DLF",
	[_c_dbr] = "DRB",
	[_c_ufl] = "ULF",
	[_c_ubr] = "URB",
	[_c_dfr] = "DRF",
	[_c_dbl] = "DLB"
};

_static const char *edgestr[] = {
	[_e_uf] = "UF",
	[_e_ub] = "UB",
	[_e_db] = "DB",
	[_e_df] = "DF",
	[_e_ur] = "UR",
	[_e_ul] = "UL",
	[_e_dl] = "DL",
	[_e_dr] = "DR",
	[_e_fr] = "FR",
	[_e_fl] = "FL",
	[_e_bl] = "BL",
	[_e_br] = "BR"
};

_static const char *movestr[] = {
	[_move_U]  = "U",
	[_move_U2] = "U2",
	[_move_U3] = "U'",
	[_move_D]  = "D",
	[_move_D2] = "D2",
	[_move_D3] = "D'",
	[_move_R]  = "R",
	[_move_R2] = "R2",
	[_move_R3] = "R'",
	[_move_L]  = "L",
	[_move_L2] = "L2",
	[_move_L3] = "L'",
	[_move_F]  = "F",
	[_move_F2] = "F2",
	[_move_F3] = "F'",
	[_move_B]  = "B",
	[_move_B2] = "B2",
	[_move_B3] = "B'",
};

_static const char *transstr[] = {
	[_trans_UFr] = "rotation UF",
	[_trans_UFm] = "mirrored UF",
	[_trans_ULr] = "rotation UL",
	[_trans_ULm] = "mirrored UL",
	[_trans_UBr] = "rotation UB",
	[_trans_UBm] = "mirrored UB",
	[_trans_URr] = "rotation UR",
	[_trans_URm] = "mirrored UR",
	[_trans_DFr] = "rotation DF",
	[_trans_DFm] = "mirrored DF",
	[_trans_DLr] = "rotation DL",
	[_trans_DLm] = "mirrored DL",
	[_trans_DBr] = "rotation DB",
	[_trans_DBm] = "mirrored DB",
	[_trans_DRr] = "rotation DR",
	[_trans_DRm] = "mirrored DR",
	[_trans_RUr] = "rotation RU",
	[_trans_RUm] = "mirrored RU",
	[_trans_RFr] = "rotation RF",
	[_trans_RFm] = "mirrored RF",
	[_trans_RDr] = "rotation RD",
	[_trans_RDm] = "mirrored RD",
	[_trans_RBr] = "rotation RB",
	[_trans_RBm] = "mirrored RB",
	[_trans_LUr] = "rotation LU",
	[_trans_LUm] = "mirrored LU",
	[_trans_LFr] = "rotation LF",
	[_trans_LFm] = "mirrored LF",
	[_trans_LDr] = "rotation LD",
	[_trans_LDm] = "mirrored LD",
	[_trans_LBr] = "rotation LB",
	[_trans_LBm] = "mirrored LB",
	[_trans_FUr] = "rotation FU",
	[_trans_FUm] = "mirrored FU",
	[_trans_FRr] = "rotation FR",
	[_trans_FRm] = "mirrored FR",
	[_trans_FDr] = "rotation FD",
	[_trans_FDm] = "mirrored FD",
	[_trans_FLr] = "rotation FL",
	[_trans_FLm] = "mirrored FL",
	[_trans_BUr] = "rotation BU",
	[_trans_BUm] = "mirrored BU",
	[_trans_BRr] = "rotation BR",
	[_trans_BRm] = "mirrored BR",
	[_trans_BDr] = "rotation BD",
	[_trans_BDm] = "mirrored BD",
	[_trans_BLr] = "rotation BL",
	[_trans_BLm] = "mirrored BL",
};

static uint8_t inverse_trans_table[48] = {
	[_trans_UFr] = _trans_UFr,
	[_trans_UFm] = _trans_UFm,
	[_trans_ULr] = _trans_URr,
	[_trans_ULm] = _trans_ULm,
	[_trans_UBr] = _trans_UBr,
	[_trans_UBm] = _trans_UBm,
	[_trans_URr] = _trans_ULr,
	[_trans_URm] = _trans_URm,
	[_trans_DFr] = _trans_DFr,
	[_trans_DFm] = _trans_DFm,
	[_trans_DLr] = _trans_DLr,
	[_trans_DLm] = _trans_DRm,
	[_trans_DBr] = _trans_DBr,
	[_trans_DBm] = _trans_DBm,
	[_trans_DRr] = _trans_DRr,
	[_trans_DRm] = _trans_DLm,
	[_trans_RUr] = _trans_FRr,
	[_trans_RUm] = _trans_FLm,
	[_trans_RFr] = _trans_LFr,
	[_trans_RFm] = _trans_RFm,
	[_trans_RDr] = _trans_BLr,
	[_trans_RDm] = _trans_BRm,
	[_trans_RBr] = _trans_RBr,
	[_trans_RBm] = _trans_LBm,
	[_trans_LUr] = _trans_FLr,
	[_trans_LUm] = _trans_FRm,
	[_trans_LFr] = _trans_RFr,
	[_trans_LFm] = _trans_LFm,
	[_trans_LDr] = _trans_BRr,
	[_trans_LDm] = _trans_BLm,
	[_trans_LBr] = _trans_LBr,
	[_trans_LBm] = _trans_RBm,
	[_trans_FUr] = _trans_FUr,
	[_trans_FUm] = _trans_FUm,
	[_trans_FRr] = _trans_RUr,
	[_trans_FRm] = _trans_LUm,
	[_trans_FDr] = _trans_BUr,
	[_trans_FDm] = _trans_BUm,
	[_trans_FLr] = _trans_LUr,
	[_trans_FLm] = _trans_RUm,
	[_trans_BUr] = _trans_FDr,
	[_trans_BUm] = _trans_FDm,
	[_trans_BRr] = _trans_LDr,
	[_trans_BRm] = _trans_RDm,
	[_trans_BDr] = _trans_BDr,
	[_trans_BDm] = _trans_BDm,
	[_trans_BLr] = _trans_RDr,
	[_trans_BLm] = _trans_LDm,
};

/******************************************************************************
Section: AVX2 fast methods

This section contains performance-critical methods that rely on AVX2
intructions such as routines for moving or transforming the cube.
******************************************************************************/

#if defined(CUBE_AVX2)

#include <immintrin.h>

typedef __m256i cube_fast_t;

#define _co2_avx2 _mm256_set_epi64x(0, 0, 0, 0x6060606060606060)
#define _cocw_avx2 _mm256_set_epi64x(0, 0, 0, 0x2020202020202020)
#define _cp_avx2 _mm256_set_epi64x(0, 0, 0, 0x0707070707070707)
#define _ep_avx2 _mm256_set_epi64x(0x0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0, 0)
#define _eo_avx2 _mm256_set_epi64x(0x10101010, 0x1010101010101010, 0, 0)

_static_inline cube_fast_t fastcube(
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t
);
_static cube_fast_t cubetofast(cube_t);
_static cube_t fasttocube(cube_fast_t);
_static_inline bool equal_fast(cube_fast_t, cube_fast_t);
_static_inline bool issolved_fast(cube_fast_t);
_static_inline cube_fast_t invertco_fast(cube_fast_t);
_static_inline cube_fast_t compose_fast(cube_fast_t, cube_fast_t);

_static_inline int64_t coord_fast_co(cube_fast_t);
_static_inline int64_t coord_fast_csep(cube_fast_t);
_static_inline int64_t coord_fast_cocsep(cube_fast_t);
_static_inline int64_t coord_fast_eo(cube_fast_t);
_static_inline int64_t coord_fast_esep(cube_fast_t);

_static_inline cube_fast_t
fastcube(
	uint8_t c_ufr,
	uint8_t c_ubl,
	uint8_t c_dfl,
	uint8_t c_dbr,
	uint8_t c_ufl,
	uint8_t c_ubr,
	uint8_t c_dfr,
	uint8_t c_dbl,

	uint8_t e_uf,
	uint8_t e_ub,
	uint8_t e_db,
	uint8_t e_df,
	uint8_t e_ur,
	uint8_t e_ul,
	uint8_t e_dl,
	uint8_t e_dr,
	uint8_t e_fr,
	uint8_t e_fl,
	uint8_t e_bl,
	uint8_t e_br
)
{
	return _mm256_set_epi8(
		0, 0, 0, 0, e_br, e_bl, e_fl, e_fr,
		e_dr, e_dl, e_ul, e_ur, e_df, e_db, e_ub, e_uf,
		0, 0, 0, 0, 0, 0, 0, 0,
		c_dbl, c_dfr, c_ubr, c_ufl, c_dbr, c_dfl, c_ubl, c_ufr
	);
}

_static cube_fast_t
cubetofast(cube_t a)
{
	uint8_t aux[32];

	memset(aux, 0, 32);
	memcpy(aux, &a.corner, 8);
	memcpy(aux + 16, &a.edge, 12);

	return _mm256_loadu_si256((__m256i_u *)&aux);
}

_static cube_t
fasttocube(cube_fast_t c)
{
	cube_t a;
	uint8_t aux[32];

	_mm256_storeu_si256((__m256i_u *)aux, c);
	memcpy(&a.corner, aux, 8);
	memcpy(&a.edge, aux + 16, 12);

	return a;
}

_static_inline bool
equal_fast(cube_fast_t c1, cube_fast_t c2)
{
	int32_t mask;
	__m256i cmp;

	cmp = _mm256_cmpeq_epi8(c1, c2);
	mask = _mm256_movemask_epi8(cmp);

	return mask == ~0;
}

_static_inline bool
issolved_fast(cube_fast_t cube)
{
	return equal_fast(cube, solved_fast);
}

_static_inline cube_fast_t
invertco_fast(cube_fast_t c)
{
        cube_fast_t co, shleft, shright, summed, newco, cleanco, ret;

        co = _mm256_and_si256(c, _co2_avx2);
        shleft = _mm256_slli_epi32(co, 1);
        shright = _mm256_srli_epi32(co, 1);
        summed = _mm256_or_si256(shleft, shright);
        newco = _mm256_and_si256(summed, _co2_avx2);
        cleanco = _mm256_xor_si256(c, co);
        ret = _mm256_or_si256(cleanco, newco);

        return ret;
}

_static_inline cube_fast_t
compose_fast(cube_fast_t c1, cube_fast_t c2)
{
	cube_fast_t s, b, eo2, co1, co2, aux, auy1, auy2, auz1, auz2;

	/* Permute and clean unused bits */
	s = _mm256_shuffle_epi8(c1, c2);
	b = _mm256_set_epi8(
		~0, ~0, ~0, ~0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	s = _mm256_andnot_si256(b, s);

	/* Change EO */
	eo2 = _mm256_and_si256(c2, _eo_avx2);
	s = _mm256_xor_si256(s, eo2);

	/* Change CO */
	co1 = _mm256_and_si256(s, _co2_avx2);
	co2 = _mm256_and_si256(c2, _co2_avx2);
	aux = _mm256_add_epi8(co1, co2);
	auy1 = _mm256_add_epi8(aux, _cocw_avx2);
	auy2 = _mm256_srli_epi32(auy1, 2);
	auz1 = _mm256_add_epi8(aux, auy2);
	auz2 = _mm256_and_si256(auz1, _co2_avx2);

	/* Put together */
	s = _mm256_andnot_si256(_co2_avx2, s);
	s = _mm256_or_si256(s, auz2);

	return s;
}

_static_inline int64_t
coord_fast_co(cube_fast_t c)
{
	cube_fast_t co;
	int64_t mem[4], ret, i, p;

	co = _mm256_and_si256(c, _co2_avx2);
	_mm256_storeu_si256((__m256i *)mem, co);

	mem[0] >>= 5L;
	for (i = 0, ret = 0, p = 1; i < 7; i++, mem[0] >>= 8L, p *= 3)
		ret += (mem[0] & 3L) * p;

	return ret;
}

_static_inline int64_t
coord_fast_csep(cube_fast_t c)
{
	cube_fast_t cp, shifted;
	int64_t mask;

	cp = _mm256_and_si256(c, _cp_avx2);
	shifted = _mm256_slli_epi32(cp, 5);
	mask = _mm256_movemask_epi8(shifted);

	return mask & 0x7F;
}

_static_inline int64_t
coord_fast_cocsep(cube_fast_t c)
{
	return (coord_fast_co(c) << 7) + coord_fast_csep(c);
}

_static_inline int64_t
coord_fast_eo(cube_fast_t c)
{
	cube_fast_t eo, shifted;
	int64_t mask;

	eo = _mm256_and_si256(c, _eo_avx2);
	shifted = _mm256_slli_epi32(eo, 3);
	mask = _mm256_movemask_epi8(shifted);

	return mask >> 17;
}

_static_inline int64_t
coord_fast_esep(cube_fast_t c)
{
	cube_fast_t ep;
	int64_t e, mem[4], i, j, k, l, ret1, ret2, bit1, bit2, is1;

	ep = _mm256_and_si256(c, _ep_avx2);
	_mm256_storeu_si256((__m256i *)mem, ep);

	mem[3] <<= 8L;
	ret1 = ret2 = 0;
	k = l = 4;
	for (i = 0, j = 0; i < 12; i++, mem[i/8 + 2] >>= 8L) {
		e = mem[i/8 + 2];

		bit1 = (e & _esepbit1) >> 2L;
		bit2 = (e & _esepbit2) >> 3L;
		is1 = (1 - bit2) * bit1;

		ret1 += bit2 * binomial[11-i][k];
		k -= bit2;

		ret2 += is1 * binomial[7-j][l];
		l -= is1;
		j += (1-bit2);
	}

	return ret1 * 70 + ret2;
}

/******************************************************************************
Section: ARM NEON fast methods

This section contains performance-critical methods that rely on ARM NEON
intructions such as routines for moving or transforming the cube.
******************************************************************************/

#elif defined(CUBE_NEON)

typedef struct {
	uint8x16_t corner;
	uint8x16_t edge;
} cube_fast_t;

/* TODO! */

/******************************************************************************
Section: portable fast methods

This section contains performance-critical methods that do not use
advanced CPU instructions. They are used as an alternative to the ones
in the previous section(s) for unsupported architectures.
******************************************************************************/

#else

typedef cube_t cube_fast_t;

_static_inline cube_fast_t fastcube(
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t
);
_static cube_fast_t cubetofast(cube_t);
_static cube_t fasttocube(cube_fast_t);
_static_inline bool equal_fast(cube_fast_t, cube_fast_t);
_static_inline bool issolved_fast(cube_fast_t);
_static_inline cube_fast_t invertco_fast(cube_fast_t);
_static_inline cube_fast_t compose_fast(cube_fast_t, cube_fast_t);

_static_inline int64_t coord_fast_co(cube_fast_t);
_static_inline int64_t coord_fast_csep(cube_fast_t);
_static_inline int64_t coord_fast_cocsep(cube_fast_t);
_static_inline int64_t coord_fast_eo(cube_fast_t);
_static_inline int64_t coord_fast_esep(cube_fast_t);

_static_inline cube_fast_t
fastcube(
	uint8_t c_ufr,
	uint8_t c_ubl,
	uint8_t c_dfl,
	uint8_t c_dbr,
	uint8_t c_ufl,
	uint8_t c_ubr,
	uint8_t c_dfr,
	uint8_t c_dbl,

	uint8_t e_uf,
	uint8_t e_ub,
	uint8_t e_db,
	uint8_t e_df,
	uint8_t e_ur,
	uint8_t e_ul,
	uint8_t e_dl,
	uint8_t e_dr,
	uint8_t e_fr,
	uint8_t e_fl,
	uint8_t e_bl,
	uint8_t e_br
)
{
	cube_fast_t cube = {
		.corner = {
			c_ufr, c_ubl, c_dfl, c_dbr, c_ufl, c_ubr, c_dfr, c_dbl
		},
		.edge = {
			e_uf, e_ub, e_db, e_df, e_ur, e_ul,
			e_dl, e_dr, e_fr, e_fl, e_bl, e_br
		}
	};

	return cube;
}

_static cube_fast_t
cubetofast(cube_t cube)
{
	cube_fast_t fast;
	memcpy(&fast, &cube, sizeof(cube_fast_t));
	return fast;
}

_static cube_t
fasttocube(cube_fast_t fast)
{
	cube_t cube;
	memcpy(&cube, &fast, sizeof(cube_fast_t));
	return cube;
}

_static_inline bool
equal_fast(cube_fast_t c1, cube_fast_t c2)
{
	uint8_t i;
	bool ret;

	ret = true;
	for (i = 0; i < 8; i++)
		ret = ret && c1.corner[i] == c2.corner[i];
	for (i = 0; i < 12; i++)
		ret = ret && c1.edge[i] == c2.edge[i];

	return ret;
}

_static_inline bool
issolved_fast(cube_fast_t cube)
{
	return equal_fast(cube, solved_fast);
}

_static_inline cube_fast_t
invertco_fast(cube_fast_t c)
{
	uint8_t i, piece, orien;
	cube_fast_t ret;

	ret = c;
	for (i = 0; i < 8; i++) {
		piece = c.corner[i];
		orien = ((piece << 1) | (piece >> 1)) & _cobits2;
		ret.corner[i] = (piece & _pbits) | orien;
	}

	return ret;
}

_static_inline cube_fast_t
compose_fast(cube_fast_t c1, cube_fast_t c2)
{
	cube_fast_t ret;
	uint8_t i, piece1, piece2, p, orien, aux, auy;

	ret = zero_fast;

	for (i = 0; i < 12; i++) {
		piece2 = c2.edge[i];
		p = piece2 & _pbits;
		piece1 = c1.edge[p];
		orien = (piece2 ^ piece1) & _eobit;
		ret.edge[i] = (piece1 & _pbits) | orien;
	}

	for (i = 0; i < 8; i++) {
		piece2 = c2.corner[i];
		p = piece2 & _pbits;
		piece1 = c1.corner[p];
		aux = (piece2 & _cobits) + (piece1 & _cobits);
		auy = (aux + _ctwist_cw) >> 2U;
		orien = (aux + auy) & _cobits2;
		ret.corner[i] = (piece1 & _pbits) | orien;
	}

	return ret;
}

_static_inline int64_t
coord_fast_co(cube_fast_t c)
{
	int i, p;
	int64_t ret;

	for (ret = 0, i = 0, p = 1; i < 7; i++, p *= 3)
		ret += p * (c.corner[i] >> _coshift);

	return ret;
}

/*
For corner separation, we consider the axis (a.k.a. tetrad) each
corner belongs to as 0 or 1 and we translate this sequence into binary.
Ignoring the last bit, we have a value up to 2^7, but not all values are
possible. Encoding this as a number from 0 to C(8,4) would save about 40%
of space, but we are not going to use this coordinate in large tables.
*/
_static_inline int64_t
coord_fast_csep(cube_fast_t c)
{
	int i, p;
	int64_t ret;

	for (ret = 0, i = 0, p = 1; i < 7; i++, p *= 2)
		ret += p * ((c.corner[i] & _csepbit) >> 2U);

	return ret;
}

_static_inline int64_t
coord_fast_cocsep(cube_fast_t c)
{
	return (coord_fast_co(c) << 7) + coord_fast_csep(c);
}

_static_inline int64_t
coord_fast_eo(cube_fast_t c)
{
	int i, p;
	int64_t ret;

	for (ret = 0, i = 1, p = 1; i < 12; i++, p *= 2)
		ret += p * (c.edge[i] >> _eoshift);

	return ret;
}

/*
We encode the edge separation as a number from 0 to C(12,4)*C(8,4).
It can be seen as the composition of two "subset index" coordinates.
*/
_static_inline int64_t
coord_fast_esep(cube_fast_t c)
{
	int64_t i, j, k, l, ret1, ret2, bit1, bit2, is1;

	for (i = 0, j = 0, k = 4, l = 4, ret1 = 0, ret2 = 0; i < 12; i++) {
		/* Simple version:
		if (c.edge[i] & _esepbit2) {
			ret1 += binomial[11-i][k--];
		} else {
			if (c.edge[i] & _esepbit1)
				ret2 += binomial[7-j][l--];
			j++;
		}
		*/

		bit1 = (c.edge[i] & _esepbit1) >> 2U;
		bit2 = (c.edge[i] & _esepbit2) >> 3U;
		is1 = (1 - bit2) * bit1;

		ret1 += bit2 * binomial[11-i][k];
		k -= bit2;

		ret2 += is1 * binomial[7-j][l];
		l -= is1;
		j += (1-bit2);
	}

	return ret1 * 70 + ret2;
}

#endif

/******************************************************************************
Section: generic methods

This section contains generic functionality, including the public functions.
Some of these routines depend on the efficient functions implemented in the
previous sections, while some other operate directly on the cube.
******************************************************************************/

#define _move(M, c) compose_fast(c, _move_cube_ ## M)
#define _premove(M, c) compose_fast(_move_cube_ ## M, c)
#define _trans_rotation(T, c) \
	compose_fast(compose_fast(_trans_cube_ ## T, c), \
	_trans_cube_ ## T ## _inverse)
#define _trans_mirrored(T, c) \
	invertco_fast(compose_fast(compose_fast(_trans_cube_ ## T, c), \
	_trans_cube_ ## T ## _inverse))

_static int permsign(uint8_t *, int);
_static uint8_t readco(const char *);
_static uint8_t readcp(const char *);
_static uint8_t readeo(const char *);
_static uint8_t readep(const char *);
_static cube_t readcube_H48(const char *);
_static uint8_t readpiece_LST(const char **);
_static cube_t readcube_LST(const char *);
_static int writepiece_LST(uint8_t, char *);
_static void writecube_H48(cube_t, char *);
_static void writecube_LST(cube_t, char *);
_static uint8_t readmove(char);
_static uint8_t readmodifier(char);
_static uint8_t readtrans(const char *);
_static int writemoves(uint8_t *, int, char *);
_static void writetrans(uint8_t, char *);
_static cube_fast_t move(cube_fast_t, uint8_t);
_static cube_fast_t transform(cube_fast_t, uint8_t);

_static_inline int64_t coord_h48(cube_fast_t, uint32_t *, uint8_t);

cube_t
solvedcube(void)
{
	return solved;
}

bool
isconsistent(cube_t cube)
{
	uint8_t i, p, e, piece;
	bool found[12];

	for (i = 0; i < 12; i++)
		found[i] = false;
	for (i = 0; i < 12; i++) {
		piece = cube.edge[i];
		p = piece & _pbits;
		e = piece & _eobit;
		if (p >= 12)
			goto inconsistent_ep;
		if (e != 0 && e != _eobit)
			goto inconsistent_eo;
		found[p] = true;
	}
	for (i = 0; i < 12; i++)
		if (!found[i])
			goto inconsistent_ep;

	for (i = 0; i < 8; i++)
		found[i] = false;
	for (i = 0; i < 8; i++) {
		piece = cube.corner[i];
		p = piece & _pbits;
		e = piece & _cobits;
		if (p >= 8)
			goto inconsistent_cp;
		if (e != 0 && e != _ctwist_cw && e != _ctwist_ccw)
			goto inconsistent_co;
		found[p] = true;
	}
	for (i = 0; i < 8; i++)
		if (!found[i])
			goto inconsistent_co;

	return true;

inconsistent_ep:
	DBG_LOG("Inconsistent EP\n");
	return false;
inconsistent_cp:
	DBG_LOG("Inconsistent CP\n");
	return false;
inconsistent_eo:
	DBG_LOG("Inconsistent EO\n");
	return false;
inconsistent_co:
	DBG_LOG("Inconsistent CO\n");
	return false;
}

bool
issolvable(cube_t cube)
{
	uint8_t i, eo, co, piece, edges[12], corners[8];

	DBG_ASSERT(isconsistent(cube), false,
	    "issolvable: cube is inconsistent\n");

	for (i = 0; i < 12; i++)
		edges[i] = cube.edge[i] & _pbits;
	for (i = 0; i < 8; i++)
		corners[i] = cube.corner[i] & _pbits;

	if (permsign(edges, 12) != permsign(corners, 8))
		goto issolvable_parity;

	eo = 0;
	for (i = 0; i < 12; i++) {
		piece = cube.edge[i];
		eo += (piece & _eobit) >> _eoshift;
	}
	if (eo % 2 != 0)
		goto issolvable_eo;

	co = 0;
	for (i = 0; i < 8; i++) {
		piece = cube.corner[i];
		co += (piece & _cobits) >> _coshift;
	}
	if (co % 3 != 0)
		goto issolvable_co;

	return true;

issolvable_parity:
	DBG_LOG("EP and CP parities are different\n");
	return false;
issolvable_eo:
	DBG_LOG("Odd number of flipped edges\n");
	return false;
issolvable_co:
	DBG_LOG("Sum of corner orientation is not multiple of 3\n");
	return false;
}

bool
issolved(cube_t cube)
{
	return equal(cube, solved);
}

bool
equal(cube_t c1, cube_t c2)
{
	int i;
	bool ret;

	ret = true;
	for (i = 0; i < 8; i++)
		ret = ret && c1.corner[i] == c2.corner[i];
	for (i = 0; i < 12; i++)
		ret = ret && c1.edge[i] == c2.edge[i];

	return ret;
}

bool
iserror(cube_t cube)
{
	return equal(cube, zero);
}

cube_t
compose(cube_t c1, cube_t c2)
{
	DBG_ASSERT(isconsistent(c1) && isconsistent(c2),
	    zero, "compose error: inconsistent cube\n")

	return fasttocube(compose_fast(cubetofast(c1), cubetofast(c2)));
}

cube_t
inverse(cube_t cube)
{
	cube_t ret;
	uint8_t i, piece, orien;

	DBG_ASSERT(isconsistent(cube), zero,
	    "inverse error: inconsistent cube\n");

	ret = zero;

	for (i = 0; i < 12; i++) {
		piece = cube.edge[i];
		orien = piece & _eobit;
		ret.edge[piece & _pbits] = i | orien;
	}

	for (i = 0; i < 8; i++) {
		piece = cube.corner[i];
		orien = ((piece << 1) | (piece >> 1)) & _cobits2;
		ret.corner[piece & _pbits] = i | orien;
	}

	return ret;
}

cube_t
applymoves(cube_t cube, const char *buf)
{
	cube_fast_t fast;
	uint8_t r, m;
	const char *b;

	DBG_ASSERT(isconsistent(cube), zero,
	    "move error: inconsistent cube\n");

	fast = cubetofast(cube);

	for (b = buf; *b != '\0'; b++) {
		while (*b == ' ' || *b == '\t' || *b == '\n')
			b++;
		if (*b == '\0')
			goto applymoves_finish;
		if ((r = readmove(*b)) == _error)
			goto applymoves_error;
		if ((m = readmodifier(*(b+1))) != 0)
			b++;
		fast = move(fast, r + m);
	}

applymoves_finish:
	return fasttocube(fast);

applymoves_error:
	DBG_LOG("applymoves error\n");
	return zero;
}

cube_t
applytrans(cube_t cube, const char *buf)
{
	cube_fast_t fast;
	uint8_t t;

	DBG_ASSERT(isconsistent(cube), zero,
	    "transformation error: inconsistent cube\n");

	t = readtrans(buf);
	fast = cubetofast(cube);
	fast = transform(fast, t);

	return fasttocube(fast);
}

cube_t
readcube(const char *format, const char *buf)
{
	cube_t cube;

	if (!strcmp(format, "H48")) {
		cube = readcube_H48(buf);
	} else if (!strcmp(format, "LST")) {
		cube = readcube_LST(buf);
	} else {
		DBG_LOG("Cannot read cube in the given format\n");
		cube = zero;
	}

	return cube;
}

void
writecube(const char *format, cube_t cube, char *buf)
{
	char *errormsg;
	size_t len;

	if (!isconsistent(cube)) {
		errormsg = "ERROR: cannot write inconsistent cube";
		goto writecube_error;
	}

	if (!strcmp(format, "H48")) {
		writecube_H48(cube, buf);
	} else if (!strcmp(format, "LST")) {
		writecube_LST(cube, buf);
	} else {
		errormsg = "ERROR: cannot write cube in the given format";
		goto writecube_error;
	}

	return;

writecube_error:
	DBG_LOG("writecube error, see stdout for details\n");
	len = strlen(errormsg);
	memcpy(buf, errormsg, len);
	buf[len] = '\n';
	buf[len+1] = '\0';
}

_static int
permsign(uint8_t *a, int n)
{
	int i, j;
	uint8_t ret = 0;

	for (i = 0; i < n; i++)
		for (j = i+1; j < n; j++)
			ret += a[i] > a[j] ? 1 : 0;

	return ret % 2;
}

_static uint8_t
readco(const char *str)
{
	if (*str == '0')
		return 0;
	if (*str == '1')
		return _ctwist_cw;
	if (*str == '2')
		return _ctwist_ccw;

	DBG_LOG("Error reading CO\n");
	return _error;
}

_static uint8_t
readcp(const char *str)
{
	uint8_t c;

	for (c = 0; c < 8; c++)
		if (!strncmp(str, cornerstr[c], 3) ||
		    !strncmp(str, cornerstralt[c], 3))
			return c;

	DBG_LOG("Error reading CP\n");
	return _error;
}

_static uint8_t
readeo(const char *str)
{
	if (*str == '0')
		return 0;
	if (*str == '1')
		return _eflip;

	DBG_LOG("Error reading EO\n");
	return _error;
}

_static uint8_t
readep(const char *str)
{
	uint8_t e;

	for (e = 0; e < 12; e++)
		if (!strncmp(str, edgestr[e], 2))
			return e;

	DBG_LOG("Error reading EP\n");
	return _error;
}

_static cube_t
readcube_H48(const char *buf)
{
	int i;
	uint8_t piece, orient;
	cube_t ret = {0};
	const char *b;
	
	b = buf;

	for (i = 0; i < 12; i++) {
		while (*b == ' ' || *b == '\t' || *b == '\n')
			b++;
		if ((piece = readep(b)) == _error)
			return zero;
		b += 2;
		if ((orient = readeo(b)) == _error)
			return zero;
		b++;
		ret.edge[i] = piece | orient;
	}
	for (i = 0; i < 8; i++) {
		while (*b == ' ' || *b == '\t' || *b == '\n')
			b++;
		if ((piece = readcp(b)) == _error)
			return zero;
		b += 3;
		if ((orient = readco(b)) == _error)
			return zero;
		b++;
		ret.corner[i] = piece | orient;
	}

	return ret;
}

_static uint8_t
readpiece_LST(const char **b)
{
	uint8_t ret;
	bool read;

	while (**b == ',' || **b == ' ' || **b == '\t' || **b == '\n')
		(*b)++;

	for (ret = 0, read = false; **b >= '0' && **b <= '9'; (*b)++) {
		read = true;
		ret = ret * 10 + (**b) - '0';
	}

	return read ? ret : _error;
}

_static cube_t
readcube_LST(const char *buf)
{
	int i;
	cube_t ret = {0};

	for (i = 0; i < 8; i++)
		ret.corner[i] = readpiece_LST(&buf);

	for (i = 0; i < 12; i++)
		ret.edge[i] = readpiece_LST(&buf);

	return ret;
}

_static int
writepiece_LST(uint8_t piece, char *buf)
{
	char digits[3];
	int i, len = 0;

	while (piece != 0) {
		digits[len++] = (piece % 10) + '0';
		piece /= 10;
	}

	if (len == 0)
		digits[len++] = '0';

	for (i = 0; i < len; i++)
		buf[i] = digits[len-i-1];

	buf[len] = ',';
	buf[len+1] = ' ';

	return len+2;
}

_static void
writecube_H48(cube_t cube, char *buf)
{
	uint8_t piece, perm, orient;
	int i;

	for (i = 0; i < 12; i++) {
		piece = cube.edge[i];
		perm = piece & _pbits;
		orient = (piece & _eobit) >> _eoshift;
		buf[4*i    ] = edgestr[perm][0];
		buf[4*i + 1] = edgestr[perm][1];
		buf[4*i + 2] = orient + '0';
		buf[4*i + 3] = ' ';
	}
	for (i = 0; i < 8; i++) {
		piece = cube.corner[i];
		perm = piece & _pbits;
		orient = (piece & _cobits) >> _coshift;
		buf[48 + 5*i    ] = cornerstr[perm][0];
		buf[48 + 5*i + 1] = cornerstr[perm][1];
		buf[48 + 5*i + 2] = cornerstr[perm][2];
		buf[48 + 5*i + 3] = orient + '0';
		buf[48 + 5*i + 4] = ' ';
	}

	buf[48+39] = '\0';
}

_static void
writecube_LST(cube_t cube, char *buf)
{
	int i, ptr;
	uint8_t piece;

	ptr = 0;

	for (i = 0; i < 8; i++) {
		piece = cube.corner[i];
		ptr += writepiece_LST(piece, buf + ptr);
	}

	for (i = 0; i < 12; i++) {
		piece = cube.edge[i];
		ptr += writepiece_LST(piece, buf + ptr);
	}

	*(buf+ptr-2) = 0;
}

_static uint8_t
readmove(char c)
{
	switch (c) {
	case 'U':
		return _move_U;
	case 'D':
		return _move_D;
	case 'R':
		return _move_R;
	case 'L':
		return _move_L;
	case 'F':
		return _move_F;
	case 'B':
		return _move_B;
	default:
		return _error;
	}
}

_static uint8_t
readmodifier(char c)
{
	switch (c) {
	case '1': /* Fallthrough */
	case '2': /* Fallthrough */
	case '3':
		return c - '0' - 1;
	case '\'':
		return 2;
	default:
		return 0;
	}
}

_static uint8_t
readtrans(const char *buf)
{
	uint8_t t;

	for (t = 0; t < 48; t++)
		if (!strncmp(buf, transstr[t], 11))
			return t;

	DBG_LOG("readtrans error\n");
	return _error;
}

_static int
writemoves(uint8_t *m, int n, char *buf)
{
	int i;
	size_t len;
	const char *s;
	char *b;

	for (i = 0, b = buf; i < n; i++, b++) {
		s = movestr[m[i]];
		len = strlen(s);
		memcpy(b, s, len);
		b += len;	
		*b = ' ';
	}

	if (b != buf)
		b--; /* Remove last space */
	*b = '\0';

	return b - buf;
}

_static void
writetrans(uint8_t t, char *buf)
{
	if (t >= 48)
		memcpy(buf, "error trans", 11);
	else
		memcpy(buf, transstr[t], 11);
	buf[11] = '\0';
}

_static cube_fast_t
move(cube_fast_t c, uint8_t m)
{
	switch (m) {
	case _move_U:
		return _move(U, c);
	case _move_U2:
		return _move(U2, c);
	case _move_U3:
		return _move(U3, c);
	case _move_D:
		return _move(D, c);
	case _move_D2:
		return _move(D2, c);
	case _move_D3:
		return _move(D3, c);
	case _move_R:
		return _move(R, c);
	case _move_R2:
		return _move(R2, c);
	case _move_R3:
		return _move(R3, c);
	case _move_L:
		return _move(L, c);
	case _move_L2:
		return _move(L2, c);
	case _move_L3:
		return _move(L3, c);
	case _move_F:
		return _move(F, c);
	case _move_F2:
		return _move(F2, c);
	case _move_F3:
		return _move(F3, c);
	case _move_B:
		return _move(B, c);
	case _move_B2:
		return _move(B2, c);
	case _move_B3:
		return _move(B3, c);
	default:
		DBG_LOG("move error, unknown move\n");
		return zero_fast;
	}
}

_static cube_fast_t
transform(cube_fast_t c, uint8_t t)
{
	switch (t) {
	case _trans_UFr:
		return _trans_rotation(UFr, c);
	case _trans_ULr:
		return _trans_rotation(ULr, c);
	case _trans_UBr:
		return _trans_rotation(UBr, c);
	case _trans_URr:
		return _trans_rotation(URr, c);
	case _trans_DFr:
		return _trans_rotation(DFr, c);
	case _trans_DLr:
		return _trans_rotation(DLr, c);
	case _trans_DBr:
		return _trans_rotation(DBr, c);
	case _trans_DRr:
		return _trans_rotation(DRr, c);
	case _trans_RUr:
		return _trans_rotation(RUr, c);
	case _trans_RFr:
		return _trans_rotation(RFr, c);
	case _trans_RDr:
		return _trans_rotation(RDr, c);
	case _trans_RBr:
		return _trans_rotation(RBr, c);
	case _trans_LUr:
		return _trans_rotation(LUr, c);
	case _trans_LFr:
		return _trans_rotation(LFr, c);
	case _trans_LDr:
		return _trans_rotation(LDr, c);
	case _trans_LBr:
		return _trans_rotation(LBr, c);
	case _trans_FUr:
		return _trans_rotation(FUr, c);
	case _trans_FRr:
		return _trans_rotation(FRr, c);
	case _trans_FDr:
		return _trans_rotation(FDr, c);
	case _trans_FLr:
		return _trans_rotation(FLr, c);
	case _trans_BUr:
		return _trans_rotation(BUr, c);
	case _trans_BRr:
		return _trans_rotation(BRr, c);
	case _trans_BDr:
		return _trans_rotation(BDr, c);
	case _trans_BLr:
		return _trans_rotation(BLr, c);
	case _trans_UFm:
		return _trans_mirrored(UFm, c);
	case _trans_ULm:
		return _trans_mirrored(ULm, c);
	case _trans_UBm:
		return _trans_mirrored(UBm, c);
	case _trans_URm:
		return _trans_mirrored(URm, c);
	case _trans_DFm:
		return _trans_mirrored(DFm, c);
	case _trans_DLm:
		return _trans_mirrored(DLm, c);
	case _trans_DBm:
		return _trans_mirrored(DBm, c);
	case _trans_DRm:
		return _trans_mirrored(DRm, c);
	case _trans_RUm:
		return _trans_mirrored(RUm, c);
	case _trans_RFm:
		return _trans_mirrored(RFm, c);
	case _trans_RDm:
		return _trans_mirrored(RDm, c);
	case _trans_RBm:
		return _trans_mirrored(RBm, c);
	case _trans_LUm:
		return _trans_mirrored(LUm, c);
	case _trans_LFm:
		return _trans_mirrored(LFm, c);
	case _trans_LDm:
		return _trans_mirrored(LDm, c);
	case _trans_LBm:
		return _trans_mirrored(LBm, c);
	case _trans_FUm:
		return _trans_mirrored(FUm, c);
	case _trans_FRm:
		return _trans_mirrored(FRm, c);
	case _trans_FDm:
		return _trans_mirrored(FDm, c);
	case _trans_FLm:
		return _trans_mirrored(FLm, c);
	case _trans_BUm:
		return _trans_mirrored(BUm, c);
	case _trans_BRm:
		return _trans_mirrored(BRm, c);
	case _trans_BDm:
		return _trans_mirrored(BDm, c);
	case _trans_BLm:
		return _trans_mirrored(BLm, c);
	default:
		DBG_LOG("transform error, unknown transformation\n");
		return zero_fast;
	}
}

/* h is the number of eo bits used */
_static_inline int64_t
coord_h48(cube_fast_t c, uint32_t *cocsepdata, uint8_t h)
{
	cube_fast_t d;
	int64_t cocsep, coclass, esep, eo, esize, ret;
	uint32_t data;
	uint8_t ttrep;

	DBG_ASSERT(h <= 11, -1, "coord_h48: h must be between 0 and 11\n");

	cocsep = coord_fast_cocsep(c);
	data = cocsepdata[cocsep];
	coclass = (data & (0xFFFFU << 16U)) >> 16U;
	ttrep = (data & (0xFFU << 8U)) >> 8U;

	d = transform(c, ttrep); /* TODO: transform only edges */
	esep = coord_fast_esep(d);
	eo = coord_fast_eo(d);

	esize = (_12c4 * _8c4) << h;
	ret = (coclass * esize) + (esep << h) + (eo >> (11-h));

	return ret;
}

/******************************************************************************
Section: moves, move sequences and transformations

This section contains methods to work with moves and arrays of moves. They
do not rely on the cube structure.
******************************************************************************/

_static_inline bool allowednextmove(uint8_t *, uint8_t);
_static_inline uint8_t inverse_trans(uint8_t);
_static_inline uint8_t movebase(uint8_t);
_static_inline uint8_t moveaxis(uint8_t);

_static bool
allowednextmove(uint8_t *moves, uint8_t n)
{
	uint8_t base[3], axis[3];

	if (n < 2)
		return true;

	base[0] = movebase(moves[n-1]);
	axis[0] = moveaxis(moves[n-1]);
	base[1] = movebase(moves[n-2]);
	axis[1] = moveaxis(moves[n-2]);

	if (base[0] == base[1] || (axis[0] == axis[1] && base[0] < base[1]))
		return false;

	if (n == 2)
		return true;

	base[2] = movebase(moves[n-3]);
	axis[2] = moveaxis(moves[n-3]);

	return axis[1] != axis[2] || base[0] != base[2];
}

_static_inline uint8_t
inverse_trans(uint8_t t)
{
	return inverse_trans_table[t];
}

_static_inline uint8_t
movebase(uint8_t move)
{
	return move / 3;
}

_static_inline uint8_t
moveaxis(uint8_t move)
{
	return move / 6;
}

/******************************************************************************
Section: auxiliary procedures for H48 optimal solver (temporary)
******************************************************************************/

#define _esep_ind(i)      (i / 8U)
#define _esep_shift(i)    (4U * (i % 8U))
#define _esep_mask(i)     (((1U << 4U) - 1U) << _esep_shift(i))
#define _visited_ind(i)   (i / 8U)
#define _visited_mask(i)  (1U << (i % 8U))

typedef struct {
	cube_fast_t cube;
	uint8_t *visited;
	uint8_t *moves;
	uint8_t nmoves;
	uint8_t depth;
	uint16_t *nclasses;
	uint32_t *cocsepdata;
	uint32_t *buf32;
} dfsarg_gendata_t;

_static size_t gendata_cocsep(void *);
_static uint32_t gendata_cocsep_dfs( /* TODO: use dfsarg */
    cube_fast_t, uint8_t, uint8_t, uint16_t *, uint32_t *, uint8_t *);

_static size_t gendata_esep(const void *, void *);
_static uint32_t gendata_esep_dfs(dfsarg_gendata_t *);

_static_inline bool get_visited(const uint8_t *, int64_t);
_static_inline void set_visited(uint8_t *, int64_t);
_static_inline uint8_t get_esep_pval(const uint32_t *, int64_t);
_static_inline void set_esep_pval(uint32_t *, int64_t, uint8_t);

/*
Each element of the cocsep table is a uint32_t used as follows:
  - Lowest 8-bit block: pruning value
  - Second-lower 8-bit block: "ttrep" (transformation to representative)
  - Top 16-bit block: symcoord value
After the data as described above, more auxiliary information is appended:
  - A uint32_t representing the number of symmetry classes
  - A uint32_t representing the highest value of the pruning table
  - One uint32_t for each "line" of the pruning table, representing the number
    of positions having that pruning value.
*/
_static size_t
gendata_cocsep(void *buf)
{
	size_t tablesize = _3p7 << 7U;
	size_t visitedsize = (tablesize + 7U) / 8U;
	size_t infosize = 12;

	cube_fast_t solved;
	uint32_t *buf32, *info, cc;
	uint16_t n;
	uint8_t i, j, visited[visitedsize];

	buf32 = (uint32_t *)buf;
	info = buf32 + tablesize;
	memset(buf32, 0xFFU, 4*tablesize);
	memset(info, 0, 4*infosize);

	solved = cubetofast(solvedcube());
	for (i = 0, n = 0, cc = 0; i < 10; i++) {
		memset(visited, 0, visitedsize);
		DBG_LOG("cocsep: generating depth %" PRIu8 "\n", i);
		cc = gendata_cocsep_dfs(solved, 0, i, &n, buf32, visited);
		info[i+2] = cc;
		DBG_LOG("found %" PRIu32 "\n", cc);
	}

	info[0] = (uint32_t)n;
	info[1] = 9U; /* Known max pruning value */
	DBG_ASSERT(n == COCSEP_CLASSES, 0,
	    "cocsep: computed %" PRIu16 " symmetry classes, "
	    "expected %" PRIu16 "\n", n, COCSEP_CLASSES);

	DBG_LOG("cocsep data computed\n");
	DBG_LOG("Symmetry classes: %" PRIu32 "\n", info[0]);
	DBG_LOG("Maximum pruning value: %" PRIu32 "\n", info[1]);
	DBG_LOG("Pruning value distribution:\n");
	for (j = 0; j < 10; j++)
		DBG_LOG("%" PRIu8 ":\t%" PRIu32 "\n", j, info[j+2]);

	return 4*(tablesize + infosize);
}

_static uint32_t
gendata_cocsep_dfs(
	cube_fast_t c,
	uint8_t depth,
	uint8_t maxdepth,
	uint16_t *n,
	uint32_t *buf32,
	uint8_t *visited
)
{
	uint8_t m, t, tinv, olddepth;
	uint32_t cc;
	int64_t i;
	cube_fast_t d;

	i = coord_fast_cocsep(c);
	olddepth = (uint8_t)(buf32[i] & 0xFFU);
	if (olddepth < depth || get_visited(visited, i))
		return 0;
	set_visited(visited, i);

	if (depth == maxdepth) {
		if ((buf32[i] & 0xFFU) != 0xFFU)
			return 0;

		for (t = 0, cc = 0; t < 48; t++) {
			d = transform(c, t);
			i = coord_fast_cocsep(d);
			set_visited(visited, i);
			tinv = inverse_trans(t);
			cc += (buf32[i] & 0xFFU) == 0xFFU;
			buf32[i] = (*n << 16U) | (tinv << 8U) | depth;
		}
		(*n)++;

		return cc;
	}

	for (m = 0, cc = 0; m < 18; m++) {
		d = move(c, m);
		cc += gendata_cocsep_dfs(d, depth+1, maxdepth, n, buf32, visited);
	}

	return cc;
}

/*
TODO description
generating fixed table with h=0, k=4
*/
_static size_t
gendata_esep(const void *cocsepdata, void *buf)
{
	size_t tablesize = (COCSEP_CLASSES * _12c4 * _8c4) / 2U;
	size_t visitedsize = (tablesize * 2U + 7U) / 8U;
	size_t infosize = 25; /* TODO unknown yet */

	uint32_t *buf32, *info, cc;
	uint8_t moves[20];
	dfsarg_gendata_t arg;

	arg.visited = malloc(visitedsize);
	buf32 = (uint32_t *)buf;
	info = buf32 + tablesize;
	memset(buf32, 0xFFU, 4*tablesize);
	memset(info, 0, 4*infosize);

	arg.cube = cubetofast(solvedcube());
	arg.moves = moves;
	arg.nmoves = 0;
	arg.cocsepdata = (uint32_t *)cocsepdata;
	arg.buf32 = buf32;
	/* TODO loop until no more is done, not until 12! (or hardcode limits)*/
	for (arg.depth = 0, cc = 0; arg.depth < 12; arg.depth++) {
		DBG_LOG("esep: generating depth %" PRIu8 "\n", arg.depth);
		memset(arg.visited, 0, visitedsize);
		cc = gendata_esep_dfs(&arg);
		info[arg.depth+1] = cc;
		DBG_LOG("found %" PRIu32 "\n", cc);
	}
	free(arg.visited);

	return cc;
}

_static uint32_t
gendata_esep_dfs(dfsarg_gendata_t *arg)
{
	uint8_t m, t, olddepth;
	uint32_t cc;
	uint64_t i;
	cube_fast_t d;
	dfsarg_gendata_t nextarg;

	if (!allowednextmove(arg->moves, arg->nmoves))
		return 0;

	if (arg->nmoves > 0)
		arg->cube = move(arg->cube, arg->moves[arg->nmoves-1]);

	i = coord_h48(arg->cube, arg->cocsepdata, 0U);
	olddepth = get_esep_pval(arg->buf32, i);

	if (olddepth < arg->nmoves || get_visited(arg->visited, i))
		return 0;
	set_visited(arg->visited, i);

	if (arg->nmoves == arg->depth) {
		if (olddepth != 15U)
			return 0;

		for (t = 0, cc = 0; t < 48; t++) {
			d = transform(arg->cube, t);
			i = coord_h48(d, arg->cocsepdata, 0U);
			set_visited(arg->visited, i);
			cc += get_esep_pval(arg->buf32, i) == 15U;
			set_esep_pval(arg->buf32, i, arg->depth);
		}

		return cc;
	}


	nextarg = *arg;
	nextarg.nmoves = arg->nmoves + 1;
	for (m = 0, cc = 0; m < 18; m++) {
		nextarg.cube = arg->cube;
		nextarg.moves[arg->nmoves] = m;
		cc += gendata_esep_dfs(&nextarg);
	}

	return cc;
}

_static_inline bool get_visited(const uint8_t *a, int64_t i)
{
	return a[_visited_ind(i)] & _visited_mask(i);
}

_static_inline void set_visited(uint8_t *a, int64_t i)
{
	a[_visited_ind(i)] |= _visited_mask(i);
}

_static_inline uint8_t
get_esep_pval(const uint32_t *buf32, int64_t i)
{
	return (buf32[_esep_ind(i)] & _esep_mask(i)) >> _esep_shift(i);
}

_static_inline void
set_esep_pval(uint32_t *buf32, int64_t i, uint8_t val)
{
	buf32[_esep_ind(i)] =
	    (buf32[_esep_ind(i)] & (~_esep_mask(i))) | (val << _esep_shift(i));
}

/******************************************************************************
Section: solvers

Here you can find the implementation of all the solving algorithms.
******************************************************************************/

typedef struct {
	cube_fast_t cube;
	uint8_t depth;
	int64_t maxsols;
	char **nextsol;
	int64_t *nsols;
	uint8_t nmoves;
	uint8_t moves[20];
	uint8_t (*estimate)(cube_fast_t);
} dfsarg_generic_t;

_static void solve_generic_appendsolution(dfsarg_generic_t *);
_static int solve_generic_dfs(dfsarg_generic_t *);
_static int64_t solve_generic(cube_t, const char *, int8_t, int8_t, int64_t,
    int8_t, char *, uint8_t (*)(cube_fast_t));
_static uint8_t estimate_simple(cube_fast_t);
_static int64_t solve_simple(cube_t, int8_t, int8_t, int64_t, int8_t, char *);

int64_t
solve(
	cube_t cube,
	const char *solver,
	const char *options,
	const char *nisstype,
	int8_t minmoves,
	int8_t maxmoves,
	int64_t maxsols,
	int8_t optimal,
	const void *data,
	char *solutions
)
{
	DBG_WARN(!strcmp(options, ""),
	     "solve: 'options' not implemented yet, ignoring\n");

	DBG_WARN(!strcmp(nisstype, ""),
	     "solve: NISS not implemented yet, ignoring 'nisstype'\n");

	DBG_WARN(data == NULL,
	    "solve: 'data' not implemented yet, ignoring\n");

	if (!strcmp(solver, "optimal") || !strcmp(solver, "simple")) {
		return solve_simple(
			cube,
			minmoves,
			maxmoves,
			maxsols,
			optimal,
			solutions
		);
	} else {
		DBG_LOG("solve: unknown solver '%s'\n", solver);
		return -1;
	}

	DBG_LOG("solve: error\n");
	return -1;
}

void
multisolve(
	int n,
	cube_t *cube,
	const char *solver,
	const void *data,
	char *sols
)
{
	char *s;
	int i;

	s = sols;
	for (i = 0; i < n; i++) {
		solve(cube[i], solver, "", "normal", 0, -1, 1, 0, NULL, s);
		while (s++);
	}
}

int64_t
gendata(const char *solver, void *data)
{
	DBG_LOG("gendata: not implemented yet\n");

	return -1;
}

_static void
solve_generic_appendsolution(dfsarg_generic_t *arg)
{
	int strl;

	strl = writemoves(arg->moves, arg->depth, *arg->nextsol);
	DBG_LOG("Solution found: %s\n", *arg->nextsol);
	*arg->nextsol += strl;
	**arg->nextsol = '\n';
	(*arg->nextsol)++;
	(*arg->nsols)++;
}

_static int
solve_generic_dfs(dfsarg_generic_t *arg)
{
	dfsarg_generic_t nextarg;
	uint8_t m, bound;
	int64_t ret;

	if (!allowednextmove(arg->moves, arg->nmoves))
		return 0;

	if (arg->nmoves > 0)
		arg->cube = move(arg->cube, arg->moves[arg->nmoves-1]);

	bound = arg->estimate(arg->cube);
	if (*arg->nsols == arg->maxsols || bound + arg->nmoves > arg->depth)
		return 0;

	if (bound == 0) {
		if (arg->nmoves != arg->depth)
			return 0;
		solve_generic_appendsolution(arg);
		return 1;
	}

	/* memcpy(&nextarg, arg, sizeof(dfsarg_generic_t)); */
	nextarg = *arg;
	nextarg.nmoves = arg->nmoves + 1;
	for (m = 0, ret = 0; m < 18; m++) {
		nextarg.cube = arg->cube;
		nextarg.moves[arg->nmoves] = m;
		ret += solve_generic_dfs(&nextarg);
	}

	return ret;
}

_static int64_t
solve_generic(
	cube_t cube,
	const char *nisstype,
	/* TODO: handle NISS */
	int8_t minmoves,
	int8_t maxmoves,
	int64_t maxsols,
	int8_t optimal,
	char *sols,
	uint8_t (*estimate)(cube_fast_t)
	/* TODO: add validator */
	/* TODO: maybe add data for estimate */
	/* TODO: add moveset (and allowednext?) */
)
{
	dfsarg_generic_t arg;
	int64_t ret, tmp, first;

	if (!issolvable(cube)) {
		DBG_LOG("solve: cube is not solvable\n");
		return -1;
	}

	if (issolved(cube)) {
		DBG_LOG("solve: cube is already solved\n");
		sols[0] = '\n';
		sols[1] = 0;
		return 1;
	}

	DBG_WARN(!strcmp(nisstype, ""),
	    "solve: NISS not implemented yet, 'nisstype' ignored\n");

	if (minmoves < 0) {
		DBG_LOG("solve: 'minmoves' is negative, setting to 0\n");
		minmoves = 0;
	}

	if (maxmoves < 0) {
		DBG_LOG("solve: invalid 'maxmoves', setting to 20\n");
		maxmoves = 20;
	}

	if (maxsols < 0) {
		DBG_LOG("solve: 'maxsols' is negative\n");
		return -1;
	}

	if (maxsols == 0) {
		DBG_LOG("solve: 'maxsols' is 0\n");
		return 0;
	}

	if (sols == NULL) {
		DBG_LOG("solve: return parameter 'sols' is NULL\n");
		return -1;
	}

	if (estimate == NULL) {
		DBG_LOG("solve: 'estimate' is NULL\n");
		return -1;
	}

	arg = (dfsarg_generic_t) {
		.cube = cubetofast(cube),
		.maxsols = maxsols,
		.nextsol = &sols,
		.nsols = &ret,
		.nmoves = 0,
		.moves = {0},
		.estimate = estimate,
	};

	ret = 0;
	first = -1;
	for (arg.depth = minmoves; arg.depth <= maxmoves; arg.depth++) {
		tmp = solve_generic_dfs(&arg);
		if (tmp != 0)
			first = arg.depth;

		DBG_LOG("Found %" PRId64 " solution%s at depth %" PRIu8 "\n",
		        tmp, tmp == 1 ? "" : "s", arg.depth);

		if (ret >= maxsols)
			break;

		if (optimal >= 0 && first >= 0 && arg.depth - first == optimal)
			break;
	}

	DBG_ASSERT(ret <= maxsols, ret,
	    "solve: found more than 'maxsols' solutions\n");

	return ret;
}

_static uint8_t
estimate_simple(cube_fast_t cube)
{
	return issolved_fast(cube) ? 0 : 1;
}

_static int64_t
solve_simple(
	cube_t cube,
	int8_t minmoves,
	int8_t maxmoves,
	int64_t maxsols,
	int8_t optimal,
	char *solutions
)
{
	return solve_generic(
		cube,
		"",
		minmoves,
		maxmoves,
		maxsols,
		optimal,
		solutions,
		&estimate_simple
	);
}