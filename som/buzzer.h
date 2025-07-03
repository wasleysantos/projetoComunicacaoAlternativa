#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>

void buzzer_init(unsigned int pin);
void tone(unsigned int frequency, unsigned int duration);
void play_sos_alert(void);
void play_emergency_select_pattern(void);
void play_normal_select_pattern(void);
void play_word_end_pattern(void);

#endif // BUZZER_H
