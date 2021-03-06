
#include "chess_index.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

const piece_t           PIECE_INDEX_PIECES[] = {QUEEN, ROOK, BISHOP, KNIGHT, PAWN};
const int               PIECE_INDEX_COUNTS[] = {1, 2, 2, 2, 8};

const cpiece_t          WHITE_PIECES[] = {WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT, WHITE_PAWN};
const cpiece_t          BLACK_PIECES[] = {BLACK_QUEEN, BLACK_ROOK, BLACK_BISHOP, BLACK_KNIGHT, BLACK_PAWN};


/********************************************************
* 		old masks
********************************************************/
/*{{{*/
/*
// file a - h
const int64 File_Mask = {
       -9187201950435737472,
        4629771061636907072,
        2314885530818453536,
        1157442765409226768,
         578721382704613384,
         289360691352306692,
         144680345676153346,
          72340172838076673,
};

// rank 1 - 8
const int64 Rank_Mask = {
                        255,
                      65280,
                   16711680,
                 4278190080,
              1095216660480,
            280375465082880,
          71776119061217280,
         -72057594037927936,
};

// diagonal sw -> ne
const int64 Diagonal_Mask = {
                         128,
                       32832,
                     8405024,
                  2151686160,
                550831656968,
             141012904183812,
           36099303471055874,
        -9205322385119247871,
         4620710844295151872,
         2310355422147575808,
         1155177711073755136,
          577588855528488960,
          288794425616760832,
          144396663052566528,
           72057594037927936
}

// diagagnols nw -> se
// https://en.wikipedia.org/wiki/Anti-diagonal_matrix
const int64 ADiagonal_Mask = {
         -9223372036854775808,
          4647714815446351872,
          2323998145211531264,
          1161999622361579520,
           580999813328273408,
           290499906672525312,
           145249953336295424,
            72624976668147840,
              283691315109952,
                1108169199648,
                   4328785936,
                     16909320,
                        66052,
                          258,
                            1,
}
*/

/*}}}*/


PG_FUNCTION_INFO_V1(char_to_int);

/********************************************************
* 		util
********************************************************/
/*{{{*/
Datum
char_to_int(PG_FUNCTION_ARGS)
{
    char			c = PG_GETARG_CHAR(0);
    PG_RETURN_INT32((int32)c);
}

//http://www.cse.yorku.ca/~oz/hash.html
unsigned int 
_sdbm_hash(char * str)
{
	unsigned long long hash = 0;
	int c;

	while ((c = *str++))
		hash = c + (hash << 6) + (hash << 16) - hash;
	return hash;
}

unsigned 
short _pindex_in(char * str)
{
    char            check[] = "QRRBBNNPPPPPPPP";
    unsigned short  result=0;
    unsigned char   i;

    //CH_NOTICE("_pindex_in: %s, %i", str, strlen(str));

    if (strlen(str) != PIECE_INDEX_SUM)
        BAD_TYPE_IN("pindex", str);

    for (i=0; i<=PIECE_INDEX_SUM; i++)
        if (!(str[i] == check[i] || str[i] == '.'))
            BAD_TYPE_IN("pindex", str);

    for (i=0; i<=PIECE_INDEX_SUM; i++)
        if (str[i] != '.')
            SET_BIT16(result, PIECE_INDEX_SUM -1 - i);

    //CH_NOTICE("val out: %i, size:%ui", result, sizeof(result));
    //
    return result;
}

/**
 * Ansi C "itoa" based on Kernighan & Ritchie's "Ansi C":
 */
	
static void 
strreverse(char* begin, char* end) {
	char aux;
	while(end>begin)
		aux=*end, *end--=*begin, *begin++=aux;
}
	
void 
ch_itoa(int value, char* str, int base) {
	
	static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	char* wstr=str;
	int sign;

	// Validate base
	if (base<2 || base>35){ *wstr='\0'; return; }
	
	// Take care of sign
	if ((sign=value) < 0) value = -value;

	// Conversion. Number is reversed.
	do *wstr++ = num[value%base]; while(value/=base);
	if(sign<0) *wstr++='-';
	*wstr='\0';
	// Reverse string
	strreverse(str,wstr-1);
}

