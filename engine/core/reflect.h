#pragma once

#define CLASS(...) class __attribute__((annotate("reflect-class;" #__VA_ARGS__)))
#define UNION(...) union __attribute__((annotate("reflect-class;" #__VA_ARGS__)))
#define PROPERTY(...) __attribute__((annotate("reflect-property;" #__VA_ARGS__)))
#define FUNCTION(...) __attribute__((annotate("reflect-function;" #__VA_ARGS__)))
#define META_OBJECT
