#include "ten_api.h"
#include "ten_state.h"
#include "ten_assert.h"
#include "ten_macros.h"
#include "ten_api.h"
#include "ten_com.h"
#include "ten_gen.h"
#include "ten_env.h"
#include "ten_fmt.h"
#include "ten_sym.h"
#include "ten_str.h"
#include "ten_idx.h"
#include "ten_rec.h"
#include "ten_fun.h"
#include "ten_cls.h"
#include "ten_fib.h"
#include "ten_upv.h"
#include "ten_dat.h"
#include "ten_ptr.h"
#include "ten_lib.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

struct ApiState {
    Scanner   scan;
    Finalizer finl;
    
    TVal val1;
    TVal val2;
};

void
apiFinl( State* state, Finalizer* finl ) {
    ApiState* api = structFromFinl( ApiState, finl );
    stateRemoveScanner( state, &api->scan );
    stateFreeRaw( state, api, sizeof(ApiState) );
}

void
apiScan( State* state, Scanner* scan ) {
    ApiState* api = structFromScan( ApiState, scan );
    
    tvMark( api->val1 );
    tvMark( api->val2 );
}

void
apiInit( State* state ) {
    Part apiP;
    ApiState* api = stateAllocRaw( state, &apiP, sizeof(ApiState) );
    api->finl.cb = apiFinl; stateInstallFinalizer( state, &api->finl );
    api->scan.cb = apiScan; stateInstallScanner( state, &api->scan );
    
    api->val1 = tvUdf();
    api->val2 = tvUdf();
    
    stateCommitRaw( state, &apiP );
    
    state->apiState = api;
}

#ifdef ten_TEST
void
apiTest( State* state ) {
    ten_State* s = (ten_State*)state;
    
    // Stack.
    ten_Tup t1 = ten_pushA( s, "UNLIDS", true, (long)123, 1.2, "abc" );
    ten_Var t1v1 = { .tup = &t1, .loc = 0 };
    ten_Var t1v2 = { .tup = &t1, .loc = 1 };
    ten_Var t1v3 = { .tup = &t1, .loc = 2 };
    ten_Var t1v4 = { .tup = &t1, .loc = 3 };
    ten_Var t1v5 = { .tup = &t1, .loc = 4 };
    ten_Var t1v6 = { .tup = &t1, .loc = 5 };
    
    tenAssert( ten_size( s, &t1 ) == 6 );
    tenAssert( ten_isUdf( s, &t1v1 ) );
    tenAssert( ten_isNil( s, &t1v2 ) );
    tenAssert( ten_isLog( s, &t1v3 ) );
    tenAssert( ten_isInt( s, &t1v4 ) );
    tenAssert( ten_isDec( s, &t1v5 ) );
    tenAssert( ten_isSym( s, &t1v6 ) );
    
    ten_Tup t2 = ten_top( s );
    ten_Var t2v1 = { .tup = &t2, .loc = 0 };
    ten_Var t2v2 = { .tup = &t2, .loc = 1 };
    ten_Var t2v3 = { .tup = &t2, .loc = 2 };
    ten_Var t2v4 = { .tup = &t2, .loc = 3 };
    ten_Var t2v5 = { .tup = &t2, .loc = 4 };
    ten_Var t2v6 = { .tup = &t2, .loc = 5 };
    
    tenAssert( ten_size( s, &t2 ) == 6 );
    tenAssert( ten_isUdf( s, &t2v1 ) );
    tenAssert( ten_isNil( s, &t2v2 ) );
    tenAssert( ten_isLog( s, &t2v3 ) );
    tenAssert( ten_isInt( s, &t2v4 ) );
    tenAssert( ten_isDec( s, &t2v5 ) );
    tenAssert( ten_isSym( s, &t2v6 ) );

    ten_Tup t3 = ten_dup( s, &t1 );
    ten_Var t3v1 = { .tup = &t3, .loc = 0 };
    ten_Var t3v2 = { .tup = &t3, .loc = 1 };
    ten_Var t3v3 = { .tup = &t3, .loc = 2 };
    ten_Var t3v4 = { .tup = &t3, .loc = 3 };
    ten_Var t3v5 = { .tup = &t3, .loc = 4 };
    ten_Var t3v6 = { .tup = &t3, .loc = 5 };

    tenAssert( ten_size( s, &t3 ) == 6 );
    tenAssert( ten_isUdf( s, &t3v1 ) );
    tenAssert( ten_isNil( s, &t3v2 ) );
    tenAssert( ten_isLog( s, &t3v3 ) );
    tenAssert( ten_isInt( s, &t3v4 ) );
    tenAssert( ten_isDec( s, &t3v5 ) );
    tenAssert( ten_isSym( s, &t3v6 ) );
    
    ten_pop( s );
    ten_pop( s );
    
    // Globals.
    ten_def( s, ten_sym( s, "var1" ), ten_int( s, 1 ) );
    ten_def( s, ten_sym( s, "var2" ), ten_int( s, -2 ) );
    ten_set( s, ten_sym( s, "var2" ), ten_int( s, 2 ) );
    
    tenAssert( ten_equal( s, ten_get( s, ten_sym( s, "var1" ) ), ten_int( s, 1 ) ) );
    tenAssert( ten_equal( s, ten_get( s, ten_sym( s, "var2" ) ), ten_int( s, 2 ) ) );
    
    // Types.
    ten_Var* type = ten_udf( s );
    ten_type( s, ten_int( s, 1 ), type );
    
    tenAssert( !strcmp( ten_getSymBuf( s, type ), "Int" ) );
    
    // Testing ten_expect() is a bit too difficult here,
    // will have to do it manually instead.
    
    // Misc.
    tenAssert( ten_equal( s, ten_int( s, 1 ), ten_int( s, 1 ) ) );
    tenAssert( !ten_equal( s, ten_int( s, 1 ), ten_int( s, 2 ) ) );
    tenAssert( !ten_equal( s, ten_int( s, 1 ), ten_dec( s, 1.0 ) ) );
    
    // Compilation and execution.
    char const* script =
        "def var3: 3\n"
        "def var4: 4\n";
    char const* expr =
        "var3 + var4";
    
    ten_Var* fib  = ten_udf( s );
    ten_Tup  args = ten_pushA( s, "" );
    ten_compileScript( s, script, ten_SCOPE_GLOBAL, ten_COM_FIB, fib );
    ten_Tup  rets = ten_cont( s, fib, &args );
    ten_pop( s );
    
    tenAssert( ten_size( s, &rets ) == 0 );
    tenAssert( ten_equal( s, ten_get( s, ten_sym( s, "var3" ) ), ten_int( s, 3 ) ) );
    tenAssert( ten_equal( s, ten_get( s, ten_sym( s, "var4" ) ), ten_int( s, 4 ) ) );
    
    rets = ten_executeExpr( s, expr );
    ten_Var ret = { .tup = &rets, .loc = 0 };
    
    tenAssert( ten_size( s, &rets ) == 1 );
    tenAssert( ten_equal( s, &ret, ten_int( s, 7 ) ) );
    
    // TODO: other tests.
}
#endif

