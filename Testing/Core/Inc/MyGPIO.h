#ifndef MY_GPIO_H
#define MY_GPIO_H

#define GPIO_Px0_POSITION 0
#define GPIO_Px1_POSITION 1
#define GPIO_Px2_POSITION 2
#define GPIO_Px3_POSITION 3
#define GPIO_Px4_POSITION 4
#define GPIO_Px5_POSITION 5
#define GPIO_Px6_POSITION 6
#define GPIO_Px7_POSITION 7
#define GPIO_Px8_POSITION 8
#define GPIO_Px9_POSITION 9
#define GPIO_Px10_POSITION 10
#define GPIO_Px11_POSITION 11
#define GPIO_Px12_POSITION 12
#define GPIO_Px13_POSITION 13
#define GPIO_Px14_POSITION 14
#define GPIO_Px15_POSITION 15

#define GPIO_MODER_MASK(POS) (0b11U << ((POS) * 2))

#define GPIO_MODER_ALTERNATE_FUNCTION(POS) (0b10U << ((POS) * 2))
#define GPIO_MODER_OUTPUT(POS) (0b01U << ((POS) * 2))
#define GPIO_MODER_ANALOG(POS) (0b11U << ((POS) * 2))

#define GPIO_OTYPER_MASK(POS) (0b1U << (POS))

#define AF0 0b0000U
#define AF1 0b0001U
#define AF2 0b0010U
#define AF3 0b0011U
#define AF4 0b0100U
#define AF5 0b0101U
#define AF6 0b0110U
#define AF7 0b0111U

#define GPIO_AFR_MASK(POS) (0b1111U << ((POS) * 4))
#define GPIO_AFR_PERIPHERY(AF, POS) ((AF) << ((POS) * 4))

#define GPIOA_ODR (GPIOA->ODR)
#define GPIOC_ODR (GPIOC->ODR)

#endif