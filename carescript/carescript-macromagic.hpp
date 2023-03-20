#ifndef CARESCRIPT_MACRO_MAGIC_HPP
#define CARESCRIPT_MACRO_MAGIC_HPP

// please ignore, it just... happened

#define _cc_error(...) do { settings.error_msg = __VA_ARGS__; return carescript::script_null; } while(0)
#define _cc_error_if(expr,...) if(!!(expr)) _cc_error(__VA_ARGS__); else do {} while(0)
#define _cc_empty(...)
#define _cc_defer(...) __VA_ARGS__ _cc_empty()
#define _cc_obstruct(...) __VA_ARGS__ _cc_defer(_cc_empty)()
#define _cc_eval(...)  _cc_eval1(_cc_eval1(_cc_eval1(__VA_ARGS__)))
#define _cc_eval1(...) _cc_eval2(_cc_eval2(_cc_eval2(__VA_ARGS__)))
#define _cc_eval2(...) _cc_eval3(_cc_eval3(_cc_eval3(__VA_ARGS__)))
#define _cc_eval3(...) _cc_eval4(_cc_eval4(_cc_eval4(__VA_ARGS__)))
#define _cc_eval4(...) _cc_eval5(_cc_eval5(_cc_eval5(__VA_ARGS__)))
#define _cc_eval5(...) __VA_ARGS__
#define _cc_second(a, b, ...) b
#define cc_builtin_var_requires(variable, ...) \
    if(_cc_eval(_cc_requires1(variable, __VA_ARGS__))) { \
        auto _rg = (variable); \
        _cc_error("argument " #variable " does match any of these types: "  _cc_chain(__VA_ARGS__) " (got: " + ((_rg)).get_type() + ")"); \
    } else do {} while (0)
#define cc_builtin_var_not_requires(variable, ...) \
    if(!(_cc_eval(_cc_requires1(variable, __VA_ARGS__)))) { \
        auto _rg = (variable); \
        _cc_error("argument " #variable " is not allowed to match any of these types: "  _cc_chain(__VA_ARGS__) " (got: " + ((_rg)).get_type() + ")"); \
    } else do {} while (0)
#define cc_builtin_same_type(variable1, variable2) if((variable1).get_type() != (variable2).get_type()) {\
        auto _rg1 = (variable1); \
        auto _rg2 = (variable2); \
        _cc_error(#variable1 " and "#variable2 " must have the same type (" #variable1 ": " + (_rg1).get_type() + " | " #variable2 ": " + (_rg2).get_type() + ")");\
    } else do {} while (0)
#define cc_builtin_arg_min(args, minimum) _cc_error_if(args.size() < minimum,\
       "argument minimum not reached (minimum: " + std::to_string(minimum) + " got: " + std::to_string(args.size()) + ")"\
    )
#define cc_builtin_arg_range(args, minimum, maximum) do {\
        cc_builtin_arg_min(args,minimum);\
        cc_builtin_arg_max(args,maximum);\
    }  while (0)
#define cc_builtin_arg_max(args, maximum) _cc_error_if(args.size() > maximum,\
        "argument maximum is reached (maximum: " + std::to_string(maximum) + " got: " + std::to_string(args.size()) + ")"\
    )
#define cc_builtin_if_ignore() do{ if(!settings.should_run.empty() && !settings.should_run.top()) return script_null; }while(0)
#define cc_operator_var_requires(variable, op, ...) \
    if(_cc_eval(_cc_requires1(variable, __VA_ARGS__))) { \
        _cc_error(op ": " #variable " doesn't match any of these types: "  _cc_chain(__VA_ARGS__) " (got: " + (variable).get_type() + ")"); \
    } else do {} while (0)
#define cc_operator_same_type(variable1, variable2, op) if((variable1).get_type() != (variable2).get_type()) {\
        _cc_error(#op ": " #variable1 " and "#variable2 " must have the same type (" #variable1 ": " + (variable1).get_type() + " | " #variable2 ": " + (variable2).get_type() + ")");\
    } else do {} while (0)
#define _cc_requires1(variable, type1, ...) _cc_second(__VA_OPT__(,) _cc_requires2(variable, type1, __VA_ARGS__), _cc_requires3(variable, type1))
#define _cc_requires2(variable, type1, ...) _cc_requires3(variable, type1) && _cc_obstruct(_cc_requires4)()(variable, __VA_ARGS__)
#define _cc_requires3(variable, type) !is_typeof<type>(variable)
#define _cc_requires4() _cc_requires1
#define _cc_chain(...) _cc_eval(_cc_chain1(__VA_ARGS__))
#define _cc_chain1(...) _cc_second(__VA_OPT__(,) _cc_chain2(__VA_ARGS__), )
#define _cc_chain2(car, ...) + (car()).get_type() + " " _cc_obstruct(_cc_chain3)()(__VA_ARGS__)
#define _cc_chain3() _cc_chain1
#define _cc_join(a,b) _cc_join2(a,b)
#define _cc_join2(a,b) a ## b

// #define CARESCRIPT_MACRO_MAGIC_DEF
// ^ well, better not. But I'll leave it here because it's funny
#ifdef CARESCRIPT_MACRO_MAGIC_DEF
#define _cc_onshot_def(decl,V,end) for( \
            decl; \
            V; \
            (V = end) \
        )