void
ten_init( ten_State* s, ten_Config* config, jmp_buf* errJmp ) {
    State* state = (State*)s;
    stateInit( state, config, errJmp );
}

void
ten_finl( ten_State* s ) {
    State* state = (State*)s;
    stateFinl( state );
}

ten_Tup
ten_pushA( ten_State* s, char const* pat, ... ) {
    va_list ap; va_start( ap, pat );
    ten_Tup t = ten_pushV( s, pat, ap );
    va_end( ap );
    return t;
}

ten_Tup
ten_pushV( ten_State* s, char const* pat, va_list ap ) {
    State* state = (State*)s;
    uint   n   = (uint)strlen( pat );
    Tup    tup = statePush( state, n );
    
    for( uint i = 0 ; i < n ; i++ ) {
        switch( pat[i] ) {
            case 'U':
                tupAt( tup, i ) = tvUdf();
            break;
            case 'N':
                tupAt( tup, i ) = tvNil();
            break;
            case 'L':
                tupAt( tup, i ) = tvLog( va_arg( ap, int ) );
            break;
            case 'I':
                tupAt( tup, i ) = tvInt( va_arg( ap, long ) );
            break;
            case 'D':
                tupAt( tup, i ) = tvDec( va_arg( ap, double ) );
            break;
            case 'S': {
                char const* str = va_arg( ap, char const* );
                SymT        sym = symGet( state, str, strlen( str ) );
                tupAt( tup, i ) = tvSym( sym );
            } break;
            case 'P': {
                PtrT ptr = ptrGet( state, NULL, va_arg( ap, void* ) );
                tupAt( tup, i ) = tvPtr( ptr );
            } break;
            case 'V': {
                ten_Var* var = va_arg( ap, ten_Var* );
                tupAt( tup, i ) = ref(var);
            } break;
            default: {
                strAssert( false, "Invalid type char" );
            } break;
        }
    }
    
    ten_Tup t; memcpy( &t, &tup, sizeof(Tup) );
    return t;
}

ten_Tup
ten_top( ten_State* s ) {
    State* state = (State*)s;
    Tup    tup = stateTop( state );
    ten_Tup t; memcpy( &t, &tup, sizeof(Tup) );
    return t;
}

void
ten_pop( ten_State* s ) {
    State* state = (State*)s;
    statePop( state );
}

ten_Tup
ten_dup( ten_State* s, ten_Tup* tup ) {
    State* state = (State*)s;
    uint   n   = ((Tup*)tup)->size;
    Tup    dup = statePush( state, n );
    
    for( uint i = 0 ; i < n ; i++ )
        tupAt( dup, i ) = tupAt( *(Tup*)tup, i );
    
    ten_Tup t; memcpy( &t, &dup, sizeof(Tup) );
    return t;
}

unsigned
ten_size( ten_State* state, ten_Tup* tup ) {
    return ((Tup*)tup)->size;
}

