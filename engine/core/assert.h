//
// Created by lasagnaphil on 2021-07-25.
//

#ifndef THESYSTEM_ASSERT_H
#define THESYSTEM_ASSERT_H

#if (__clang_analyzer__ || TS_USE_STDASSERT)
#include <assert.h>
#define TS_ASSERT(cond) assert(cond)
#define TS_ASSERT_MSG(cond, msg) assert(cond)
#define TS_ASSERT_DBG(conf, msg) assert(cond)
#define TS_ASSERT_DBG_MSG(conf, msg) assert(cond)
#endif

#define TS_ASSERT(cond) do { if (!(cond)) { }}

#endif //THESYSTEM_ASSERT_H
