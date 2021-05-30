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

#define _XTAL_FREQ 4000000

void sendInfrared(uint8_t *, uint16_t *);

void main(void) {
    //PICマイコンの設定
    OSCCON = 0b01101000;
    ANSELA = 0b00000110;    //RA2をアナログに設定
    TRISA  = 0b00000111;    //RA0とRA2を入力に設定
    
    //タイマー1の設定
    T1CONbits.TMR1CS = 0b00;    //クロック源に1MHzを使用
    T1CONbits.T1CKPS = 0b11;    //プリスケール値は1:8
    T1CONbits.TMR1ON = 0;       //タイマー1を停止
    
    //PWMの設定
    APFCONbits.CCP1SEL = 1;          //CCPをRA5に割り当て
    CCP1CONbits.CCP1M  = 0b1100;     //PWM機能を有効にして、PWM信号をactive-highに設定
    CCP1CONbits.P1M    = 0b00;       //RA2はGPIOに設定
    T2CONbits.T2CKPS   = 0b00;       //プリスケーラを1に設定
    PR2                = 25;         //38kHz  周期 = ( PR2 + 1 ) x 4 x 1クロック分の時間 x プリスケーラ値
    CCPR1L             = 35 / 4;     //デューティー比は1/3  デューティーサイクル = (10ビットの値) x 1クロック分の時間 x プリスケーラ値
    CCP1CONbits.DC1B   = 35 & 0b11;  //下位2ビット
    
    ADCON0bits.ADON   = 1;           //ADCを有効
    ADCON1bits.ADFM   = 1;           //結果を右詰めに格納
    ADCON1bits.ADCS   = 0b0000;      //ADCクロックを選択
    ADCON1bits.ADPREF = 0b00;        //参照電圧をVddに設定
    
    IOCAPbits.IOCAP0 = 1;            //RA0の立ち上がりエッジ状態変化割り込みを有効
    INTCONbits.IOCIE = 1;            //状態変化割り込みを有効
    INTCONbits.GIE   = 1;            //グローバル割り込みを有効

    uint8_t data0[4] = {'S', 'C', 0x00, 0x00};
    uint8_t data1[4] = {'S', 'C', 0x00, 0x01};
    uint8_t data2[4] = {'S', 'C', 0x00, 0x02};
    uint8_t data3[4] = {'S', 'C', 0x00, 0x03};
    
    TMR1ON = 1;
    uint16_t cnt = 0;                //スリープに入るまでの時間計測用変数
    
    LATA4 = 1;                       //スリープモード判断用LEDを光らせる

    while(1) {
        //ADCを使ってボタンの押されている状況を見る
        ADCON0bits.CHS    = 0b00010;     //チャンネル(AN2)を設定
        ADCON0bits.ADGO = 1;
        while(ADGO);
        int data = (int)ADRES;
        __delay_ms(10);
        ADCON0bits.CHS    = 0b00001;     //チャンネル(AN1)を設定
        ADCON0bits.ADGO = 1;
        while(ADGO);
        uint8_t val = ADRES >> 2;
        
        //0xFFごとにcntをインクリメントし、ボタンが押されずに1分ほど経過したらスリープモードに入る
        if(TMR1H >= 0xFF) {
            cnt++;
            if(cnt >= 1700) {
                LATA4 = 0;              //スリープに入るから消しとく

                TRISA  = 0b11111111;    //全部を入力に設定すると低消費電力になるみたい
                IOCAFbits.IOCAF0 = 0;   //割り込みフラグをリセット
                
                SLEEP();
                NOP();
                
                TRISA  = 0b00000101;    //元に戻しておく
                
                LATA4 = 1;              //スリープが解除したから点灯しとく
                cnt = 0;   
            }
            
        }
        
        if(data >=0 && data <= 50) {
            data0[2] = val;
            sendInfrared(data0, &cnt);
        } else if(data >= 480 && data <= 530) {
            data1[2] = val;
            sendInfrared(data1, &cnt);
        } else if(data >= 650 && data <= 710) {
            data2[2] = val;
            sendInfrared(data2, &cnt);
        } else if(data >= 740 && data <= 800) {
            data3[2] = val;
            sendInfrared(data3, &cnt);
        }
    }
    return;
}

void sendInfrared(uint8_t *data, uint16_t *cnt) {
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
    
    //タイマー1のカウンタを初期化
    TMR1H = 0;
    TMR1L = 0;
    
    //ボタンが押されたら再び0からインクリメントする
    *cnt = 0;
    
    return;
}