void
ten_def( ten_State* s, ten_Var* name, ten_Var* val ) {
    State* state = (State*)s;
    TVal nameV = ref(name);
    funAssert( tvIsSym( nameV ), "Wrong type for 'name', need Sym", NULL );
    
    SymT nameS = tvGetSym( nameV );
    uint loc   = envAddGlobal( state, nameS );
    
    TVal* gval = envGetGlobalByLoc( state, loc );
    *gval = ref(val);
}

void
ten_set( ten_State* s, ten_Var* name, ten_Var* val ) {
    State* state = (State*)s;
    TVal nameV = ref(name);
    funAssert( tvIsSym( nameV ), "Wrong type for 'name', need Sym", NULL );
    
    SymT  nameS = tvGetSym( nameV );
    TVal* gval  = envGetGlobalByName( state, nameS );
    *gval = ref(val);
}

ten_Var*
ten_get( ten_State* s, ten_Var* name ) {
    State* state = (State*)s;
    TVal nameV = ref(name);
    funAssert( tvIsSym( nameV ), "Wrong type for 'name', need Sym", NULL );
    
    SymT     nameS = tvGetSym( nameV );
    TVal*    gval  = envGetGlobalByName( state, nameS );
    ten_Var* tmp   = stateTmp( state, tvUdf() );
    if( gval )
        ref(tmp) = *gval;
    
    return tmp;
}

void
ten_type( ten_State* s, ten_Var* var, ten_Var* dst ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    ref(dst) = tvSym( libType( state, ref(var) ) );
}

void
ten_expect( ten_State* s, char const* what, ten_Var* type, ten_Var* var ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    TVal typeV = ref(type);
    funAssert( tvIsSym( typeV ), "Wrong type for 'type', need Sym", NULL );
    
    libExpect( state, what, tvGetSym( typeV ), ref(var) );
}

bool
ten_equal( ten_State* s, ten_Var* var1, ten_Var* var2 ) {
    State* state = (State*)s;
    
    return tvEqual( ref(var1), ref(var2) );
}

ten_Var*
ten_udf( ten_State* s ) {
    State* state = (State*)s;
    return stateTmp( state, tvUdf() );
}

ten_Var*
ten_nil( ten_State* s ) {
    State* state = (State*)s;
    return stateTmp( state, tvNil() );
}

ten_Var*
ten_log( ten_State* s, bool log ) {
    State* state = (State*)s;
    return stateTmp( state, tvLog( log ) );
}

ten_Var*
ten_int( ten_State* s, long in ) {
    State* state = (State*)s;
    return stateTmp( state, tvInt( in ) );
}

ten_Var*
ten_dec( ten_State* s, double dec ) {
    State* state = (State*)s;
    return stateTmp( state, tvDec( dec ) );
}

ten_Var*
ten_sym( ten_State* s, char const* sym ) {
    State* state = (State*)s;
    SymT t = symGet( state, sym, strlen( sym ) );
    return stateTmp( state, tvSym( t ) );
}

ten_Var*
ten_ptr( ten_State* s, void* ptr ) {
    State* state = (State*)s;
    PtrT t = ptrGet( state, ptr, NULL );
    return stateTmp( state, tvPtr( t ) );
}

ten_Var*
ten_str( ten_State* s, char const* str ) {
    State* state = (State*)s;
    String* t = strNew( state, str, strlen( str ) );
    return stateTmp( state, tvObj( t ) );
}

typedef struct {
    ten_Source source;
    FILE*      file;
} FileSource;

typedef struct {
    Defer defer;
    FILE* file;
} CloseDefer;

static void
closeDefer( State* state, Defer* d ) {
    CloseDefer* defer = (CloseDefer*)d;
    fclose( defer->file );
}

static int
fileNext( ten_Source* s ) {
    FileSource* source = (FileSource*)s;
    return getc( source->file );
}

static Closure*
compileFile( State* state, FILE* file, ten_ComScope scope )  {
    CloseDefer defer = {
        .defer = { .cb = closeDefer },
        .file  = file
    };
    stateInstallDefer( state, (Defer*)&defer );
    
    FileSource source = {
        .source = { .next = fileNext },
        .file   = file
    };
    
    ComParams params = {
        .file   = "<unknown>",
        .params = NULL,
        .debug  = state->config.debug,
        .global = (scope == ten_SCOPE_GLOBAL),
        .script = true,
        .src    = (ten_Source*)&source
    };
    Closure* cls = comCompile( state, &params );
    
    stateCommitDefer( state, (Defer*)&defer );
    
    return cls;
}

void
ten_compileFile( ten_State* s, FILE* file, ten_ComScope scope, ten_ComType out, ten_Var* dst ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    Closure* cls = compileFile( state, file, scope );
    if( out == ten_COM_CLS )
        ref(dst) = tvObj( cls );
    else
        api->val1 = tvObj( cls );
    
    Fiber* fib = fibNew( state, cls );
    ref(dst) = tvObj( fib );
    
    api->val1 = tvUdf();
}

