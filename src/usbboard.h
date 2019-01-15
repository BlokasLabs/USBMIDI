/*
 * DigiSpark (Micronucleus) specific config
 * This might work for other people too, but it's not enabled by default
 * For details, see:
 *  - https://github.com/micronucleus/micronucleus
 *  - https://digistump.com/wiki/digispark/tutorials/connecting
 *  - http://digistump.com/package_digistump_index.json
 */

#ifdef ARDUINO_AVR_DIGISPARK

#if defined (__AVR_ATtiny44__) || defined (__AVR_ATtiny84__)
#define USB_CFG_IOPORTNAME      B
#define USB_CFG_DMINUS_BIT      1
#define USB_CFG_DPLUS_BIT       2

#elif defined (__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)
#define USB_CFG_IOPORTNAME      B
#define USB_CFG_DMINUS_BIT      3
#define USB_CFG_DPLUS_BIT       4

#elif defined (__AVR_ATtiny87__) || defined (__AVR_ATtiny167__)
#define USB_CFG_IOPORTNAME      B
#define USB_CFG_DMINUS_BIT      3
#define USB_CFG_DPLUS_BIT       6

#elif defined (__AVR_ATtiny461__) || defined (__AVR_ATtiny861__)
#define USB_CFG_IOPORTNAME      B
#define USB_CFG_DMINUS_BIT      5
#define USB_CFG_DPLUS_BIT       6

#else /*	ATtiny2313, ATmega8/48/88/168/328	*/
#define USB_CFG_IOPORTNAME      D
#define USB_CFG_DMINUS_BIT      3
#define USB_CFG_DPLUS_BIT       2

#endif

#if defined (__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)
#define USB_INTR_CFG            PCMSK
#define USB_INTR_CFG_SET        (1<<USB_CFG_DPLUS_BIT)
#define USB_INTR_ENABLE_BIT     PCIE
#define USB_INTR_PENDING_BIT    PCIF
#define USB_INTR_VECTOR         SIG_PIN_CHANGE
#endif

#if defined (__AVR_ATtiny87__) || defined (__AVR_ATtiny167__)
#define USB_INTR_CFG            PCMSK1
#define USB_INTR_CFG_SET        (1 << USB_CFG_DPLUS_BIT)
#define USB_INTR_CFG_CLR        0
#define USB_INTR_ENABLE         PCICR
#define USB_INTR_ENABLE_BIT     PCIE1
#define USB_INTR_PENDING        PCIFR
#define USB_INTR_PENDING_BIT    PCIF1
#define USB_INTR_VECTOR         PCINT1_vect
#endif

#endif /* ARDUINO_AVR_DIGISPARK */