/*}}}*/
/********************************************************
* 		diagonals
********************************************************/
char _diagonal_in(char square)/*{{{*/
{
    char            d;

    switch (square+1) {
        case 1:
            d = -7; break;
        case 9: case 2:
            d = -6; break;
        case 17: case 10: case 3:
            d = -5; break;
        case 25: case 18: case 11: case 4:
            d = -4; break;
        case 33: case 26: case 19: case 12: case 5:
            d = -3; break;
        case 41: case 34: case 27: case 20: case 13: case 6:
            d = -2; break;
        case 49: case 42: case 35: case 28: case 21: case 14: case 7:
            d = -1; break;
        case 57: case 50: case 43: case 36: case 29: case 22: case 15: case 8:
            d = 0; break;
        case 58: case 51: case 44: case 37: case 30: case 23: case 16:
            d = 1; break;
        case 59: case 52: case 45: case 38: case 31: case 24:
            d = 2; break;
        case 60: case 53: case 46: case 39: case 32:
            d = 3; break;
        case 61: case 54: case 47: case 40:
            d = 4; break;
        case 62: case 55: case 48:
            d = 5; break;
        case 63: case 56:
            d = 6; break;
        case 64:
            d = 7; break;
        default:
            CH_ERROR("bad square %d for diagonal", square);
            break;
    }
    return d;
}

char _adiagonal_in(char square)
{
    char            d;

    switch (square+1) {
        case 57:
            d = -7; break;
        case 49: case 58:
            d = -6; break;
        case 41: case 50: case 59:
            d = -5; break;
        case 33: case 42: case 51: case 60:
            d = -4; break;
        case 25: case 34: case 43: case 52: case 61:
            d = -3; break;
        case 17: case 26: case 35: case 44: case 53: case 62:
            d = -2; break;
        case 9: case 18: case 27: case 36: case 45: case 54: case 63:
            d = -1; break;
        case 1: case 10: case 19: case 28: case 37: case 46: case 55: case 64:
            d = 0; break;
        case 2: case 11: case 20: case 29: case 38: case 47: case 56:
            d = 1; break;
        case 3: case 12: case 21: case 30: case 39: case 48:
            d = 2; break;
        case 4: case 13: case 22: case 31: case 40:
            d = 3; break;
        case 5: case 14: case 23: case 32:
            d = 4; break;
        case 6: case 15: case 24:
            d = 5; break;
        case 7: case 16:
            d = 6; break;
        case 8:
            d = 7; break;
        default:
            CH_ERROR("bad square %d for adiagonal", square);
            break;
    }
    return d;
}
/*}}}*/




int
_get_array_arg(PG_FUNCTION_ARGS, const size_t idx, Datum ** valsContent, bool ** valsNullFlags)
{
    //https://github.com/pjungwir/aggs_for_arrays/blob/master/array_to_max.c
	ArrayType 			*vals;            // Our arguments:
	Oid 				valsType;           // The array element type:
	int16 				valsTypeWidth;      // The array element type widths for our input array:
	bool 				valsTypeByValue;    // The array element type "is passed by value" flags (not really used):
	char 				valsTypeAlignmentCode; // The array element type alignment codes (not really used):
	int32               valsLength;         // The size of the input array:

	if (PG_ARGISNULL(idx)) { ereport(ERROR, (errmsg("Null arrays not accepted"))); } 
	vals = PG_GETARG_ARRAYTYPE_P(idx);
	if (ARR_NDIM(vals) == 0) { ereport(ERROR, (errmsg("array has zero dimensions"))); }
	if (ARR_NDIM(vals) > 1) { ereport(ERROR, (errmsg("One-dimesional arrays are required"))); }

	valsType = ARR_ELEMTYPE(vals);
	valsLength = (ARR_DIMS(vals))[0];
	get_typlenbyvalalign(valsType, &valsTypeWidth, &valsTypeByValue, &valsTypeAlignmentCode);
	deconstruct_array(vals, valsType, valsTypeWidth, valsTypeByValue, valsTypeAlignmentCode, valsContent, valsNullFlags, &valsLength);
    return valsLength;
}
