/*
 * waveplayer.c
 *
 *  Created on: 18 lip 2014
 *      Author: Pawe³
 */

#include "waveplayer.h"
#define SAMPLE_BUFFER_SIZE 512

void convertBufferTo12bit(int16_t * buffer){
	uint16_t i;
	for(i=0;i<SAMPLE_BUFFER_SIZE;i++){
		buffer[i] = (buffer[i]+ 32768)>>4;
	}
}
