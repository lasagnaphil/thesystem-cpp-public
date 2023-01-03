//
// Created by lasagnaphil on 8/15/2021.
//

// From https://gist.github.com/ForeverZer0/0a4f80fc02b96e19380ebb7a3debbee5

#ifndef THESYSTEM_TIMER_H
#define THESYSTEM_TIMER_H

/*
  Easy embeddable cross-platform high resolution timer function. For each
  platform we select the high resolution timer. You can call the 'ns()'
  function in your file after embedding this.
*/
#include <stdint.h>

uint64_t time_ns();

#endif //THESYSTEM_TIMER_H
