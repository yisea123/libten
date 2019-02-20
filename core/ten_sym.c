#include "ten_sym.h"
#include "ten_state.h"
#include "ten_assert.h"
#include <string.h>
#include <limits.h>

typedef struct SymNode {
    struct SymNode*  next;
    struct SymNode** link;
    
    uint   hash;
    bool   mark;
    uint   loc;
    size_t len;
    char*  buf;
} SymNode;

#define SYM_SHORT_LIM (4)
#define SYM_SIZE_BYTE (sizeof(SymT)-1)

typedef union {
    SymT s;
    char b[sizeof(SymT)];
} SymBuf;

struct SymState {
    Finalizer finl;
    
    uint count;
    uint next;
    
    struct {
        uint      cap;
        SymNode** buf;
    } map;
    
    struct {
        uint      cap;
        SymNode** buf;
    } nodes;
    
    SymNode* recycled;
    SymBuf   symBuf;
};

#define addNode( LIST, NODE )                           \
    do {                                                \
        (NODE)->next = *(LIST);                         \
        (NODE)->link = (LIST);                          \
        *(LIST) = (NODE);                               \
        if( (NODE)->next )                              \
            (NODE)->next->link = &(NODE)->next;         \
    } while( 0 )

#define remNode( NODE )                                 \
    do {                                                \
        *(NODE)->link = (NODE)->next;                   \
        if( (NODE)->next )                              \
            (NODE)->next->link = (NODE)->link;          \
    } while( 0 )

static uint
nextMapCap( uint cap );

static uint
hash( char const* str, size_t len );

static void
growMap( State* state );

static void
symFinl( State* state, Finalizer* finl ) {
    SymState* symState = (SymState*)finl;
    
    stateFreeRaw(
        state,
        symState->map.buf,
        sizeof(SymNode*)*symState->map.cap
    );
    stateFreeRaw(
        state,
        symState->nodes.buf,
        sizeof(SymNode*)*symState->nodes.cap
    );
    stateFreeRaw( state, symState, sizeof(SymState) );
    state->symState = NULL;
}

void
symInit( State* state ) {
    uint mcap = nextMapCap( 0 );
    
    Part stateP;
    SymState* symState = stateAllocRaw( state, &stateP, sizeof(SymState) );
    
    Part mapP;
    SymNode** map = stateAllocRaw( state, &mapP, sizeof(SymNode*)*mcap );
    for( uint i = 0 ; i < mcap ; i++ )
        map[i] = NULL;
    
    uint ncap = 7;
    Part nodesP;
    SymNode** nodes = stateAllocRaw( state, &nodesP, sizeof(SymNode*)*ncap );
    for( uint i = 0 ; i < ncap ; i++ )
        nodes[i] = NULL;
    
    symState->count     = 0;
    symState->next      = 0;
    symState->map.cap   = mcap;
    symState->map.buf   = map;
    symState->nodes.cap = ncap;
    symState->nodes.buf = nodes;
    symState->recycled  = NULL;
    symState->finl.cb   = symFinl;
    stateInstallFinalizer( state, &symState->finl );
    stateCommitRaw( state, &stateP );
    stateCommitRaw( state, &mapP );
    stateCommitRaw( state, &nodesP );
    
    state->symState = symState;
}

SymT
symGet( State* state, char const* buf, size_t len ) {
    SymState* symState = state->symState;
    
    // Encode short symbols directly in the int value.
    if( len <= SYM_SHORT_LIM ) {
        SymBuf u;
        u.b[SYM_SIZE_BYTE] = len + 1;
        
        for( uint i = 0 ; i < len ; i++ )
            u.b[i] = buf[i];
        u.b[len] = '\0';
        return u.s;
    }
    
    // Look for an existing node with the same content.
    uint h = hash( buf, len );
    uint s = h % symState->map.cap;
    SymNode* node = symState->map.buf[s];
    while( node ) {
        if( node->len == len && !memcmp( node->buf, buf, len ) )
            break;
        node = node->next;
    }
    
    // If the symbol doesn't exist then either create or
    // recycle a node to put it in.
    if( !node ) {
        if( symState->recycled ) {
            node = symState->recycled;
            remNode( node );
        }
        else {
            Part nodeP;
            node = stateAllocRaw( state, &nodeP, sizeof(SymNode) );
            node->len = 0;
            node->buf = NULL;
            
            tenAssert( symState->next < UINT_MAX );
            node->loc = symState->next++;
            if( node->loc >= symState->nodes.cap ) {
                Part nodesP = {
                    .ptr = symState->nodes.buf,
                    .sz  = symState->nodes.cap*sizeof(SymNode*)
                };
                
                uint      cap   = symState->nodes.cap*2;
                SymNode** nodes = stateResizeRaw( state, &nodesP, sizeof(SymNode*)*cap );
                for( uint i = symState->nodes.cap ; i < cap ; i++ )
                    nodes[i] = NULL;
                
                symState->nodes.cap = cap;
                symState->nodes.buf = nodes;
                stateCommitRaw( state, &nodesP );
            }
            stateCommitRaw( state, &nodeP );
        }
        
        addNode( &symState->map.buf[s], node );
        
        Part conP;
        char* con = stateAllocRaw( state, &conP, len+1 );
        memcpy( con, buf, len );
        con[len] = '\0';
        stateCommitRaw( state, &conP );
        
        node->len  = len;
        node->buf  = con;
        node->mark = false;
        node->hash = h;
        symState->nodes.buf[node->loc] = node;
        symState->count++;
        
        if( symState->count*3 >= symState->map.cap )
            growMap( state );
    }
    
    return node->loc;
}