static Closure*
compilePath( State* state, char const* path, ten_ComScope scope ) {
    FILE* file = fopen( path, "r" );
    if( file == NULL )
        stateErrFmtA( state, ten_ERR_SYSTEM, "%s", strerror( errno ) );
    
    CloseDefer defer = {
        .defer = { .cb = closeDefer },
        .file  = file
    };
    stateInstallDefer( state, (Defer*)&defer );
    
    FileSource source = {
        .source = { .next = fileNext },
        .file   = file
    };
    
    ComParams params = {
        .file   = path,
        .params = NULL,
        .debug  = state->config.debug,
        .global = (scope == ten_SCOPE_GLOBAL),
        .script = true,
        .src    = (ten_Source*)&source
    };
    
    Closure* cls = comCompile( state, &params );
    
    stateCommitDefer( state, (Defer*)&defer );
    
    return cls;
}

void
ten_compilePath( ten_State* s, char const* path, ten_ComScope scope, ten_ComType out, ten_Var* dst ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    Closure* cls = compilePath( state, path, scope );
    if( out == ten_COM_CLS )
        ref(dst) = tvObj( cls );
    else
        api->val1 = tvObj( cls );
    
    Fiber* fib = fibNew( state, cls );
    ref(dst) = tvObj( fib );
    
    api->val1 = tvUdf();
}

typedef struct {
    ten_Source  source;
    char const* string;
    size_t      next;
} StringSource;

static int
stringNext( ten_Source* s ) {
    StringSource* source = (StringSource*)s;
    if( source->string[source->next] == '\0' )
        return -1;
    else
        return source->string[source->next++];
}

static Closure*
compileScript( State* state, char const* script, ten_ComScope scope ) {
    StringSource source = {
        .source = { .next = stringNext },
        .string = script,
        .next   = 0
    };
    
    ComParams params = {
        .file   = "<unknown>",
        .params = NULL,
        .debug  = state->config.debug,
        .global = (scope == ten_SCOPE_GLOBAL),
        .script = true,
        .src    = (ten_Source*)&source
    };
    return comCompile( state, &params );
}

void
ten_compileScript( ten_State* s, char const* script, ten_ComScope scope, ten_ComType out, ten_Var* dst ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    Closure* cls = compileScript( state, script, scope );
    if( out == ten_COM_CLS )
        ref(dst) = tvObj( cls );
    else
        api->val1 = tvObj( cls );
    
    Fiber* fib = fibNew( state, cls );
    ref(dst) = tvObj( fib );
    
    api->val1 = tvUdf();
}

static Closure*
compileExpr( State* state, char const** pnames, char const* expr ) {
    StringSource source = {
        .source = { .next = stringNext },
        .string = expr,
        .next   = 0
    };
    
    ComParams params = {
        .file   = "<unknown>",
        .params = pnames,
        .debug  = state->config.debug,
        .global = false,
        .script = false,
        .src    = (ten_Source*)&source
    };
    return comCompile( state, &params );
}

void
ten_compileExpr( ten_State* s, char const** pnames, char const* expr, ten_ComType out, ten_Var* dst ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    Closure* cls = compileExpr( state, pnames, expr );
    if( out == ten_COM_CLS )
        ref(dst) = tvObj( cls );
    else
        api->val1 = tvObj( cls );
    
    Fiber* fib = fibNew( state, cls );
    ref(dst) = tvObj( fib );
    
    api->val1 = tvUdf();
}

void
ten_executeFile( ten_State* s, FILE* file, ten_ComScope scope ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    Closure* cls = compileFile( state, file, scope );
    api->val1 = tvObj( cls );
    Fiber* fib = fibNew( state, cls );
    api->val1 = tvObj( fib );
    
    Tup args = statePush( state, 0 );
    fibCont( state, fib, &args );
    statePop( state );
    
    api->val1 = tvUdf();
    fibPropError( state, fib );
}

void
ten_executePath( ten_State* s, char const* path, ten_ComScope scope ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    Closure* cls = compilePath( state, path, scope );
    api->val1 = tvObj( cls );
    Fiber* fib = fibNew( state, cls );
    api->val1 = tvObj( fib );
    
    Tup args = statePush( state, 0 );
    fibCont( state, fib, &args );
    statePop( state );
    
    api->val1 = tvUdf();
    fibPropError( state, fib );
}

void
ten_executeScript( ten_State* s, char const* script, ten_ComScope scope ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    Closure* cls = compileScript( state, script, scope );
    api->val1 = tvObj( cls );
    Fiber* fib = fibNew( state, cls );
    api->val1 = tvObj( fib );
    
    Tup args = statePush( state, 0 );
    fibCont( state, fib, &args );
    statePop( state );
    
    api->val1 = tvUdf();
    fibPropError( state, fib );
}

ten_Tup
ten_executeExpr( ten_State* s, char const* expr ) {
    State*    state = (State*)s;
    ApiState* api   = state->apiState;
    
    Closure* cls = compileExpr( state, NULL, expr );
    api->val1 = tvObj( cls );
    Fiber* fib = fibNew( state, cls );
    api->val1 = tvObj( fib );
    
    Tup args = statePush( state, 0 );
    Tup ret  = fibCont( state, fib, &args );
    statePop( state );
    
    api->val1 = tvUdf();
    fibPropError( state, fib );
    
    ten_Tup t;
    memcpy( &t, &ret, sizeof(Tup) );
    return t;
}

