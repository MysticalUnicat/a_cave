#ifndef _PP_H_
#define _PP_H_

#define _pp_cat_1(x, y) x ## y
#define _pp_cat_0(x, y) _pp_cat_1(x, y)
#define CAT(x, y) _pp_cat_0(x, y)

#define _pp_eval_0(...) __VA_ARGS__
#define _pp_eval_1(...) _pp_eval_0(_pp_eval_0(_pp_eval_0(_pp_eval_0(__VA_ARGS__))))
#define _pp_eval_2(...) _pp_eval_1(_pp_eval_1(_pp_eval_1(_pp_eval_1(__VA_ARGS__))))
#define _pp_eval _pp_eval_2

#define _pp_end(...)
#define _pp_out
#define _pp_comma ,

#define _pp_get_end2() 0, _pp_end
#define _pp_get_end1(...) _pp_get_end2
#define _pp_get_end(...) _pp_get_end1
#define _pp_next_0(X, Y, ...) Y _pp_out
#define _pp_next_1(X, Y) _pp_next_0(X, Y, 0)
#define _pp_next(X, Y) _pp_next_1(_pp_get_end X, Y)

#define _pp_map_0(F, X, PEEK, ...) F(X) _pp_next(PEEK, _pp_map_1)(F, PEEK, __VA_ARGS__)
#define _pp_map_1(F, X, PEEK, ...) F(X) _pp_next(PEEK, _pp_map_0)(F, PEEK, __VA_ARGS__)

#define MAP(F, ...) _pp_eval(_pp_map_1(F, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

#endif // _PP_H_
