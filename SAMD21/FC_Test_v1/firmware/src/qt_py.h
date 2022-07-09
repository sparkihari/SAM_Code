/* 
 * File:   qt_py.h
 * Author: haris
 *
 * Created on July 9, 2022, 11:37 PM
 */

#ifndef QT_PY_H
#define	QT_PY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "configuration.h"
#include "definitions.h"

#ifdef	__cplusplus
extern "C" {
#endif


// *****************************************************************************
// *****************************************************************************
// Section: Function prototypes 
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
sercom_registers_t * QT_PY_USARTAddressGet ( void );
void QT_PY_DmaInterruptDisbale(void);
void QT_PY_DmaInterruptEnable(void);
uint32_t QT_PY_USARTFrequencyGet(void);
bool QT_PY_USARTSetup(USART_SERIAL_SETUP * serialSetup, uint32_t clkFrequency);
USART_ERROR QT_PY_USARTErrorGet(void);



#ifdef	__cplusplus
}
#endif

#endif	/* QT_PY_H */

