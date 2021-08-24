#ifndef _OVERLOAD_H
#define _OVERLOAD_H

#define ovrld2(_1, _2, func,...) func
#define ovrld3(_1, _2, _3, func,...) func

/* how to make overlad functions
 *
 * #define func_with_3_overloading(args...) ovrld3(args, _internal_3_input_func,\
 * 							_internal_2_input_func,\
 * 							_internal_1_input_func)(args)
 *
 */

#endif
