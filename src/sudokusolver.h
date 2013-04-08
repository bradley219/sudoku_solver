#ifndef _SUDOKUSOLVER_H_
#define _SUDOKUSOLVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "debugp.h"

#define DEBUG_LEVEL 0 
#define PRINT_AS_YOU_SOLVE

typedef struct {
	int blankcount;
	int depth;
	int *blanks;
	int magic[81];
	int ints[81];
	uint16_t history[81];
} puzzle_t;

#endif
