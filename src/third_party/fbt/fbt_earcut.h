/*
MIT LICENSE
Copyright (c) 2026 Frances Telfar
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef FBTE_INCLUDE_FBT_EARCUT_H
#define FBTE_INCLUDE_FBT_EARCUT_H

#define FBTE_VERSION 1

#include <stdlib.h>
#include <math.h>
#include <string.h>
typedef unsigned char fbte_uc;
typedef unsigned short fbte_us;
typedef unsigned int fbte_ui;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FBTEDEF
#ifdef FBT_EARCUT_STATIC
#define FBTEDEF static
#else
#define FBTEDEF extern
#endif
#endif

typedef struct
{
    fbte_uc *vertex_data;
    int      vertex_count;
    int      vertex_field_stride;
    int      vertex_comp;
    fbte_ui *index_data;
    int      index_count;
} FBTE_Data;

FBTEDEF FBTE_Data *fbte_load_from_memory(fbte_uc const *point_buffer,
                                         int            point_count,
                                         int            field_stride,
                                         fbte_ui const *part_index_buffer,
                                         int            part_index_count,
                                         int            flags);
FBTEDEF void       fbte_earcut_free     (void *retval_from_fbte_load);

#ifdef __cplusplus
}
#endif

//
//
////    end header file    /////////////////////////////////

#endif // FBTE_INCLUDE_FBT_EARCUT_H

#ifdef FBT_EARCUT_IMPLEMENTATION

#ifdef __cplusplus
#define FBTE_EXTERN extern "C"
#else
#define FBTE_EXTERN extern
#endif

#ifndef _MSC_VER
# ifdef __cplusplus
#  define fbte_inline inline
# else
#  define fbte_inline
# endif
#else
# define fbte_inline __forceinline
#endif

#if defined(_MSC_VER) || defined(__SYMBIAN32__)
typedef unsigned short fbte__u16;
typedef   signed short fbte__s16;
typedef unsigned int   fbte__u32;
typedef   signed int   fbte__s32;
#else
#include <stdint.h>
typedef uint16_t       fbte__u16;
typedef int16_t        fbte__s16;
typedef uint32_t       fbte__u32;
typedef int32_t        fbte__s32;
#endif
typedef float          fbte__f32;
typedef double         fbte__f64;
typedef fbte__s32      fbte__bool;
#if defined(_MSC_VER) || defined(__SYMBIAN32__)
typedef unsigned long long fbte__u64;
#else
typedef uint64_t       fbte__u64;
#endif

typedef unsigned char validate_u32[sizeof(fbte__u32)==4 ? 1 : -1];

#ifdef _MSC_VER
# define FBTE_NOTUSED(v) (void)(v)
#else
# define FBTE_NOTUSED(v) (void)sizeof(v)
#endif

#if defined(FBTE_MALLOC) && defined(FBTE_FREE) && (defined(FBTE_REALLOC) || defined(FBTE_REALLOC_SIZED))
// ok
#elif !defined(FBTE_MALLOC) && !defined(FBTE_FREE) && !defined(FBTE_REALLOC) && !defined(FBTE_REALLOC_SIZED)
// ok
#else
# error "Must define all or none of FBTE_MALLOC, FBTE_FREE, and FBTE_REALLOC (or FBTE_REALLOC_SIZED)."
#endif

#ifndef FBTE_MALLOC
# define FBTE_MALLOC(sz)       malloc(sz)
# define FBTE_REALLOC(p,newsz) realloc(p,newsz)
# define FBTE_FREE(p)          free(p)
#endif

#ifndef FBTE_REALLOC_SIZED
# define FBTE_REALLOC_SIZED(p,oldsz,newsz) FBTE_REALLOC(p,newsz)
#endif

#define FBTE_EPSILON 1e-12

/////////////////////////
// Linked List Building Macros

// Linked List Macro Helpers
#define FBTE__CheckNil(nil,p) ((p) == 0 || (p) == nil)
#define FBTE__SetNil(nil,p) ((p) = nil)

// Doubly Linked Lists
#define FBTE__DLLInsert_NPZ(nil,f,l,p,n,next,prev) (FBTE__CheckNil(nil,f) ? \
((f) = (l) = (n), FBTE__SetNil(nil,(n)->next), FBTE__SetNil(nil,(n)->prev)) :\
FBTE__CheckNil(nil,p) ? \
((n)->next = (f), (f)->prev = (n), (f) = (n), FBTE__SetNil(nil,(n)->prev)) :\
((p)==(l)) ? \
((l)->next = (n), (n)->prev = (l), (l) = (n), FBTE__SetNil(nil, (n)->next)) :\
(((!FBTE__CheckNil(nil,p) && FBTE__CheckNil(nil,(p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define FBTE__DLLPushBack_NPZ(nil,f,l,n,next,prev) FBTE__DLLInsert_NPZ(nil,f,l,l,n,next,prev)
#define FBTE__DLLPushFront_NPZ(nil,f,l,n,next,prev) FBTE__DLLInsert_NPZ(nil,l,f,f,n,prev,next)
#define FBTE__DLLRemove_NPZ(nil,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)),\
((n) == (l) ? (l) = (l)->prev : (0)),\
(FBTE__CheckNil(nil,(n)->prev) ? (0) :\
((n)->prev->next = (n)->next)),\
(FBTE__CheckNil(nil,(n)->next) ? (0) :\
((n)->next->prev = (n)->prev)))

// doubly-linked-list helpers
#define FBTE__DLLInsert_NP(f,l,p,n,next,prev) FBTE__DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define FBTE__DLLPushBack_NP(f,l,n,next,prev) FBTE__DLLPushBack_NPZ(0,f,l,n,next,prev)
#define FBTE__DLLPushFront_NP(f,l,n,next,prev) FBTE__DLLPushFront_NPZ(0,f,l,n,next,prev)
#define FBTE__DLLRemove_NP(f,l,n,next,prev) FBTE__DLLRemove_NPZ(0,f,l,n,next,prev)
#define FBTE__DLLInsert(f,l,p,n) FBTE__DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define FBTE__DLLPushBack(f,l,n) FBTE__DLLPushBack_NPZ(0,f,l,n,next,prev)
#define FBTE__DLLPushFront(f,l,n) FBTE__DLLPushFront_NPZ(0,f,l,n,next,prev)
#define FBTE__DLLRemove(f,l,n) FBTE__DLLRemove_NPZ(0,f,l,n,next,prev)

typedef union FBTE__Point2f64 FBTE__Point2f64;
union FBTE__Point2f64
{
    struct
    {
        fbte__f64 x;
        fbte__f64 y;
    };
    fbte__f64 v[2];
};

typedef struct FBTE__Vertex FBTE__Vertex;
struct FBTE__Vertex
{
    FBTE__Point2f64 p;
    fbte__u32 index;
};

typedef struct FBTE__Vertex_Node FBTE__Vertex_Node;
struct FBTE__Vertex_Node
{
    FBTE__Vertex_Node *next;
    FBTE__Vertex_Node *prev;
    FBTE__Vertex v;
};

typedef struct FBTE__Vertex_List FBTE__Vertex_List;
struct FBTE__Vertex_List
{
    FBTE__Vertex_Node *first;
    FBTE__Vertex_Node *last;
    fbte__u64 count;
};

static void *fbte__malloc(fbte__u32 size)
{
    return FBTE_MALLOC(size);
}

static fbte__f64 fbte__orientation_2f64(FBTE__Point2f64 a, FBTE__Point2f64 b, FBTE__Point2f64 c)
{
    return (b.x - a.x) * (c.y - a.y)
         - (b.y - a.y) * (c.x - a.x);
}

static fbte__f64 fbte__dist_2f64(FBTE__Point2f64 a, FBTE__Point2f64 b)
{
    fbte__f64 dx = a.x - b.x;
    fbte__f64 dy = a.y - b.y;
    return dx*dx + dy*dy;
}

static fbte__bool fbte__match_2f64(FBTE__Point2f64 a, FBTE__Point2f64 b)
{
    return fabs(a.x - b.x) <= FBTE_EPSILON &&
        fabs(a.y - b.y) <= FBTE_EPSILON;
}

static fbte__bool fbte__point_on_segment_2f64(FBTE__Point2f64 p, FBTE__Point2f64 a, FBTE__Point2f64 b)
{
    if (fabs(fbte__orientation_2f64(a, b, p)) > FBTE_EPSILON)
    {
        return 0;
    }

    return p.x >= fmin(a.x, b.x) - FBTE_EPSILON &&
        p.x <= fmax(a.x, b.x) + FBTE_EPSILON &&
        p.y >= fmin(a.y, b.y) - FBTE_EPSILON &&
        p.y <= fmax(a.y, b.y) + FBTE_EPSILON;
}

static fbte__bool fbte__segments_intersect_2f64(FBTE__Point2f64 a, FBTE__Point2f64 b, FBTE__Point2f64 c, FBTE__Point2f64 d)
{
    fbte__f64 o1 = fbte__orientation_2f64(a, b, c);
    fbte__f64 o2 = fbte__orientation_2f64(a, b, d);
    fbte__f64 o3 = fbte__orientation_2f64(c, d, a);
    fbte__f64 o4 = fbte__orientation_2f64(c, d, b);

    if (((o1 > FBTE_EPSILON && o2 < -FBTE_EPSILON) ||
         (o1 < FBTE_EPSILON && o2 >  FBTE_EPSILON)) &&
        ((o3 > FBTE_EPSILON && o4 < -FBTE_EPSILON) ||
         (o3 < -FBTE_EPSILON && o4 > FBTE_EPSILON)))
    {
        return 1;
    }

    if (fabs(o1) <= FBTE_EPSILON && fbte__point_on_segment_2f64(c, a, b))
    {
        return 1;
    }
    if (fabs(o2) <= FBTE_EPSILON && fbte__point_on_segment_2f64(d, a, b))
    {
        return 1;
    }
    if (fabs(o3) <= FBTE_EPSILON && fbte__point_on_segment_2f64(a, c, d))
    {
        return 1;
    }
    if (fabs(o4) <= FBTE_EPSILON && fbte__point_on_segment_2f64(b, c, d))
    {
        return 1;
    }
    return 0;
}

static fbte__bool fbte__point_in_triangle_2f64(FBTE__Point2f64 a, FBTE__Point2f64 b, FBTE__Point2f64 c, FBTE__Point2f64 p)
{
    fbte__f64 d1 = fbte__orientation_2f64(a, b, p);
    fbte__f64 d2 = fbte__orientation_2f64(b, c, p);
    fbte__f64 d3 = fbte__orientation_2f64(c, a, p);

    fbte__bool has_neg = 0;
    if (d1 < -FBTE_EPSILON) has_neg = 1;
    if (d2 < -FBTE_EPSILON) has_neg = 1;
    if (d3 < -FBTE_EPSILON) has_neg = 1;
    fbte__bool has_pos = 0;
    if (d1 > FBTE_EPSILON) has_pos = 1;
    if (d2 > FBTE_EPSILON) has_pos = 1;
    if (d3 > FBTE_EPSILON) has_pos = 1;
    return !(has_neg && has_pos);
}

static fbte__f64 fbte__signed_area2(FBTE__Vertex_Node *ring)
{
    fbte__f64 sum = 0.f;
    FBTE__Vertex_Node *p = ring;
    do
    {
        sum += p->v.p.x*p->next->v.p.y - p->next->v.p.x*p->v.p.y;
        p = p->next;
    } while (p != ring);
    return sum;
}

static fbte__bool fbte__segment_crosses_ring(FBTE__Point2f64 s0, FBTE__Point2f64 s1,
                                             FBTE__Vertex_Node *endpoint_a, FBTE__Vertex_Node *endpoint_b,
                                             FBTE__Vertex_Node *ring)
{
    FBTE__Vertex_Node *e = ring;
    do
    {
        FBTE__Vertex_Node *en = e->next;

        if (e != endpoint_a && e != endpoint_b && en != endpoint_a && en != endpoint_b)
        {
            if (fbte__segments_intersect_2f64(s0, s1, e->v.p, en->v.p))
            {
                return 1;
            }
        }
        e = en;
    } while (e != ring);

    return 0;
}

static fbte__bool fbte__is_ear(FBTE__Vertex_Node *ear, fbte__f64 area_sign)
{
    FBTE__Vertex_Node *a = ear->prev;
    FBTE__Vertex_Node *c = ear->next;
    fbte__f64 orient;
    FBTE__Vertex_Node *p;

    if (a == c || a == ear)
    {
        return 0;
    }

    orient = fbte__orientation_2f64(a->v.p, ear->v.p, c->v.p);

    if (area_sign <= FBTE_EPSILON && area_sign >= -FBTE_EPSILON)
    {
        return 0;
    }

    if ((area_sign > 0.0 && orient <= FBTE_EPSILON) ||
        (area_sign < 0.0 && orient >= -FBTE_EPSILON))
    {
        return 0;
    }

    for (p = c->next; p != a; p = p->next)
    {
        if (fbte__match_2f64(p->v.p, a->v.p) ||
            fbte__match_2f64(p->v.p, ear->v.p) ||
            fbte__match_2f64(p->v.p, c->v.p))
        {
            continue;
        }
        if (fbte__point_in_triangle_2f64(a->v.p, ear->v.p, c->v.p, p->v.p))
        {
            return 0;
        }
    }
    return 1;
}

static void fbte__reverse_ring(FBTE__Vertex_Node *ring)
{
    FBTE__Vertex_Node *p = ring;
    do
    {
        FBTE__Vertex_Node *original_next = p->next;
        p->next = p->prev;
        p->prev = original_next;
        p = original_next;
    } while (p != ring);
}

static FBTE__Vertex_Node *fbte__eliminate_hole(FBTE__Vertex_Node *outer_first,
                                               FBTE__Vertex_Node *hole_first,
                                               FBTE__Vertex_Node **out_a2,
                                               FBTE__Vertex_Node **out_b2)
{
    FBTE__Vertex_Node *hv;
    FBTE__Vertex_Node *bridge;
    FBTE__Vertex_Node *q;
    fbte__f64 ray_x;
    fbte__f64 best_x;
    fbte__f64 best_dist;
    fbte__f64 hy;
    FBTE__Vertex_Node *a2;
    FBTE__Vertex_Node *b2;
    FBTE__Vertex_Node *an;
    FBTE__Vertex_Node *bp;

    *out_a2 = 0;
    *out_b2 = 0;

    hv = hole_first;
    q = hole_first->next;
    while (q != hole_first)
    {
        if (q->v.p.x > hv->v.p.x ||
            (q->v.p.x == hv->v.p.x && q->v.p.y < hv->v.p.y))
        {
            hv = q;
        }
        q = q->next;
    }

    hy = hv->v.p.y;
    ray_x = hv->v.p.x;
    bridge = 0;
    best_x = INFINITY;

    q = outer_first;
    do
    {
        FBTE__Vertex_Node *n = q->next;
        fbte__f64 y0 = q->v.p.y;
        fbte__f64 y1 = n->v.p.y;

        if (fabs(y1 - y0) > FBTE_EPSILON &&
            ((y0 <= hy && hy < y1) || (y1 <= hy && hy < y0)))
        {
            fbte__f64 t = (hy - y0) / (y1 - y0);
            fbte__f64 x = q->v.p.x + t * (n->v.p.x - q->v.p.x);

            if (x > ray_x + FBTE_EPSILON && x < best_x)
            {
                best_x = x;
                bridge = q;
            }
        }

        q = n;
    } while (q != outer_first);

    if (bridge == 0)
    {
        return outer_first;
    }

    {
        FBTE__Vertex_Node *candidates[2];
        int candidate_count = 2;
        int i;

        if (fbte__dist_2f64(hv->v.p, bridge->v.p) <=
            fbte__dist_2f64(hv->v.p, bridge->next->v.p))
        {
            candidates[0] = bridge;
            candidates[1] = bridge->next;
        }
        else
        {
            candidates[0] = bridge->next;
            candidates[1] = bridge;
        }

        bridge = 0;
        best_dist = 0.0;

        for (i = 0; i < candidate_count; i++)
        {
            FBTE__Vertex_Node *candidate = candidates[i];

            if (!fbte__segment_crosses_ring(hv->v.p, candidate->v.p,
                                            hv, candidate, outer_first) &&
                !fbte__segment_crosses_ring(hv->v.p, candidate->v.p,
                                            hv, candidate, hole_first))
            {
                fbte__f64 d = fbte__dist_2f64(hv->v.p, candidate->v.p);
                if (bridge == 0 || d < best_dist)
                {
                    bridge = candidate;
                    best_dist = d;
                }
            }
        }
    }

    if (bridge == 0)
    {
        return outer_first;
    }

    a2 = (FBTE__Vertex_Node *)fbte__malloc((fbte__u32)sizeof(FBTE__Vertex_Node));
    b2 = (FBTE__Vertex_Node *)fbte__malloc((fbte__u32)sizeof(FBTE__Vertex_Node));
    if (a2 == 0 || b2 == 0)
    {
        FBTE_FREE(a2);
        FBTE_FREE(b2);
        return outer_first;
    }

    a2->v = bridge->v;
    b2->v = hv->v;

    an = bridge->next;
    bp = hv->prev;

    bridge->next = hv;
    hv->prev = bridge;

    a2->next = an;
    an->prev = a2;

    b2->next = a2;
    a2->prev = b2;

    bp->next = b2;
    b2->prev = bp;

    *out_a2 = a2;
    *out_b2 = b2;

    return outer_first;
}

static void fbte__triangulate_ring(FBTE__Vertex_Node *start, fbte_ui *out, int *out_count, int out_cap)
{
    FBTE__Vertex_Node *ear;
    fbte__f64 area_sign;
    int remaining;
    int guard;

    *out_count = 0;
    if (start == 0) return;

    area_sign = fbte__signed_area2(start);

    remaining = 0;
    {
        FBTE__Vertex_Node *q = start;
        do { remaining++; q = q->next; } while (q != start);
    }

    if (remaining < 3) return;

    ear = start;
    guard = 0;

    while (remaining > 3)
    {
        if (guard > remaining)
        {
            break;
        }

        if (fbte__is_ear(ear, area_sign))
        {
            FBTE__Vertex_Node *a = ear->prev;
            FBTE__Vertex_Node *c = ear->next;

            if (*out_count + 3 > out_cap) return;

            out[(*out_count)++] = (fbte_ui)a->v.index;
            out[(*out_count)++] = (fbte_ui)ear->v.index;
            out[(*out_count)++] = (fbte_ui)c->v.index;

            a->next = c;
            c->prev = a;

            ear = c;
            remaining--;
            guard = 0;
        }
        else
        {
            ear = ear->next;
            guard++;
        }
    }

    if (remaining == 3 && *out_count + 3 <= out_cap)
    {
        out[(*out_count)++] = (fbte_ui)ear->prev->v.index;
        out[(*out_count)++] = (fbte_ui)ear->v.index;
        out[(*out_count)++] = (fbte_ui)ear->next->v.index;
    }
}

FBTEDEF FBTE_Data *fbte_load_from_memory(fbte_uc const *point_buffer,
                                         int            point_count,
                                         int            field_stride,
                                         fbte_ui const *part_index_buffer,
                                         int            part_index_count,
                                         int            flags)
{
    FBTE_Data          *result;
    FBTE__Vertex_Node  *nodes;
    FBTE__Vertex_Node  *outer_first;
    FBTE__Vertex_Node **extras;
    int                 extras_cap;
    int                 extras_used;
    int                 stride;
    int                 hole_count;
    int                 ring_count;
    int                 i;
    fbte__f64          *out_verts;
    fbte_ui            *out_indices;
    int                 out_index_cap;
    int                 out_index_used;
    FBTE__Vertex_Node  *p;
 
    FBTE_NOTUSED(flags); /* reserved for future use */
 
    if (point_buffer == 0 || point_count < 3)
    {
        return 0;
    }
 
    stride     = (field_stride > 0) ? field_stride : (int)sizeof(fbte__f64);
    hole_count = (part_index_count > 1 && part_index_buffer != 0) ? part_index_count - 1 : 0;
 
    nodes = (FBTE__Vertex_Node *)fbte__malloc((fbte__u32)(sizeof(FBTE__Vertex_Node) * (fbte__u32)point_count));
    if (nodes == 0)
    {
        return 0;
    }
 
    for (i = 0; i < point_count; i++)
    {
        fbte__f64 xy[2];
        memcpy(xy, point_buffer + (size_t)i * (size_t)stride * 2, sizeof(xy));
        nodes[i].v.p.x   = xy[0];
        nodes[i].v.p.y   = xy[1];
        nodes[i].v.index = (fbte__u32)i;
        nodes[i].next    = 0;
        nodes[i].prev    = 0;
    }
 
    {
        int outer_start = (part_index_count > 0 && part_index_buffer != 0) ?
                          (int)part_index_buffer[0] : 0;
        int outer_end = (part_index_count > 1 && part_index_buffer != 0) ?
                        (int)part_index_buffer[1] : point_count;
        FBTE__Vertex_List lst;
 
        if (outer_start < 0) outer_start = 0;
        if (outer_start > point_count) outer_start = point_count;
        if (outer_end < outer_start) outer_end = outer_start;
        if (outer_end > point_count) outer_end = point_count;
 
        lst.first = 0;
        lst.last  = 0;
        lst.count = 0;
 
        for (i = outer_start; i < outer_end; i++)
        {
            FBTE__DLLPushBack(lst.first, lst.last, &nodes[i]);
            lst.count++;
        }
 
        if (lst.count < 3)
        {
            FBTE_FREE(nodes);
            return 0;
        }
 
        lst.last->next  = lst.first;
        lst.first->prev = lst.last;
        outer_first     = lst.first;
 
        if (fbte__signed_area2(outer_first) < 0.0)
        {
            fbte__reverse_ring(outer_first);
        }
    }
 
    extras      = 0;
    extras_cap  = hole_count * 2;
    extras_used = 0;
 
    if (extras_cap > 0)
    {
        extras = (FBTE__Vertex_Node **)fbte__malloc((fbte__u32)(sizeof(FBTE__Vertex_Node *) * (fbte__u32)extras_cap));
    }
 
    for (i = 0; i < hole_count; i++)
    {
        int part_index = i + 1;
        int start = (int)part_index_buffer[part_index];
        int end   = (part_index + 1 < part_index_count) ?
                    (int)part_index_buffer[part_index + 1] : point_count;
        FBTE__Vertex_List lst;
        FBTE__Vertex_Node *a2 = 0;
        FBTE__Vertex_Node *b2 = 0;
        int j;
 
        if (start < 0) start = 0;
        if (end > point_count) end = point_count;
        if (end - start < 3) continue;
 
        lst.first = 0;
        lst.last  = 0;
        lst.count = 0;
 
        for (j = start; j < end; j++)
        {
            FBTE__DLLPushBack(lst.first, lst.last, &nodes[j]);
            lst.count++;
        }
 
        lst.last->next  = lst.first;
        lst.first->prev = lst.last;
 
        if (fbte__signed_area2(lst.first) > 0.0)
        {
            fbte__reverse_ring(lst.first);
        }
 
        outer_first = fbte__eliminate_hole(outer_first, lst.first, &a2, &b2);
 
        if (a2 != 0 && extras != 0 && extras_used + 2 <= extras_cap)
        {
            extras[extras_used++] = a2;
            extras[extras_used++] = b2;
        }
    }
 
    ring_count = 0;
    p = outer_first;
    do { ring_count++; p = p->next; } while (p != outer_first);
 
    out_index_cap  = (ring_count >= 3) ? (ring_count - 2) * 3 : 0;
    out_index_used = 0;
    out_indices    = 0;
 
    if (out_index_cap > 0)
    {
        out_indices = (fbte_ui *)fbte__malloc((fbte__u32)(sizeof(fbte_ui) * (fbte__u32)out_index_cap));
        if (out_indices != 0)
        {
            fbte__triangulate_ring(outer_first, out_indices, &out_index_used, out_index_cap);
        }
        else
        {
            out_index_cap = 0;
        }
    }
 
    out_verts = (fbte__f64 *)fbte__malloc((fbte__u32)(sizeof(fbte__f64) * 2 * (fbte__u32)point_count));
    if (out_verts != 0)
    {
        for (i = 0; i < point_count; i++)
        {
            out_verts[i * 2 + 0] = nodes[i].v.p.x;
            out_verts[i * 2 + 1] = nodes[i].v.p.y;
        }
    }
 
    result = (FBTE_Data *)fbte__malloc((fbte__u32)sizeof(FBTE_Data));
    if (result != 0)
    {
        result->vertex_data  = (fbte_uc *)out_verts;
        result->vertex_count = point_count;
        result->vertex_field_stride = field_stride;
        result->vertex_comp  = 2;
        result->index_data   = out_indices;
        result->index_count  = out_index_used;
    }
    else
    {
        FBTE_FREE(out_verts);
        FBTE_FREE(out_indices);
    }
 
    for (i = 0; i < extras_used; i++)
    {
        FBTE_FREE(extras[i]);
    }
    FBTE_FREE(extras);
    FBTE_FREE(nodes);
 
    return result;
}

FBTEDEF void fbte_earcut_free(void *retval_from_fbte_load)
{
    FBTE_Data *data = (FBTE_Data *)retval_from_fbte_load;
    if (data == 0) return;
    FBTE_FREE(data->vertex_data);
    FBTE_FREE(data->index_data);
    FBTE_FREE(data);
}


#endif // FBT_EARCUT_IMPLEMENTATION