bool
ten_isUdf( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    return tvIsUdf( ref(var) );
}

void
ten_setUdf( ten_State* s, ten_Var* dst ) {
    State* state = (State*)s;
    ref(dst) = tvUdf();
}

bool
ten_isNil( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    return tvIsNil( ref(var) );
}

void
ten_setNil( ten_State* s, ten_Var* dst ) {
    State* state = (State*)s;
    ref(dst) = tvUdf();
}

bool
ten_isLog( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    return tvIsLog( ref(var) );
}

void
ten_setLog( ten_State* s, bool log, ten_Var* dst ) {
    State* state = (State*)s;
    ref(dst) = tvLog( log );
}

bool
ten_getLog( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    funAssert( tvIsLog( ref(var) ), "Wrong type for 'var', need Log", NULL );
    return tvGetLog( ref(var) );
}

bool
ten_isInt( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    return tvIsInt( ref(var) );
}

void
ten_setInt( ten_State* s, long in, ten_Var* dst ) {
    State* state = (State*)s;
    ref(dst) = tvInt( in );
}

long
ten_getInt( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    funAssert( tvIsInt( ref(var) ), "Wrong type for 'var', need Int", NULL );
    return tvGetInt( ref(var) );
}

bool
ten_isDec( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    return tvIsDec( ref(var) );
}

void
ten_setDec( ten_State* s, double dec, ten_Var* dst ) {
    State* state = (State*)s;
    ref(dst) = tvDec( dec );
}

double
ten_getDec( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    funAssert( tvIsDec( ref(var) ), "Wrong type for 'var', need Dec", NULL );
    
    return tvGetDec( ref(var) );
}

bool
ten_isSym( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    return tvIsSym( ref(var) );
}

void
ten_setSym( ten_State* s, char const* sym, size_t len, ten_Var* dst ) {
    State* state = (State*)s;
    ref(dst) = tvSym( symGet( state, sym, len ) );
}

char const*
ten_getSymBuf( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    funAssert( tvIsSym( ref(var) ), "Wrong type for 'var', need Sym", NULL );
    return symBuf( state, tvGetSym( ref(var) ) );
}

size_t
ten_getSymLen( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    funAssert( tvIsSym( ref(var) ), "Wrong type for 'var', need Sym", NULL );
    return symLen( state, tvGetSym( ref(var) ) );
}

bool
ten_isPtr( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    return tvIsPtr( ref(var) );
}

void
ten_setPtr( ten_State* s, void* addr, ten_PtrInfo* info, ten_Var* dst ) {
    State* state = (State*)s;
    PtrInfo* pInfo = (PtrInfo*)info;
    funAssert(
        pInfo == NULL || pInfo->magic == PTR_MAGIC,
        "PtrInfo 'info' not initialized",
        NULL
    );
    
    ref(dst) = tvPtr( ptrGet( state, addr, pInfo ) );
}

void*
ten_getPtrAddr( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    funAssert( tvIsPtr( ref(var) ), "Wrong type for 'var', need Ptr", NULL );
    
    return ptrAddr( state, tvGetPtr( ref(var) ) );
}

ten_PtrInfo*
ten_getPtrInfo( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    funAssert( tvIsPtr( ref(var) ), "Wrong type for 'var', need Ptr", NULL );
    
    return (ten_PtrInfo*)ptrInfo( state, tvGetPtr( ref(var) ) );
}

char const*
ten_getPtrType( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    funAssert( tvIsPtr( ref(var) ), "Wrong type for 'var', need Ptr", NULL );
    
    PtrInfo* info = ptrInfo( state, tvGetPtr( ref(var) ) );
    if( info )
        return symBuf( state, info->type );
    else
        return "Ptr";
}

void
ten_initPtrInfo( ten_State* s, ten_PtrConfig* config, ten_PtrInfo* info ) {
    tenAssert( sizeof(ten_PtrInfo) >= sizeof(PtrInfo) );
    
    State*   state = (State*)s;
    PtrInfo* pInfo = (PtrInfo*)info;
    
    char const* type;
    if( config->tag )
        type = fmtA( state, false, "Ptr:%s", config->tag );
    else
        type = "Ptr";
    pInfo->type  = symGet( state, type, strlen( type ) );
    pInfo->destr = config->destr;
    pInfo->magic = PTR_MAGIC;
}

bool
ten_isStr( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    return tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_STR;
}

void
ten_newStr( ten_State* s, char const* str, size_t len, ten_Var* dst ) {
    State* state = (State*)s;
    ref(dst) = tvObj( strNew( state, str, len ) );
}

char const*
ten_getStrBuf( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    funAssert(
        tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_STR,
        "Wrong type for 'var', need Str",
        NULL
    );
    return strBuf( state, tvGetObj( val ) );
}

