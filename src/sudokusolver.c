#ifndef _SUDOKUSOLVER_SOURCE_
#define _SUDOKUSOLVER_SOURCE_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "debugp.h"

#define DEBUG_LEVEL 0 
//#define PRINT_AS_YOU_SOLVE

typedef struct {
	int blankcount;
	int depth;
	int *blanks;
	int magic[81];
	int ints[81];
	uint16_t history[81];
} puzzle_t;

int bitcount( uint16_t word )
{
    uint64_t x = word;
    const uint64_t m1  = 0x5555555555555555ULL;
    const uint64_t m2  = 0x3333333333333333ULL;
    const uint64_t h01 = 0x0101010101010101ULL;
    const uint64_t m4  = 0x0f0f0f0f0f0f0f0fULL;
    x -= (x >> 1) & m1;
    x = (x & m2) + ((x >> 2) & m2);
    x = (x + (x >> 4)) & m4;
    int retval = (x * h01)>>56;
	return retval;
}

int get_xy( puzzle_t *puz, int col, int row )
{
	return puz->ints[row*9+col];
}

void set_xy( puzzle_t *puz, int col, int row, int val )
{
	//debugp( 8, "set_xy(%d,%d,%d)\n", col, row, val );
	puz->ints[row*9+col] = val;
	return;
}

int gethx_xy( puzzle_t *puz, int col, int row )
{
	return puz->history[row*9+col];
}

int getmagic_xy( puzzle_t *puz, int col, int row )
{
	int magic = puz->magic[row*9+col];
	debugp( 8, "getmagic_xy(%d,%d) = %d\n", col, row, magic );
	return magic;
}

void sethx_xy( puzzle_t *puz, int col, int row, int val )
{
	puz->history[row*9+col] = val;
	return;
}

void markhx_xy( puzzle_t *puz, int col, int row, int val )
{
	puz->history[row*9+col] |= (1<<val);
	return;
}

void setmagic_xy( puzzle_t *puz, int col, int row, int val )
{
	//debugp( 8, "setmagic_xy(%d,%d,%d)\n", col, row, val );
	puz->magic[row*9+col] = val;
	return;
}

void print_puzzle( puzzle_t *puz, char marker )
{
	int *i = puz->ints;
	for( int row = 0; row < 9; row++ )
	{
		for( int col = 0; col < 9; col++ )
		{
			int val = *i++;
			if( val == 0 )
				printf( ".  " );
			else
			{
				uint16_t hx = gethx_xy( puz, col, row );
				char c;
				if( hx & (1<<0) )
					c = marker;
				else
					c = ' ';
				printf( "%d%c ", val, c );
			}

			if( !((col+1)%3) && col < 8 )
				printf( "| " );
		}
		printf( "\n" );
		if( !((row+1)%3) && row < 8 )
			printf( "---------+----------+----------\n" );
	}
	printf( "\n" );
	return;
}

int squarenum( int col, int row )
{
	return (row/3) * 3 + (col/3);
}

uint16_t get_possible_by_square( puzzle_t *puz, int square )
{
	int square_x = (square % 3) * 3;
	int square_y = (square / 3) * 3;
	
	// Find which nums are used
	uint16_t used = 0;
	for( int row = square_y; row < (square_y+3); row++ )
		for( int col = square_x; col < (square_x+3); col++ )
		{
			int val = get_xy(puz,col,row);
			if(val)
			{
				if( used & (1<<val) )
				{
					debugp( 0, "Error: puzzle found invalid while checking square #%d\n", square );
					used = 0xffff;
					break;
				}
				else
				{
					used |= (1<<val);
				}
			}
		}

	uint16_t avail = (~used & 0x3fe);

	return avail;
}
uint16_t get_possible_by_row( puzzle_t *puz, int row )
{
	// Find which nums are used
	uint16_t used = 0;
	for( int col = 0; col < 9; col++ )
	{
		int val = get_xy(puz,col,row);
		if(val)
		{
			if( used & (1<<val) )
			{
				debugp( 0, "Error: puzzle found invalid while checking row %d\n", row );
				used = 0xffff;
				break;
			}
			else
			{
				used |= (1<<val);
			}
		}
	}
	uint16_t avail = (~used & 0x3ff);
	return avail;
}
uint16_t get_possible_by_col( puzzle_t *puz, int col )
{
	// Find which nums are used
	uint16_t used = 0;
	for( int row = 0; row < 9; row++ )
	{
		int val = get_xy(puz,col,row);
		if(val)
		{
			if( used & (1<<val) )
			{
				debugp( 0, "Error: puzzle found invalid while checking col %d\n", col );
				used = 0xffff;
				break;
			}
			else
			{
				used |= (1<<val);
			}
		}
	}
	uint16_t avail = (~used & 0x3ff);
	return avail;
}