#define _cc_cps_run(...) [&](){ __VA_ARGS__ ; }()
#define _cc_after_run(...) _cc_after_run_if(true,__VA_ARGS__)
#define _cc_after_run_if(expr,...) for(bool _ccvar_afterrun = true; _ccvar_afterrun && (expr); (_ccvar_afterrun = 0, _cc_cps_run(__VA_ARGS__)))
#define _cc_with_run(...) for(bool _ccvar_withrun = true; _ccvar_withrun; _ccvar_withrun = 0) \
    for(__VA_ARGS__; _ccvar_withrun; _ccvar_withrun = 0)
/* environment */
static unsigned int constexpr _cc_ENVIRONMENT_ENTERED = 0;
#define cc_environment _cc_environment1(_cc_environment2, (__LINE__))
#define _cc_environment1(macro, args) macro args
#define _cc_environment2(uid) using namespace carescript;\
    if(false) \
        static_assert(_cc_ENVIRONMENT_ENTERED == 0, "`cc_environment` invoked twice."); \
    else \
        _cc_onshot_def(int _cc_ENVIRONMENT_TEMPORARY_##uid = 1, _cc_ENVIRONMENT_TEMPORARY_##uid,0) \
            _cc_onshot_def(unsigned int constexpr _cc_ENVIRONMENT_ENTERED = __LINE__,_cc_ENVIRONMENT_TEMPORARY_##uid,0) \
                _cc_onshot_def(carescript::ScriptSettings settings,_cc_ENVIRONMENT_TEMPORARY_##uid,0) \
                    _cc_onshot_def(std::string _cc_envv_error,_cc_ENVIRONMENT_TEMPORARY_##uid,0)

#define _cc_stringize(x) _cc_stringize1(x)
#define _cc_stringize1(x) #x
#define _cc_in_environment_check() \
  static_assert (_cc_ENVIRONMENT_ENTERED > 0, "`_cc_in_environment_check()` invoked outside of cc_environment.")

#define _cc_test_envmsg(msg) do { \
    _cc_in_environment_check(); \
    std::cout << msg << "\n"; \
} while(0)

#define ccenv_run(...) do { \
    _cc_in_environment_check(); \
    _cc_envv_error = run_script((__VA_ARGS__),settings); \
} while(0)

#define ccenv_iferr \
    _cc_after_run_if(_cc_envv_error != "" || settings.error_msg != "",settings.error_msg = "",_cc_cps_run(_cc_in_environment_check()),_cc_envv_error = "") 
#define ccenv_printerr(...) do { \
    _cc_in_environment_check(); \
    std::cout << _cc_envv_error << "\n"; \
    } while(0)
#define ccenv_error() _cc_envv_error
#define ccenv_errorcatch(_on_err) \
    _cc_after_run( \
        _cc_cps_run(_cc_in_environment_check()), \
        _cc_cps_run(ccenv_iferr _on_err()) \
    )
#define ccenv_statecapsule _ccenv_statecapsule2(_cc_join(_cc_envv_is,__LINE__))
#define _ccenv_statecapsule2(name) \
    _cc_with_run(carescript::InterpreterState name(true)) \
        _cc_after_run( \
            _cc_cps_run(_cc_in_environment_check()), \
            _cc_cps_run(name.load()) \
        )
#endif 

#endif