size_t
ten_getStrLen( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    funAssert(
        tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_STR,
        "Wrong type for 'var', need Str",
        NULL
    );
    return strLen( state, tvGetObj( val ) );
}

bool
ten_isIdx( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    return tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_IDX;
}

void
ten_newIdx( ten_State* s, ten_Var* dst ) {
    State* state = (State*)s;
    ref(dst) = tvObj( idxNew( state ) );
}

bool
ten_isRec( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    return tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_REC;
}

void
ten_newRec( ten_State* s, ten_Var* idx, ten_Var* dst ) {
    State* state = (State*)s;
    TVal idxV = ref(idx);
    funAssert(
        tvIsObj( idxV ) && datGetTag( tvGetObj( idxV ) ) == OBJ_IDX,
        "Wrong type for 'idx', need Idx",
        NULL
    );
    
    ref(dst) = tvObj( recNew( state, tvGetObj( idxV ) ) );
}

void
ten_recSep( ten_State* s, ten_Var* rec ) {
    State* state = (State*)s;
    TVal recV = ref(rec);
    funAssert(
        tvIsObj( recV ) && datGetTag( tvGetObj( recV ) ) == OBJ_REC,
        "Wrong type for 'rec', need Rec",
        NULL
    );
    recSep( state, tvGetObj( recV ) );
}

void
ten_recDef( ten_State* s, ten_Var* rec, ten_Var* key, ten_Var* val ) {
    State* state = (State*)s;
    TVal recV = ref(rec);
    funAssert(
        tvIsObj( recV ) && datGetTag( tvGetObj( recV ) ) == OBJ_REC,
        "Wrong type for 'rec', need Rec",
        NULL
    );
    recDef( state, tvGetObj( recV ), ref(key), ref(val) );
}
void
ten_recSet( ten_State* s, ten_Var* rec, ten_Var* key, ten_Var* val ) {
    State* state = (State*)s;
    TVal recV = ref(rec);
    funAssert(
        tvIsObj( recV ) && datGetTag( tvGetObj( recV ) ) == OBJ_REC,
        "Wrong type for 'rec', need Rec",
        NULL
    );
    recSet( state, tvGetObj( recV ), ref(key), ref(val) );
}

void
ten_recGet( ten_State* s, ten_Var* rec, ten_Var* key, ten_Var* dst ) {
    State* state = (State*)s;
    TVal recV = ref(rec);
    funAssert(
        tvIsObj( recV ) && datGetTag( tvGetObj( recV ) ) == OBJ_REC,
        "Wrong type for 'rec', need Rec",
        NULL
    );
    ref(dst) = recGet( state, tvGetObj( recV ), ref(key) );
}

bool
ten_isFun( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    return tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_FUN;
}

void
ten_newFun( ten_State* s, ten_FunParams* p, ten_Var* dst ) {
    State* state = (State*)s;
    
    SymT sparams[TUP_MAX];
    
    uint nParams = 0;
    bool vParams = false;
    if( p->params ) {
        for( uint i = 0 ; p->params[i] != NULL ; i++ ) {
            if( i > TUP_MAX )
                stateErrFmtA( state, ten_ERR_USER, "Too many parameters, max is %u", (uint)TUP_MAX );
            if( vParams )
                stateErrFmtA( state, ten_ERR_USER, "Extra parameters after '...'" );
            
            size_t len = 0;
            if( !isalpha( p->params[i][0] ) && p->params[i][0] != '_' )
                stateErrFmtA( state, ten_ERR_USER, "Invalid parameter name" );
            
            for( uint j = 0 ; p->params[i][j] != '\0' && p->params[i][j] != '.' ; j++ ) {
                if( !isalnum( p->params[i][j] ) && p->params[i][j] != '_' )
                    stateErrFmtA( state, ten_ERR_USER, "Invalid parameter name" );
                len++;
            }
            
            if( p->params[i][len] == '.' ) {
                if( !strcmp( &p->params[i][len], "..." ) )
                    vParams = true;
                else
                    stateErrFmtA( state, ten_ERR_USER, "Invalid parameter name" );
            }
            else {
                nParams++;
            }
            
            sparams[i] = symGet( state, p->params[i], len );
        }
    }
    
    Part paramsP;
    SymT* params = stateAllocRaw( state, &paramsP, sizeof(SymT)*nParams );
    memcpy( params, params, sizeof(SymT)*nParams );
    
    Function* fun =
        funNewNat( state, nParams, vParams ? idxNew( state ) : NULL, p->cb );
    fun->u.nat.params = params;
    if( p->name )
        fun->u.nat.name = symGet( state, p->name, strlen( p->name ) );
    
    stateCommitRaw( state, &paramsP );
    ref(dst) = tvObj( fun );
}

bool
ten_isCls( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    return tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_CLS;
}