uint16_t possible( puzzle_t *puz, int col, int row )
{
	uint16_t choices = 0;
	
	uint16_t row_choices = get_possible_by_row( puz, row );
	uint16_t col_choices = get_possible_by_col( puz, col );
	uint16_t square_choices = get_possible_by_square( puz, squarenum(col,row) );

	if( 
			(square_choices == 0xffff) ||
			(row_choices == 0xffff) ||
			(col_choices == 0xffff) )
	{
		choices = 0xffff;
	}
	else
	{
		choices = square_choices;
		choices &= row_choices;
		choices &= col_choices;
	}

	return choices;
}

void reset_vals( puzzle_t *puz, int which )
{
	int idx = 0;
	int skip = which;
	for( int row = 0; row < 9; row++ )
		for( int col = 0; col < 9; col++ )
		{
			if( idx > skip )
			{
				uint16_t hx = gethx_xy( puz, col, row );
				if( hx & (1<<0) )
				{
					set_xy( puz, col, row, 0 );
				}
			}
			idx++;
		}
	return;
}

uint64_t max_iterations( puzzle_t *puz )
{
	uint64_t perms = -1;
	uint64_t last = 0;

	for( int i=0; i < puz->blankcount; i++ )
	{
		int idx = puz->blanks[i];
		uint16_t pos = possible( puz, idx % 9, idx / 9 );
		if( pos == 0xffff )
		{
			perms = 0;
			break;
		}
		else
		{
			int poscount = bitcount(pos);

			if( perms == -1 )
				perms = poscount;
			else
				perms *= poscount;

			if( perms < last )
				perms = 0xffffffffffffffff;

			last = perms;
			debugp( 8, "poscount=%d; perms=%llu\n", poscount, perms );
		}
	}

	debugp( 8, "max: %llu\n", perms );
	return perms;
}

int blankchange( puzzle_t *puz, int which, int direction )
{
	int retval = -1;
	
	int *bp = puz->blanks + which;

	int idx = *bp;
	int val = puz->ints[idx];

	puz->ints[idx] = 0;
	uint16_t pos = possible( puz, (idx % 9), idx / 9 );
	int poscount = bitcount(pos);

	if( poscount > 0 )
	{
		debugp( 8, "found %d possible for (%d,%d): ", poscount,idx%9,idx/9 );

		// make a list
		int *poslist = malloc( sizeof(int) * poscount );
		int *pp = poslist;
		int *current = NULL;
		for( int i=1; i <=9; i++ )
		{
			if( pos & (1<<i) )
			{
				debugp( 8, "%d,", i );
				*pp = i;
				if( i == val )
					current = pp;
				pp++;
			}
		}
		debugp( 8, "\n" );
		
		if( val == 0 )
		{
			// if the value was originally unset
			current = poslist;
			puz->ints[idx] = *current;
			retval = 1;
		}

		// go up or down
		else if( direction < 0 )
		{
			if( current <= poslist )
			{
				puz->ints[idx] = 0;
				retval = -200;
			}
			else
			{
				current--;
				puz->ints[idx] = *current;
				retval = 1;
			}
		}
		else if( direction > 0 )
		{
			current++;
			if( (current-poslist) >= poscount )
			{
				// overflow
				current = poslist;
				puz->ints[idx] = 0;
				retval = -2;
			}
			else
			{
				puz->ints[idx] = *current;
				retval = 1;
			}
		}

		free( poslist );
	}
	else
	{
		retval = -1;
	}
	debugp( 8, "blankchange( %d, %d ) = %d\n", which, direction, retval );
	return retval;
}

