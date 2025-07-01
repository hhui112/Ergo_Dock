#ifndef __JUDGER
#define __JUDGER
#include "stdint.h"
//ÅÐ¶ÏÆ÷½á¹¹Ìå
typedef struct {
    int threshold_move;
    int threshold_position;
    int threshold_left;
} Judger;
extern int in_state;
extern Judger judger;
extern int position_state;
extern uint16_t threshold_move;
extern uint16_t threshold_position;
extern uint16_t threshold_left;

void init_judger(Judger* judger);
int judger_he(void);
void judgerInit(void);
void sf_refreshParameter(void);
int judger_left(void);
int judger_position(void);
#endif