void
ten_newCls( ten_State* s, ten_Var* fun, ten_Var* dat, ten_Var* dst ) {
    State* state = (State*)s;
    TVal funV = ref(fun);
    funAssert(
        tvIsObj( funV ) && datGetTag( tvGetObj( funV ) ) == OBJ_FUN,
        "Wrong type for 'fun', need Fun",
        NULL
    );
    Function* funO = tvGetObj( funV );
    
    Data* datO = NULL;
    if( dat ) {
        TVal datV = ref(dat);
        funAssert(
            tvIsObj( datV ) && datGetTag( tvGetObj( datV ) ) == OBJ_DAT,
            "Wrong type for 'dat', need Dat",
            NULL
        );
        datO = tvGetObj( datV );
    }
    
    if( funO->type == FUN_NAT )
        ref(dst) = tvObj( clsNewNat( state, funO, datO ) );
    else
        ref(dst) = tvObj( clsNewVir( state, funO, NULL ) );
}

bool
ten_isFib( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    return tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_FIB;
}

void
ten_newFib( ten_State* s, ten_Var* cls, ten_Var* dst ) {
    State* state = (State*)s;
    TVal clsV = ref(cls);
    funAssert(
        tvIsObj( clsV ) && datGetTag( tvGetObj( clsV ) ) == OBJ_CLS,
        "Wrong type for 'cls', need Cls",
        NULL
    );
    Closure* clsO = tvGetObj( clsV );
    ref(dst) = tvObj( fibNew( state, clsO ) );
}

ten_Tup
ten_cont( ten_State* s, ten_Var* fib, ten_Tup* args ) {
    State* state = (State*)s;
    TVal fibV = ref(fib);
    funAssert(
        tvIsObj( fibV ) && datGetTag( tvGetObj( fibV ) ) == OBJ_FIB,
        "Wrong type for 'fib', need Fib",
        NULL
    );
    Fiber* fibO = tvGetObj( fibV );
    
    Tup tup = fibCont( state, fibO, (Tup*)args );
    
    ten_Tup t;
    memcpy( &t, &tup, sizeof(Tup) );
    return t;
}

void
ten_yield( ten_State* s, ten_Tup* vals ) {
    State* state = (State*)s;
    
    funAssert( state->fiber, "Yield without running fiber", NULL );
    
    ten_dup( s, vals );
    fibYield( state );
}

void
ten_panic( ten_State* s, ten_Var* val ) {
    State* state = (State*)s;
    stateErrVal( state, ten_ERR_PANIC, ref(val) );
}

ten_Tup
ten_call_( ten_State* s, ten_Var* cls, ten_Tup* args, char const* file, unsigned line ) {
    State* state = (State*)s;
    
    funAssert( state->fiber, "Call without running fiber", NULL );
    
    TVal clsV = ref(cls);
    funAssert(
        tvIsObj( clsV ) && datGetTag( tvGetObj( clsV ) ) == OBJ_CLS,
        "Wrong type for 'cls', need Cls",
        NULL
    );
    Closure* clsO = tvGetObj( clsV );
    
    Tup tup = fibCall_( state, clsO, (Tup*)args, file, line );
    
    ten_Tup t;
    memcpy( &t, &tup, sizeof(Tup) );
    return t;
}

ten_ErrNum
ten_getErrNum( ten_State* s, ten_Var* fib ) {
    State* state = (State*)s;
    if( fib ) {
        TVal fibV = ref(fib);
        funAssert(
            tvIsObj( fibV ) && datGetTag( tvGetObj( fibV ) ) == OBJ_FIB,
            "Wrong type for 'fib', need Fib",
            NULL
        );
        Fiber* fibO = tvGetObj( fibV );
        return fibO->errNum;
    }
    else {
        return state->errNum;
    }
}

void
ten_getErrVal( ten_State* s, ten_Var* fib, ten_Var* dst ) {
    State* state = (State*)s;
    if( fib ) {
        TVal fibV = ref(fib);
        funAssert(
            tvIsObj( fibV ) && datGetTag( tvGetObj( fibV ) ) == OBJ_FIB,
            "Wrong type for 'fib', need Fib",
            NULL
        );
        Fiber* fibO = tvGetObj( fibV );
        ref(dst) = fibO->errVal;
    }
    else {
        ref(dst) = state->errVal;
    }
}

char const*
ten_getErrStr( ten_State* s, ten_Var* fib ) {
    State* state = (State*)s;
    if( fib ) {
        TVal fibV = ref(fib);
        funAssert(
            tvIsObj( fibV ) && datGetTag( tvGetObj( fibV ) ) == OBJ_FIB,
            "Wrong type for 'fib', need Fib",
            NULL
        );
        Fiber* fibO = tvGetObj( fibV );
        if( fibO->errStr )
            return fibO->errStr;
        else
            return fmtA( state, false, "%v", fibO->errVal );
    }
    else {
        if( state->errStr )
            return state->errStr;
        else
            return fmtA( state, false, "%v", state->errVal );
    }
}

ten_Trace*
ten_getTrace( ten_State* s, ten_Var* fib ) {
    State* state = (State*)s;
    return state->trace;
}

