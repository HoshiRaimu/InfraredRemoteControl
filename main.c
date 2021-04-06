/*
 * File:   main.c
 * Author: raimu
 *
 * Created on 2021/04/03, 15:57
 */

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable (PWRT enabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will not cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

#include <xc.h>
#include <stdint.h>
#include <stdint.h>

#define _XTAL_FREQ 4000000

void sendInfrared(uint8_t *);

void main(void) {
    //PICマイコンの設定
    OSCCON = 0b01101000;
    ANSELA = 0b00000000;    //すべてデジタル
    TRISA  = 0b00010111;    //RA0とRA1とRA2とRA4を入力に設定
    
    //PWMの設定
    APFCONbits.CCP1SEL = 1;          //CCPをRA5に割り当て
    CCP1CONbits.CCP1M  = 0b1100;     //PWM機能を有効にして、PWM信号をactive-highに設定
    CCP1CONbits.P1M    = 0b00;       //RA2はGPIOに設定
    T2CONbits.T2CKPS   = 0b00;       //プリスケーラを1に設定
    PR2                = 25;        //38kHz  周期 = ( PR2 + 1 ) x 4 x 1クロック分の時間 x プリスケーラ値
    CCPR1L             = 35 / 4;     //デューティー比は1/3  デューティーサイクル = (10ビットの値) x 1クロック分の時間 x プリスケーラ値
    CCP1CONbits.DC1B   = 35 & 0b11;  //下位2ビット

    uint8_t data0[4] = {'S', 'C', 0x00, 0x00};
    uint8_t data1[4] = {'S', 'C', 0x00, 0x01};
    uint8_t data2[4] = {'S', 'C', 0x00, 0x02};
    uint8_t data3[4] = {'S', 'C', 0x00, 0x03};
    
    while(1) {
        if(RA0) sendInfrared(data0);
        else if(RA1) sendInfrared(data1);
        else if(RA2) sendInfrared(data2);
        else if(RA4) sendInfrared(data3);   
    }
    return;
}

void sendInfrared(uint8_t *data) {
    __delay_ms(50);     //安定化待ち
            
    //リーダーコードの送信
    TMR2ON = 1;
    __delay_us(9000);
    TMR2ON = 0;
    __delay_us(4500);

    //カスタムコード、データコードの送信
    for(int i = 0; i < 4; i++) {
        for(int j = 7; j >= 0; j--) {
            TMR2ON = 1;
            __delay_us(560);
            TMR2ON = 0;

            //0か1を送信
            if(data[i] & (0b00000001 << j)) {
                __delay_us(1690);
            }else {
                __delay_us(560);
            }
        }
    }

    //ストップビットの送信
    TMR2ON = 1;
    __delay_us(560);
    TMR2ON = 0;
    
    return;
}