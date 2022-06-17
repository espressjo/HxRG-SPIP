#ifndef REGDEF_H
#define REGDEF_H

#define OUTPUT 0x4011 // 4011<5-0> (1,2,4,16,32)
#define COL 0x4050 //H4RG-10 0x0000, H4RG-15 dis. 0x0001, H4RG-15 ena. 0x0003 4050<0> (0 H4RG-10, 1 H4RG-15), 4050<1> (0 disable, 1 enable)
#define CW 0x4015 // 4015<0> = 0->cold, 1->warm
#define KTC_ 0x4016 // 4016<0> 0-> no ktc, 1->use ktc, 4016<1> 0 reset once per frame (cold) 1 once per row (warm)
#define GAIN 0x401a // h401a<3-0> 0 to 15 (check gain map)
#define EXPMODE 0x4018 // 4018<0> (0 upthe ramps, 1 fowler), 4018<1> (0 full frame, 1 window)
#define INTERLEAVE 0x4052 // 4052<0> (0 disabled, 1 enabled)
#define CLOCKING 0x4017 //4017<0> (0 normal, 1 enhanced), 4017<1> (0 line by line, 1 global)
#define RREAD 0x4001
#define RRAMP 0x4003
#define RESET 0x4000
#define DROP 0x4005
#define GROUP 0x4004



#endif // REGDEF_H