int puzzle_solve( puzzle_t *puz, int *its )
{
	int retval = -1;

	int level = 0;
	uint64_t step = 0;
	uint64_t max_step = max_iterations(puz);

	if( max_step == 0 )
	{
		debugp( 0, "Error: puzzle invalid. Not solving\n" );
	}
	else
	{
		debugp( 8, "max_step = %llu\n", max_step );

		int *leveltrack = malloc( sizeof(int) * puz->blankcount );
		memset( leveltrack, 0, puz->blankcount * sizeof(int) );
		
		while( (level < (puz->blankcount)) && ((step++) < max_step) )
		{
			debugp( 8, "Step %llu/%llu: ", step, max_step );
			int rval = blankchange( puz, level, 1 );
			switch(rval)
			{
				case 1: 
					// successful
					level += 1;	
					break;

				// Cases causing backtracking
				case -1: 
					// could not find possible number
					leveltrack[level]++;
					level -= 1;
					break;
				case -2:
					debugp( 8, "Overflow at level %d\n", level );
					leveltrack[level]++;
					level -= 1;	
					break;
			
				// Invalid cases
				case -200:
					debugp( 8, "Underflow at level %d\n", level );
					assert(0);
					level -= 1;	
					break;
				default:
					debugp( 8, "Unhandled case: %d\n", rval );
					assert(0);
					break;
			}
			//print_puzzle( puz );
			//sleep(1);
		
#ifdef PRINT_AS_YOU_SOLVE
			char clear[] = { 0x1b,0x5b,0x48,0x1b,0x5b,0x32,0x4a,0x00};
			printf( "%s", clear );
			print_puzzle( puz, ' ' );
#endif
		}

		for( int l=0; l < puz->blankcount; l++ )
		{
			debugp( 8, "Level %2d: %7d backtracks\n", l, leveltrack[l] );
		}

		free(leveltrack);

		*its = step;
		retval = 0;
	}
	return retval;
}

void puzzle_init( puzzle_t *puz )
{
	int blank_count = 0;
	puz->depth = 0;

	// Find which numbers are used
	for( int col = 0; col < 9; col++ )
    {
		for( int row = 0; row < 9; row++ )
		{
			int val = get_xy( puz, col, row );
			if( val )
			{
				// Set history to zero
				sethx_xy( puz, col, row, 0 );
			}
			else
			{
				// Set history to one instance of (1<<0)
				sethx_xy( puz, col, row, (1<<0) );
				setmagic_xy( puz, col, row, 0 );
				blank_count++;
			}
		}
    }

	puz->blankcount = blank_count;
	if( puz->blankcount )
	{
		puz->blanks = malloc( sizeof(int) * puz->blankcount );
	}
	else
		puz->blanks = NULL;

	int *ip = puz->blanks;
	int idx = 0;
	for( int row = 0; row < 9; row++ )
    {
		for( int col = 0; col < 9; col++ )
		{
			if( get_xy(puz,col,row) == 0 )
			{
				*ip = idx;
				ip++;
			}
			idx++;
		}
    }
	return;
}

void puzzle_free( puzzle_t *puz )
{
	free(puz->blanks);
	return;
}

int main( int argc, char *argv[] )
{
	set_debug_level( DEBUG_LEVEL );

	if( argc < 2 )
	{
		debugp( 0, "Error: specify input file as first argument\n" );
		exit(-1);
	}

	debugp( 0, "Input file: `%s'\n", argv[1] );

	FILE *fp;
	if( ( fp = fopen( argv[1], "rb" ) ) == NULL )
	{
		debugp( 0, "Error: cannot open file `%s'\n", argv[1] );
		exit(-1);
	}

	debugp( 0, "Loading puzzle...\n" );
	
	int c;
	puzzle_t puz;
	int *pp = puz.ints;
	while( ( c = fgetc(fp) ) != EOF )
	{
		int num = -1;
		if( (c>='0') && (c<='9') )
		{
			num = c - '0';
		}
		else if( (c=='.') || (c==' ') || (c=='*') )
		{
			num = 0;
		}

		if( num >= 0 )
		{
			*pp++ = num;
		}
	}

	if( (pp-puz.ints) != 81 )
	{
		debugp( 8, "Error: invalid puzzle format\n" );
		exit(-1);
	}

	debugp( 8, "Puzzle loaded:\n" );
	puzzle_init( &puz );

	puzzle_t original;
	memcpy( &original, &puz, sizeof(puzzle_t) );

	int its;
	int result = puzzle_solve( &puz, &its );
	print_puzzle(&original,'*');
	
	if( result == 0 )
	{
		debugp( 0, "Solved with %d iterations:\n", its );
		print_puzzle(&puz,'*');
	}
	
	return 0;
}

#endif
