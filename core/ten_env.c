#include "ten_env.h"
#include "ten_state.h"
#include "ten_macros.h"
#include "ten_assert.h"
#include "ten_ntab.h"
#include "ten_sym.h"
#include "ten_ptr.h"
#include <string.h>
#include <limits.h>

#define BUF_TYPE TVal
#define BUF_NAME ValBuf
    #include "inc/buf.inc"
#undef BUF_NAME
#undef BUF_TYPE


struct EnvState {
    Finalizer finl;
    Scanner   scan;
    
    NTab*  gNames;
    ValBuf gVals;
    
    ValBuf stack;
};

void
envFinl( State* state, Finalizer* finl ) {

}

void
envScan( State* state, Scanner* scan ) {

}

void
envInit( State* state ) {
    Part envP;
    EnvState* env = stateAllocRaw( state, &envP, sizeof(EnvState) );
    env->gNames  = ntabMake( state );
    env->finl.cb = envFinl;
    env->scan.cb = envScan;
    initValBuf( state, &env->gVals );
    initValBuf( state, &env->stack );
    
    stateInstallFinalizer( state, &env->finl );
    stateInstallScanner( state, &env->scan );
    
    state->envState = env;
}

Tup
envPush( State* state, uint n ) {
    EnvState* env = state->envState;
    
    uint offset = env->stack.top;
    for( uint i = 0 ; i < n ; i++ )
        *putValBuf( state, &env->stack ) = tvUdf();
    if( n != 1 )
        *putValBuf( state, &env->stack ) = tvTup( n );
    
    return (Tup){
        .base   = &env->stack.buf,
        .offset = offset,
        .size   = n
    };
}

Tup
envTop( State* state, uint n ) {
    EnvState* env = state->envState;
    tenAssert( env->stack.top > 0 );
    
    TVal* tupv = &env->stack.buf[env->stack.top-1];
    uint  tupc = 1;
    if( tvIsTup( *tupv ) ) {
        tupc = tvGetTup( *tupv );
        tupv -= tupc;
    }
    
    return (Tup){
        .base   = &env->stack.buf,
        .offset = tupv - env->stack.buf,
        .size   = tupc
    };
}

void
envPop( State* state ) {
    EnvState* env = state->envState;
    tenAssert( env->stack.top > 0 );
    
    TVal* tupv = &env->stack.buf[env->stack.top-1];
    uint  tupc = 1;
    if( tvIsTup( *tupv ) ) {
        tupc = tvGetTup( *tupv );
        tupv -= tupc;
    }
    env->stack.top = tupv - env->stack.buf;
}

uint
envAddGlobal( State* state, SymT name ) {
    EnvState* env = state->envState;
    uint loc = ntabAdd( state, env->gNames, name );
    while( loc >= env->gVals.top )
        *putValBuf( state, &env->gVals ) = tvUdf();
    
    return loc;
}

TVal*
envGetGlobalByName( State* state, SymT name ) {
    EnvState* env = state->envState;
    uint loc = ntabGet( state, env->gNames, name );
    if( loc == UINT_MAX )
        return NULL;
    return &env->gVals.buf[loc];
}

TVal*
envGetGlobalByLoc( State* state, uint loc ) {
    EnvState* env = state->envState;
    tenAssert( loc < env->gVals.top );
    
    return &env->gVals.buf[loc];
}