char const*
symBuf( State* state, SymT sym ) {
    SymState* symState = state->symState;
    
    // If it's a short symbol then copy it into the
    // persistent symbol buffer and return a pointer
    // to its contents.
    SymBuf u = {.s = sym };
    if( u.b[SYM_SIZE_BYTE] ) {
        symState->symBuf = u;
        return symState->symBuf.b;
    }
    
    // Otherwise return the node buffer.
    tenAssert( sym < symState->next );
    return symState->nodes.buf[sym]->buf;
}

size_t
symLen( State* state, SymT sym ) {
    SymState* symState = state->symState;
    
    // If it's a short symbol then extract the
    // length from the symbol value itself.
    SymBuf u = {.s = sym };
    if( u.b[SYM_SIZE_BYTE] )
        return u.b[SYM_SIZE_BYTE] - 1;
    
    // Otherwise return the length from the respective node.
    tenAssert( sym < symState->next );
    return symState->nodes.buf[sym]->len;
}

void
symStartCycle( State* state ) {
    // NADA
}

void
symMark( State* state, SymT sym ) {
    SymState* symState = state->symState;
    
    SymBuf u = {.s = sym };
    if( u.b[SYM_SIZE_BYTE] )
        return;
    
    tenAssert( sym < symState->next );
    SymNode* node = symState->nodes.buf[sym];
    node->mark = true;
}

void
symFinishCycle( State* state ) {
    SymState* symState = state->symState;
    
    for( uint i = 0 ; i < symState->next ; i++ ) {
        SymNode* node = symState->nodes.buf[i];
        if( !node )
            continue;
        if( node->mark ) {
            node->mark = false;
            continue;
        }
        
        remNode( node );
        addNode( &symState->recycled, node );
        symState->count--;
    }
}


static uint
nextMapCap( uint cap ) {
    // We try to use prime numbers for the map
    // size while it's within a reasonable size
    // to tabulate these for; otherwise revert
    // to doubling.  Adding more primes to this
    // table should improve performance for larger
    // Indices.
    switch( cap ) {
        case 0:    return 23;
        case 23:   return 47;
        case 47:   return 97;
        case 97:   return 199;
        case 199:  return 401;
        case 401:  return 809;
        case 809:  return 1601;
        case 1601: return 3217;
        case 3217: return 6421;
        default:   return cap*2;
    }
}

static uint
hash( char const* str, size_t len ) {
    uint h = 0;
    for( size_t i = 0 ; i < len ; i++ )
        h = h*37 + str[i];
    return h;
}

static void
growMap( State* state ) {
    SymState* symState = state->symState;
    
    uint mcap = nextMapCap( symState->map.cap );
    
    Part mapP;
    SymNode** map = stateAllocRaw( state, &mapP, sizeof(SymNode*)*mcap );
    for( uint i = 0 ; i < mcap ; i++ )
        map[i] = NULL;
    
    for( uint i = 0 ; i < symState->map.cap ; i++ ) {
        SymNode* node = symState->map.buf[i];
        if( !node )
            continue;
        
        uint s = node->hash % mcap;
        
        remNode( node );
        addNode( &map[s], node );
    }
    
    stateFreeRaw( state, symState->map.buf, sizeof(SymNode*)*symState->map.cap );
    stateCommitRaw( state, &mapP );
    symState->map.cap = mcap;
    symState->map.buf = map;
}