void
ten_clearError( ten_State* s, ten_Var* fib ) {
    State* state = (State*)s;
    if( fib ) {
        TVal fibV = ref(fib);
        funAssert(
            tvIsObj( fibV ) && datGetTag( tvGetObj( fibV ) ) == OBJ_FIB,
            "Wrong type for 'fib', need Fib",
            NULL
        );
        Fiber* fibO = tvGetObj( fibV );
        fibClearError( state, fibO );
    }
    else {
        stateClearError( state );
    }
}

void
ten_propError( ten_State* s ) {
    stateErrProp( (State*)s );
}

jmp_buf*
ten_swapErrJmp( ten_State* s, jmp_buf* errJmp ) {
    return stateSwapErrJmp( (State*)s, errJmp );
}

bool
ten_isDat( ten_State* s, ten_Var* var ) {
    State* state = (State*)s;
    TVal val = ref(var);
    return tvIsObj( val ) && datGetTag( tvGetObj( val ) ) == OBJ_DAT;
}

void*
ten_newDat( ten_State* s, ten_DatInfo* info, ten_Var* dst ) {
    State* state = (State*)s;
    funAssert( info, "DatInfo 'info' is required", NULL );
    
    DatInfo* dInfo = (DatInfo*)info;
    funAssert(
        dInfo->magic == DAT_MAGIC,
        "DatInfo 'info' not initialized",
        NULL
    );
    
    Data* dat = datNew( state, dInfo );
    ref(dst) = tvObj( dat );
    return dat->data;
}

void
ten_setMember( ten_State* s, ten_Var* dat, unsigned mem, ten_Var* val ) {
    State* state = (State*)s;
    TVal datV = ref(dat);
    funAssert(
        tvIsObj( datV ) && datGetTag( tvGetObj( datV ) ) == OBJ_DAT,
        "Wrong type for 'dat', need Dat",
        NULL
    );
    Data* datO = tvGetObj( datV );
    
    funAssert( mem < datO->info->nMems, "No member %u", mem );
    datO->mems[mem] = ref(val);
}

void
ten_getMember( ten_State* s, ten_Var* dat, unsigned mem, ten_Var* dst ) {
    State* state = (State*)s;
    TVal datV = ref(dat);
    funAssert(
        tvIsObj( datV ) && datGetTag( tvGetObj( datV ) ) == OBJ_DAT,
        "Wrong type for 'dat', need Dat",
        NULL
    );
    Data* datO = tvGetObj( datV );
    
    funAssert( mem < datO->info->nMems, "No member %u", mem );
    ref(dst) = datO->mems[mem];
}

ten_Tup
ten_getMembers( ten_State* s, ten_Var* dat ) {
    State* state = (State*)s;
    TVal datV = ref(dat);
    funAssert(
        tvIsObj( datV ) && datGetTag( tvGetObj( datV ) ) == OBJ_DAT,
        "Wrong type for 'dat', need Dat",
        NULL
    );
    Data* datO = tvGetObj( datV );
    
    Tup tup = { .base = &datO->mems, .offset = 0, .size = datO->info->nMems };
    
    ten_Tup t;
    memcpy( &t, &tup, sizeof(Tup) );
    return t;
}

ten_DatInfo*
ten_getDatInfo( ten_State* s, ten_Var* dat ) {
    State* state = (State*)s;
    TVal datV = ref(dat);
    funAssert(
        tvIsObj( datV ) && datGetTag( tvGetObj( datV ) ) == OBJ_DAT,
        "Wrong type for 'dat', need Dat",
        NULL
    );
    Data* datO = tvGetObj( datV );
    return (ten_DatInfo*)datO->info;
}

char const*
ten_getDatType( ten_State* s, ten_Var* dat ) {
    State* state = (State*)s;
    TVal datV = ref(dat);
    funAssert(
        tvIsObj( datV ) && datGetTag( tvGetObj( datV ) ) == OBJ_DAT,
        "Wrong type for 'dat', need Dat",
        NULL
    );
    Data* datO = tvGetObj( datV );
    return symBuf( state, datO->info->type );
}

void*
ten_getDatBuf( ten_State* s, ten_Var* dat ) {
    State* state = (State*)s;
    TVal datV = ref(dat);
    funAssert(
        tvIsObj( datV ) && datGetTag( tvGetObj( datV ) ) == OBJ_DAT,
        "Wrong type for 'dat', need Dat",
        NULL
    );
    Data* datO = tvGetObj( datV );
    return datO->data;
}

void
ten_initDatInfo( ten_State* s, ten_DatConfig* config, ten_DatInfo* info ) {
    tenAssert( sizeof(ten_DatInfo) >= sizeof(DatInfo) );
    
    State* state = (State*)s;
    
    DatInfo* dInfo = (DatInfo*)info;
    
    char const* type;
    if( config->tag )
        type = fmtA( state, false, "Dat:%s", config->tag );
    else
        type = "Dat";
    
    dInfo->type  = symGet( state, type, strlen( type ) );
    dInfo->size  = config->size;
    dInfo->nMems = config->mems;
    dInfo->destr = config->destr;
    dInfo->magic = DAT_MAGIC;
}
