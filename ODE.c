#include <msp430.h>
#include <stdio.h>

#define maxVal 4095														
#define BR2400_H 0x01													
#define BR2400_L 0xB4													

#define DIGIT2ASCII(x) (x + 48)
#define ASCII2DIGIT(x) (x - 48)

#define dataLen 8														 

int correctSequenceLen = 0;												
unsigned char conversionCompleted = 0;										
unsigned char selNum = 1; 															          // possible values : 1, 2, 3, 4

unsigned int val; 											                          // displayed value

const unsigned char cypTo7Seg[] = {0x7E, 0x30, 0x6D, 0x79, 0x33,	
								   0x5B, 0x5F, 0x70, 0x7F, 0x7B};		        

unsigned int ledNum = 1;  												
unsigned int tempLedNum = 1;											
																		

unsigned char data[dataLen]; 											                //  uC->PC message
unsigned char dataIndex = dataLen; 										            // in  Cxxxx<CR><LF> format

int ADCResults[4];														



int scaleToPercent(unsigned int val) {
    return (int)(((double)val/4096.0)*100.0);
}																																

void activateLed(unsigned char ledNum) {
    if (ledNum < 1 || ledNum > 4) exit(1);
    long int x = scaleToPercent(val);
    TB0CCR3 = 0; TB0CCR4 = 0; TB0CCR5 = 0; TB0CCR6 = 0;   				
    if (ledNum == 1)													
    	 TB0CCR3 = x;
    else if (ledNum == 2)
    	 TB0CCR4 = x;
    else if (ledNum == 3)
    	 TB0CCR5 = x;
    else
    	 TB0CCR6 = x;
}

int main(void) {
    WDTCTL = WDTPW + WDTHOLD;
    val = 1803;

    P4DIR = 0x78;														
    P4SEL = 0x78;														

    P6DIR = 0x7F;  												
    P11DIR = 0x03; 														// b : 0000 0011 activated displays 1 and 2 (P11.1 i P11.0)
    P10DIR = 0xC0; 														// b : 1100 0000 activated displays 3 and 4 (P10.7 i P10.6)

    P11OUT = 0x02; 														
															
    TB0CCR0 = 100;
    TB0CCR3 = 100; TB0CCR4 = 75; TB0CCR5 = 50; TB0CCR6 = 25;

    TB0CCTL3 = OUTMOD_7; TB0CCTL4 = OUTMOD_7;							// reset/set
	  TB0CCTL5 = OUTMOD_7; TB0CCTL6 = OUTMOD_7; 												
    TB0CTL = TBSSEL_1 + MC_1; 											      // ACLK, up mode


    TA0CCR0 = 200; 														            // ~ 160 Hz
    TA0CTL = TASSEL_1 + MC_1; 											      // ACLK, up mode
    TA0CCTL0 = CCIE; 													            // CCR0 interrupt enabled

    P3SEL |= BIT4 | BIT5;												          // alternate selection dedicated to UART
    UCA0CTL1 |= UCSWRST;
    UCA0CTL1 |= UCSSEL_2;
    UCA0BR0 = BR2400_L;
    UCA0BR1 = BR2400_H;
    UCA0MCTL |= UCBRS_7;
    UCA0CTL1 &= ~UCSWRST;
    UCA0IE |= UCRXIE + UCTXIE; 											      // reciever and transmitter interrupts enabled

    P7SEL |= BIT7 | BIT6; 	
    P5SEL |= BIT1 | BIT0;

    ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_2;							// multiple conversion
    ADC12CTL1 = ADC12SHP+ADC12CONSEQ_1;

    ADC12MCTL0 = ADC12INCH_14;											      // chanell initialization
    ADC12MCTL1 = ADC12INCH_15;
    ADC12MCTL2 = ADC12INCH_8;
    ADC12MCTL3 = ADC12INCH_9 + ADC12EOS;														

    ADC12IE |= ADC12IE3;												
    ADC12CTL0 |= ADC12ENC;												

    __bis_SR_register(GIE); 											        // masked interrupts enabled globally
	
    while (1) {
        if (conversionCompleted) {
            dataIndex = 0;
            activateLed(ledNum);

            data[0] = 0;
            data[1] = 'C';

            data[2] = DIGIT2ASCII(val / 1000);
            data[3] = DIGIT2ASCII((val % 1000) / 100);
            data[4] = DIGIT2ASCII((val % 100) / 10);
            data[5] = DIGIT2ASCII(val % 10);

            data[6] = 0x0A; 											          // <CR>
            data[7] = 0x0D; 											          // <LF>
            UCA0TXBUF = data[dataIndex++];

            conversionCompleted = 0;
        }
    }
}

// Timer A0 interrupt routine
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
    unsigned char x;
    selNum++;
    if (selNum == 5)
        selNum = 1;

    P11OUT = 0x03; 														
    P10OUT = 0xC0; 														
																		
    switch(selNum) {													             // !! display activation, active 0 !!
        case 1 : {
            x = val / 100;
            x %= 10;
            printf("%d", x);
            P6OUT = cypTo7Seg[x];
            P11OUT = 0x02; 												
            break;
        }
        case 2 : {
            x = val / 1000;
            P6OUT = cypTo7Seg[x];
            P11OUT = 0x01; 												
            break;
        }

        case 3 : {
            x = val % 10;
            P6OUT = cypTo7Seg[x];
            P10OUT = 0x80; 												
            break;
        }
        case 4 : {
            x = val % 100;
            x /= 10;
            P6OUT = cypTo7Seg[x];
            P10OUT = 0x40; 												
            break;
        }

    }
}

// USCI interrupt routine
#pragma vector = USCI_A0_VECTOR
__interrupt void usciA0handler(void) {
    unsigned char nextChar;

    switch(UCA0IV) {

        case 2 : {														            // receiver interrupt
            nextChar = UCA0RXBUF;

            if (correctSequenceLen == 0) {
                if (nextChar == 'S')
                   correctSequenceLen++;
                else
                   correctSequenceLen = 0;
            }
            else if (correctSequenceLen == 1) {
                tempLedNum = ASCII2DIGIT(nextChar);
                if (tempLedNum >= 1 && tempLedNum <= 4)
                    correctSequenceLen++;

                else
                    correctSequenceLen = 0;
            }
            else if (correctSequenceLen == 2) {
                if (nextChar == 0x0D) {
                   ledNum = tempLedNum;
                   ADC12CTL0 |= ADC12SC; 								// correct sequence received, conversion start
                }
                correctSequenceLen = 0;
            }
        }
        case 4 : {														          // transmitter interrupt
            if (dataIndex < dataLen)
                UCA0TXBUF = data[dataIndex++];
        }
        default : break;
    }
}

// ADC12 interrupt routine
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void) {
    switch (ADC12IV) {
        case 12 : {														    
            ADCResults[0] = ADC12MEM0;
            ADCResults[1] = ADC12MEM1;
            ADCResults[2] = ADC12MEM2;
            ADCResults[3] = ADC12MEM3;
            ADC12IFG = 0x0000;											
            val = ADCResults[ledNum - 1];							
            conversionCompleted = 1;								
            break;
        }
        default : break;
    }
}
