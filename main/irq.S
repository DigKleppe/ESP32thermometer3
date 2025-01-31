#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/system.h>
#include "soc/gpio_reg.h"

#define    COMPARATOR_PIN_MSK   ( 1 << 1 )  // gpio 1 (COMPARATOR_PIN)

#define Pin_Mask            COMPARATOR_PIN_MSK 
#define PPS_INT_Sta_REG     GPIO_STATUS_REG         //where you can read the interrupt status 
#define Pin_INT_W1TC_REG    GPIO_STATUS_W1TC_REG    //where you can write the mask to to clear the interrupt status.

#define TIMR_ADDR_LO 0x6001f004 //(unsigned int*)TIMG_T0LO_REG(0); 
#define TIMR_ADDR_HI 0x6001f008 
#define TIMR_ADDR_UPD 0x6001f00c


.data
_l5_intr_stack:
.space      60
//Interrupt Handler
    .section .iram1,"ax" 
    .global     xt_highint5   
    .type       xt_highint5,@function
    .align      4
xt_highint5:
//////////////////////////////////////////////////////////////////
//save register values. It takes around 5 cpu cycles to save the register valeus.    
    movi    a0,     _l5_intr_stack
    s32i    a3,     a0,     0    
    s32i    a4,     a0,     4   //It takes 1 cpu cycle per save.
    s32i    a12,    a0,     8   
    s32i    a13,    a0,     12  
    s32i    a14,    a0,     16  
    s32i    a15,    a0,     20  

//////////////////////////////////////////////////////////////////

    movi a14, TIMR_ADDR_UPD    //Timer 0 Update address into a10
    s32i a0, a14, 0         //A write instruction copies the timer to the timer registers
    movi a14, timer_counter_value  //timer0_value address into a14
    movi a15, TIMR_ADDR_LO    //Timer 0 lower 32 bits address into a15
    l32i a13, a15, 0
    s32i a13, a14, 0        //Store lower 32 bits to timer0_value
    movi a15, TIMR_ADDR_HI    //Timer 0 higher 32 bits address into a15
    l32i a13, a15, 0
    s32i a13, a14, 4        //Store higher 32 bits to timer0_value
    memw

YourCodeHere:    
    //If you have multiple level-5 interrupt, you may have to determine which source triggered the interrupt
    //Clear GPIO interrupt status.
    movi    a3,            Pin_INT_W1TC_REG        //get REG to clear the interrupt status
    movi    a4,            Pin_Mask                //load Pin mask
    s32i    a4,            a3,            0       //write the mask to Int clear REG
    memw 

    //increase IntCounter[0] by 1
    movi    a3,    IntCounter      
    l32i    a4,    a3,    0
    addi.n  a4,    a4,    1
    s32i    a4,    a3,    0

    movi    a3,    irqFlg      // set flag
    l32i    a4,    a3,    0
    addi.n  a4,    a4,    1
    s32i    a4,    a3,    0

//Your code end here
//////////////////////////////////////////////////////////////////  

//Restore REG and return. 
SAVE_EXIT:
    movi    a0,     _l5_intr_stack
    l32i    a3,     a0,     0
    l32i    a4,     a0,     4
    l32i    a12,    a0,     8   
    l32i    a13,    a0,     12  
    l32i    a14,    a0,     16  
    l32i    a15,    a0,     20  

    rsr     a0, EXCSAVE_5                   /* restore a0 */
    rfi     5
/****************************************************
//Remember to add the following line in CMakeList.txt:
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_highint_hdl_my")
******************************************************/
    .global ld_include_highint_hdl_my       
ld_include_highint_hdl_my:


