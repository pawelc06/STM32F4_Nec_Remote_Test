/*
 * waveplayer.h
 *
 *  Created on: 18 lip 2014
 *      Author: Pawe³
 */

#ifndef WAVEPLAYER_H_
#define WAVEPLAYER_H_

#include "main.h"


void DAC_PreConfig(void);

void convertBufferTo12bit(int16_t * buffer);

void playWav(uint8_t * name);

#endif /* WAVEPLAYER_H_ */
