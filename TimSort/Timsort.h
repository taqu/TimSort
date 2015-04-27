#ifndef INC_TIMSORT_H__
#define INC_TIMSORT_H__
/**
This is porting of Timsort in listobject.c from Python 3.4.3 .

PYTHON SOFTWARE FOUNDATION LICENSE VERSION 2
--------------------------------------------

1. This LICENSE AGREEMENT is between the Python Software Foundation
("PSF"), and the Individual or Organization ("Licensee") accessing and
otherwise using this software ("Python") in source or binary form and
its associated documentation.

2. Subject to the terms and conditions of this License Agreement, PSF hereby
grants Licensee a nonexclusive, royalty-free, world-wide license to reproduce,
analyze, test, perform and/or display publicly, prepare derivative works,
distribute, and otherwise use Python alone or in any derivative version,
provided, however, that PSF's License Agreement and PSF's notice of copyright,
i.e., "Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
2011, 2012, 2013, 2014, 2015 Python Software Foundation; All Rights Reserved"
are retained in Python alone or in any derivative version prepared by Licensee.

3. In the event Licensee prepares a derivative work that is based on
or incorporates Python or any part thereof, and wants to make
the derivative work available to others as provided herein, then
Licensee hereby agrees to include in any such work a brief summary of
the changes made to Python.

4. PSF is making Python available to Licensee on an "AS IS"
basis.  PSF MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR
IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, PSF MAKES NO AND
DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS
FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF PYTHON WILL NOT
INFRINGE ANY THIRD PARTY RIGHTS.

5. PSF SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON
FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS
A RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON,
OR ANY DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.

6. This License Agreement will automatically terminate upon a material
breach of its terms and conditions.

7. Nothing in this License Agreement shall be deemed to create any
relationship of agency, partnership, or joint venture between PSF and
Licensee.  This License Agreement does not grant permission to use PSF
trademarks or trade name in a trademark sense to endorse or promote
products or services of Licensee, or any third party.

8. By copying, installing or otherwise using Python, Licensee
agrees to be bound by the terms and conditions of this License
Agreement.
*/

/**
@file Timsort.h
@author t-sakai
@date 2015/04/21 create
*/
#include <string.h>
#include <cassert>
#include <malloc.h>

namespace lcore
{
#ifndef NULL
#define NULL (0)
#endif

#define LASSERT(exp) assert(exp)

    typedef __int8 s8;
    typedef __int16 s16;
    typedef __int32 s32;

    typedef unsigned __int8 u8;
    typedef unsigned __int16 u16;
    typedef unsigned __int32 u32;

    typedef float f32;
    typedef double f64;

    template<class T>
    struct SortCompFuncType
    {
        /**
        @return true if lhs<rhs, false if lhs>=rhs
        */
        bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs<rhs;
        }
    };

    /**
    @return true if lhs<rhs, false if lhs>=rhs
    */
    //template<class T>
    //typedef bool(*SortCompFunc)(const T& lhs, const T& rhs);

    /**
    @return true if lhs<rhs, false if lhs>=rhs
    */
    template<class T>
    bool less(const T& lhs, const T& rhs)
    {
        return (lhs<rhs);
    }

    //------------------------------------------------
    //---
    //--- Timsort
    //---
    //------------------------------------------------
    template<class T, class Comp=SortCompFuncType<T> >
    class Timsort
    {
    public:
        static const s32 MAX_MERGE_PENDING = 85;
        static const s32 MIN_GALLOP = 7;
        static const s32 MERGESTATE_TEMP_SIZE = 256;

        typedef T value_type;
        typedef value_type* pointer_type;
        typedef const value_type* const_pointer_type;

        typedef value_type& reference_type;
        typedef const value_type& const_reference_type;
        typedef Comp comp_type;

        Timsort(comp_type compFunc)
            :compFunc_(compFunc)
        {}

        ~Timsort()
        {
            merge_free();
        }

        void sort(pointer_type keys, s32 size);
    private:
        Timsort(const Timsort&);
        Timsort& operator=(const Timsort&);

        inline static s32 minimum(s32 v0, s32 v1)
        {
            return (v0<v1)? v0 : v1;
        }

        inline static s32 maximum(s32 v0, s32 v1)
        {
            return (v0<v1)? v1 : v0;
        }

        struct sortslice
        {
            pointer_type keys_;

            static inline void copy(sortslice& s0, s32 i, sortslice& s1, s32 j)
            {
                s0.keys_[i] = s1.keys_[j];
            }

            static inline void copy_incr(sortslice& s0, sortslice& s1)
            {
                *s0.keys_++ = *s1.keys_++;
            }

            static inline void copy_decr(sortslice& s0, sortslice& s1)
            {
                *s0.keys_-- = *s1.keys_--;
            }

            static inline void memcpy(sortslice& s0, s32 i, sortslice& s1, s32 j, s32 n)
            {
                ::memcpy(&s0.keys_[i], &s1.keys_[j], sizeof(value_type)*n);
            }

            static inline void memmove(sortslice& s0, s32 i, sortslice& s1, s32 j, s32 n)
            {
                ::memmove(&s0.keys_[i], &s1.keys_[j], sizeof(value_type)*n);
            }

            static inline void advance(sortslice& slice, s32 n)
            {
                slice.keys_ += n;
            }

            static void reverse(pointer_type lo, pointer_type hi);
            static inline void reverse(sortslice& slice, s32 n)
            {
                reverse(slice.keys_, &slice.keys_[n]);
            }
        };

        struct s_slice
        {
            sortslice base_;
            s32 length_;
        };

        /**
        @brief �}���ʒu��2���T������}���\�[�g
        */
        void binarysort(sortslice lo, pointer_type hi, pointer_type start);

        /**
        @brief �擪���珇�����A�����镔���z��̒�����Ԃ�.
        @param lo ... �擪
        @param hi ... �I�[
        @param descending ... �o��. �~����
        */
        s32 count_run(pointer_type lo, pointer_type hi, s32& descending);

        /**
        @return p ... a[p-1]< key <=a[p]
        @param key ...
        @param a ... �\�[�g�ςݔz��
        @param n ... 0<n
        @param hint ... 0<= hint <n
        */
        s32 gallop_left(const_reference_type key, pointer_type a, s32 n, s32 hint);

        /**
        @return p ... a[p-1]<= key <a[p]
        @param key ...
        @param a ... �\�[�g�ςݔz��
        @param n ... 0<n
        @param hint ... 0<= hint <n
        */
        s32 gallop_right(const_reference_type key, pointer_type a, s32 n, s32 hint);

        void merge_init(s32 size);
        void merge_free();
        s32 merge_getmem(s32 need);
        s32 merge_compute_minrun(s32 n);

#define MERGE_GETMEM(NEED) ((NEED)<=allocated_? 0 : merge_getmem(NEED))

        /**
        @brief ssa��ssb���}�[�W
        ssb.keys_ == ssa.keys_+na, 0<na, 0<nb, na<=nb
        */
        void merge_lo(sortslice ssa, s32 na, sortslice ssb, s32 nb);

        /**
        @brief ssa��ssb���}�[�W
        ssb.keys_ == ssa.keys_+na, 0<na, 0<nb, nb<=na
        */
        void merge_hi(sortslice ssa, s32 na, sortslice ssb, s32 nb);

        /**
        @brief

        pending�X�^�b�N���ȉ��̕s�����𖞂����܂ŏ���.
        1. len[-3] > len[-2] + len[-1]
        2. len[-2] > len[-1]
        */
        void merge_collapse();
        void merge_force_collapse();

        /**
        @brief �X�^�b�N��n, n+1�Ԗڂ̕�������}�[�W
        */
        void merge_at(s32 n);

        comp_type compFunc_;

        s32 min_gallop_;
        sortslice a_;
        s32 allocated_;
        s32 numPendings_;
        s_slice pending_[MAX_MERGE_PENDING];
        value_type temparray_[MERGESTATE_TEMP_SIZE];
    };

    template<class T, class Comp>
    void Timsort<T,Comp>::sortslice::reverse(pointer_type lo, pointer_type hi)
    {
        LASSERT(NULL != lo);
        LASSERT(NULL != hi);
        --hi;
        while(lo<hi){
            value_type t = *lo;
            *lo = *hi;
            *hi = t;
            ++lo;
            --hi;
        }
    }

    // �}���ʒu��2���T������}���\�[�g
    template<class T, class Comp>
    void Timsort<T, Comp>::binarysort(sortslice lo, pointer_type hi, pointer_type start)
    {
        LASSERT(lo.keys_<=start && start<=hi);
        // [lo, start)�̓\�[�g�ς�
        if(lo.keys_ == start){
            ++start;
        }

        pointer_type l;
        pointer_type p;
        pointer_type r;

        value_type pivot;

        for(; start<hi; ++start){
            l = lo.keys_;
            r = start;
            pivot = *r;

            //lo.keys_[l] == pivot�ƂȂ�l��T��
            do{
                p = l + ((r-l)>>1);
                if(compFunc_(pivot, *p)){
                    r = p;
                } else{
                    l = p+1;
                }
            } while(l<r);
            LASSERT(l==r);

            //�z��V�t�g
            for(p=start; l<p; --p){
                *p = *(p-1);
            }
            *l = pivot;
        }
    }

    // �擪���珇�����A�����镔���z��̒�����Ԃ�.
    template<class T, class Comp>
    s32 Timsort<T, Comp>::count_run(pointer_type lo, pointer_type hi, s32& descending)
    {
        LASSERT(lo<hi);
        descending = 0;
        ++lo;
        if(lo == hi){
            return 1;
        }
        s32 n = 2;
        if(compFunc_(*lo, *(lo-1))){
            descending = 1;
            for(lo=lo+1; lo<hi; ++lo, ++n){
                if(false == compFunc_(*lo, *(lo-1))){
                    break;
                }
            }
        } else{
            for(lo=lo+1; lo<hi; ++lo, ++n){
                if(compFunc_(*lo, *(lo-1))){
                    break;
                }
            }
        }
        return n;
    }

    // p ... a[p-1]< key <=a[p]
    template<class T, class Comp>
    s32 Timsort<T, Comp>::gallop_left(const_reference_type key, pointer_type a, s32 n, s32 hint)
    {
        LASSERT(NULL != a);
        LASSERT(0<n);
        LASSERT(0<=hint && hint<n);

        a += hint;
        s32 lastoffset = 0;
        s32 offset = 1;
        if(compFunc_(*a, key)){
            //a[hint] < key, gallop right
            const s32 maxoffset = n-hint;
            while(offset<maxoffset){
                if(compFunc_(a[offset], key)){
                    lastoffset = offset;
                    offset = (offset<<1)+1;
                    if(offset<=0){//int overflow
                        offset = maxoffset;
                    }
                } else{
                    break;
                }
            }// while(offset<maxoffset)
            // a[hint + lastoffset] < key <= a[hint + offset]
            offset = minimum(offset, maxoffset);
            lastoffset += hint;
            offset += hint;

        } else{
            //key <= a[hint], gallop left
            const s32 maxoffset = hint+1;
            while(offset<maxoffset){
                if(compFunc_(a[-offset], key)){
                    break;
                } else{
                    lastoffset = offset;
                    offset = (offset<<1)+1;
                    if(offset<=0){//int overflow
                        offset = maxoffset;
                    }
                }
            }
            // a[hint - offset] < key <= a[hint - lastoffset]
            offset = minimum(offset, maxoffset);

            //lastoffset��offset���ꊷ��
            s32 t = lastoffset;
            lastoffset = hint - offset;
            offset = hint - t;
        }
        a -= hint;
        LASSERT(-1<=lastoffset && lastoffset<offset);
        LASSERT(offset<=n);

        //a[lastoffset] < key <= a[offset]
        //binary search
        ++lastoffset;
        while(lastoffset<offset){
            s32 m = lastoffset + ((offset-lastoffset)>>1);
            if(compFunc_(a[m], key)){
                lastoffset = m + 1;
            } else{
                offset = m;
            }
        }
        LASSERT(lastoffset == offset);
        return offset;
    }

    // p ... a[p-1]<= key <a[p]
    template<class T, class Comp>
    s32 Timsort<T, Comp>::gallop_right(const_reference_type key, pointer_type a, s32 n, s32 hint)
    {
        LASSERT(NULL != a);
        LASSERT(0<n);
        LASSERT(0<=hint && hint<n);

        a += hint;
        s32 lastoffset = 0;
        s32 offset = 1;
        if(compFunc_(key, *a)){
            //key < a[hint], gallop left
            const s32 maxoffset = hint + 1;
            while(offset<maxoffset){
                if(compFunc_(key, a[-offset])){
                    lastoffset = offset;
                    offset = (offset<<1)+1;
                    if(offset<=0){//int overflow
                        offset = maxoffset;
                    }
                } else{
                    break;
                }
            }// while(offset<maxoffset)
            // a[hint - offset] <= key < a[hint - lastoffset]
            offset = minimum(offset, maxoffset);

            //lastoffset��offset���ꊷ��
            s32 t = lastoffset;
            lastoffset = hint - offset;
            offset = hint - t;

        } else{
            //key <= a[hint], gallop left
            const s32 maxoffset = n-hint;
            while(offset<maxoffset){
                if(compFunc_(key, a[offset])){
                    break;
                } else{
                    lastoffset = offset;
                    offset = (offset<<1)+1;
                    if(offset<=0){//int overflow
                        offset = maxoffset;
                    }
                }
            }
            // a[hint + lastoffset] <= key < a[hint + offset]
            offset = minimum(offset, maxoffset);

            lastoffset += hint;
            offset += hint;
        }
        a -= hint;
        LASSERT(-1<=lastoffset && lastoffset<offset);
        LASSERT(offset<=n);

        //a[lastoffset] <= key < a[offset]
        //binary search
        ++lastoffset;
        while(lastoffset<offset){
            s32 m = lastoffset + ((offset-lastoffset)>>1);
            if(compFunc_(key, a[m])){
                offset = m;
            } else{
                lastoffset = m+1;
            }
        }
        LASSERT(lastoffset == offset);
        return offset;
    }

    template<class T, class Comp>
    void Timsort<T, Comp>::merge_init(s32 size)
    {
        allocated_ = (size + 1)/2;
        if(MERGESTATE_TEMP_SIZE/2<allocated_){
            allocated_ = MERGESTATE_TEMP_SIZE/2;
        }
        a_.keys_ = temparray_;
        numPendings_ = 0;
        min_gallop_ = MIN_GALLOP;
    }

    template<class T, class Comp>
    void Timsort<T, Comp>::merge_free()
    {
        if(a_.keys_ != temparray_){
            free(a_.keys_);
        }
    }

    template<class T, class Comp>
    s32 Timsort<T, Comp>::merge_getmem(s32 need)
    {
        merge_free();

        a_.keys_ = (pointer_type)malloc(need * sizeof(value_type));
        if(NULL == a_.keys_){
            return -1;
        }
        allocated_ = need;
        return 0;
    }

    template<class T, class Comp>
    s32 Timsort<T, Comp>::merge_compute_minrun(s32 n)
    {
        LASSERT(0<=n);
        s32 r=0;
        while(64<=n){
            r |= n&1;
            n >>= 1;
        }
        return n+r;
    }

    template<class T, class Comp>
    void Timsort<T, Comp>::merge_collapse()
    {
        struct s_slice* p = pending_;
        while(1<numPendings_){
            s32 n = numPendings_ - 2;
            if(0<n && p[n-1].length_<=p[n].length_ + p[n+1].length_){
                if(p[n-1].length_<p[n+1].length_){
                    --n;
                }
                merge_at(n);

            } else if(p[n].length_<=p[n+1].length_){
                merge_at(n);

            } else {
                break;
            }
        }
    }

    template<class T, class Comp>
    void Timsort<T, Comp>::merge_force_collapse()
    {
        s_slice* p = pending_;
        while(1<numPendings_){
            s32 n = numPendings_ - 2;
            if(0<n && p[n-1].length_<p[n+1].length_){
                --n;
            }
            merge_at(n);
        }
    }

    // ssa��ssb���}�[�W, na<=nb
    template<class T, class Comp>
    void Timsort<T, Comp>::merge_lo(sortslice ssa, s32 na, sortslice ssb, s32 nb)
    {
        LASSERT(0<na && 0<nb);
        LASSERT(ssa.keys_ + na == ssb.keys_);
        MERGE_GETMEM(na);

        //temporary�ɃR�s�[
        sortslice::memcpy(a_, 0, ssa, 0, na);
        sortslice dest = ssa;
        ssa = a_;

        //marge_at��gallop_right��, ssb.keys_[0] < ssa.keys_[0]�ɂȂ��Ă���
        sortslice::copy_incr(dest, ssb);
        --nb;

        if(0 == nb){
            goto Succeed;
        }
        if(1 == na){
            goto CopyB;
        }

        s32 min_gallop = min_gallop_;
        s32 k;
        for(;;){
            s32 acount = 0;
            s32 bcount = 0;

            //ssa, ssb�ǂ��炩����, �A������min_gallop�I������܂Ń}�[�W
            //�l�������ꍇ��ssa��I��
            for(;;){
                LASSERT(1<na && 0<nb);
                if(compFunc_(ssb.keys_[0], ssa.keys_[0])){
                    sortslice::copy_incr(dest, ssb);
                    ++bcount;
                    acount = 0;
                    --nb;
                    if(0==nb){
                        goto Succeed;
                    }
                    if(min_gallop<=bcount){
                        break;
                    }
                } else{
                    sortslice::copy_incr(dest, ssa);
                    ++acount;
                    bcount = 0;
                    --na;
                    if(1 == na){
                        goto CopyB;
                    }
                    if(min_gallop<=acount){
                        break;
                    }
                } //if(compFunc_
            } //for(;;){

            ++min_gallop;
            do{
                LASSERT(1<na && 0<nb);
                min_gallop -= (1<min_gallop);
                min_gallop_ = min_gallop;

                //ssa.keys_[k-1]<= ssb.keys_[0] < ssa.keys_[k], 0<k
                k = gallop_right(ssb.keys_[0], ssa.keys_, na, 0);
                acount = k;
                if(k){
                    sortslice::memcpy(dest, 0, ssa, 0, k);
                    sortslice::advance(dest, k);
                    sortslice::advance(ssa, k);
                    na -= k;
                    if(1 == na){
                        goto CopyB;
                    }
                    //��r�֐�����т��Ă����na==0�ɂ͂Ȃ�Ȃ�
                    if(0 == na){
                        goto Succeed;
                    }
                }
                sortslice::copy_incr(dest, ssb);
                --nb;
                if(0 == nb){
                    goto Succeed;
                }
                //ssb.keys_[k-1]< ssa.keys_[0] <= ssb.keys_[k], 0<k
                k = gallop_left(ssa.keys_[0], ssb.keys_, nb, 0);
                bcount = k;
                if(k){
                    sortslice::memmove(dest, 0, ssb, 0, k);
                    sortslice::advance(dest, k);
                    sortslice::advance(ssb, k);
                    nb -= k;
                    if(0 == nb){
                        goto Succeed;
                    }
                }
                sortslice::copy_incr(dest, ssa);
                --na;
                if(1 == na){
                    goto CopyB;
                }
            } while(MIN_GALLOP <= acount || MIN_GALLOP <= bcount);
            ++min_gallop;
            min_gallop_ = min_gallop;
        }
Succeed:
        if(na){
            sortslice::memcpy(dest, 0, ssa, 0, na);
        }
        return;
CopyB:
        //ssb�̎c��, ssa[0]���R�s�[
        //ssb�̎c�� < ssa[0]
        LASSERT(1 == na && 0<nb);
        sortslice::memmove(dest, 0, ssb, 0, nb);
        sortslice::copy(dest, nb, ssa, 0);
    }

    // ssa��ssb���}�[�W, nb<=na
    template<class T, class Comp>
    void Timsort<T, Comp>::merge_hi(sortslice ssa, s32 na, sortslice ssb, s32 nb)
    {
        LASSERT(0<na && 0<nb);
        LASSERT(ssa.keys_ + na == ssb.keys_);

        MERGE_GETMEM(nb);

        //temporary�ɃR�s�[
        sortslice dest = ssb;
        sortslice::advance(dest, nb-1);
        sortslice::memcpy(a_, 0, ssb, 0, nb);
        sortslice basea = ssa;
        sortslice baseb = a_;
        ssb.keys_ = a_.keys_ + nb - 1;
        sortslice::advance(ssa, na-1);
        //marge_at��gallop_left��, ssb.keys_[nb-1]< ssb.keys_[na-1] <= ssb.keys_[nb]
        sortslice::copy_decr(dest, ssa);
        --na;
        if(0 == na){
            goto Succeed;
        }
        if(1 == nb){
            goto CopyA;
        }

        s32 min_gallop = min_gallop_;
        s32 k;
        for(;;){
            s32 acount = 0;
            s32 bcount = 0;

            //ssa, ssb�ǂ��炩����, �A������min_gallop�I������܂Ń}�[�W
            //�l�������ꍇ��ssa��I��
            for(;;){
                LASSERT(0<na && 1<nb);
                if(compFunc_(ssb.keys_[0], ssa.keys_[0])){
                    sortslice::copy_decr(dest, ssa);
                    ++acount;
                    bcount = 0;
                    --na;
                    if(0==na){
                        goto Succeed;
                    }
                    if(min_gallop<=acount){
                        break;
                    }
                } else{
                    sortslice::copy_decr(dest, ssb);
                    ++bcount;
                    acount = 0;
                    --nb;
                    if(1 == nb){
                        goto CopyA;
                    }
                    if(min_gallop<=bcount){
                        break;
                    }
                } //if(compFunc_
            } //for(;;){

            ++min_gallop;
            do{
                LASSERT(0<na && 1<nb);
                min_gallop -= (1<min_gallop);
                min_gallop_ = min_gallop;

                //basea.keys_[k-1]<= ssb.keys_[0] < basea.keys_[k], 0<k
                k = gallop_right(ssb.keys_[0], basea.keys_, na, na-1);
                k = na - k;
                acount = k;
                if(k){
                    sortslice::advance(dest, -k);
                    sortslice::advance(ssa, -k);
                    sortslice::memmove(dest, 1, ssa, 1, k);
                    na -= k;
                    if(0 == na){
                        goto Succeed;
                    }
                }
                sortslice::copy_decr(dest, ssb);
                --nb;
                if(1 == nb){
                    goto CopyA;
                }
                //baseb.keys_[k-1]< ssa.keys_[0] <= baseb.keys_[k], 0<k
                k = gallop_left(ssa.keys_[0], baseb.keys_, nb, nb-1);
                k = nb - k;
                bcount = k;
                if(k){
                    sortslice::advance(dest, -k);
                    sortslice::advance(ssb, -k);
                    sortslice::memcpy(dest, 1, ssb, 1, k);
                    nb -= k;
                    if(1 == nb){
                        goto CopyA;
                    }
                    //��r�֐�����т��Ă����nb==0�ɂ͂Ȃ�Ȃ�
                    if(0 == nb){
                        goto Succeed;
                    }
                }
                sortslice::copy_decr(dest, ssa);
                --na;
                if(0 == na){
                    goto Succeed;
                }
            } while(MIN_GALLOP <= acount || MIN_GALLOP <= bcount);
            ++min_gallop;
            min_gallop_ = min_gallop;
        }
Succeed:
        if(nb){
            sortslice::memcpy(dest, -(nb-1), baseb, 0, nb);
        }
        return;
CopyA:
        LASSERT(1 == nb && 0<na);
        sortslice::memmove(dest, 1-na, ssa, 1-na, na);
        sortslice::advance(dest, -na);
        sortslice::advance(ssa, -na);
        sortslice::copy(dest, 0, ssb, 0);
    }

    // �X�^�b�N��n, n+1�Ԗڂ̕�������}�[�W
    template<class T, class Comp>
    void Timsort<T, Comp>::merge_at(s32 n)
    {
        LASSERT(2<=numPendings_);
        LASSERT(0<=n);
        LASSERT(n == numPendings_-2 || n == numPendings_-3);
        LASSERT(0<pending_[n].length_ && 0<pending_[n+1].length_);

        sortslice ssa = pending_[n].base_;
        s32 na = pending_[n].length_;
        sortslice ssb = pending_[n+1].base_;
        s32 nb = pending_[n+1].length_;
        LASSERT(0<na && 0<nb);
        LASSERT(ssa.keys_ + na == ssb.keys_);

        //�X�^�b�Nn�ԖڂɃ}�[�W���ʂ�ۑ�
        //n == numPendings_-3 �Ȃ��, �X�^�b�N��n+2���V�t�g
        if(n == numPendings_-3){
            pending_[n+1] = pending_[n+2];
        }
        pending_[n].length_ = na + nb;
        --numPendings_;

        s32 k;
        //ssa.keys_[k-1]<= ssb.keys_[0] < ssa.keys_[k]
        k = gallop_right(ssb.keys_[0], ssa.keys_, na, 0);
        sortslice::advance(ssa, k); //ssa��k-1�Ԗڂ܂ł�, ssb�ȉ�
        na -= k;
        if(na == 0){
            return;
        }

        //ssb.keys_[nb-1]< ssb.keys_[na-1] <= ssb.keys_[nb]
        nb = gallop_left(ssa.keys_[na-1], ssb.keys_, nb, nb-1);
        if(nb<=0){
            return;
        }
        if(na<=nb){
            merge_lo(ssa, na, ssb, nb);
        } else{
            merge_hi(ssa, na, ssb, nb);
        }
    }

    template<class T, class Comp>
    void Timsort<T, Comp>::sort(pointer_type keys, s32 size)
    {
        LASSERT(NULL != keys);
        LASSERT(0<=size);

        if(size<2){
            return;
        }
        sortslice lo;
        lo.keys_ = keys;
        merge_init(size);
        s32 minrun = merge_compute_minrun(size);

        do{
            s32 descending;
            s32 n = count_run(lo.keys_, lo.keys_+size, descending);
            if(descending){
                sortslice::reverse(lo, n);
            }

            if(n<minrun){
                const s32 force = (size<=minrun)? size : minrun;
                binarysort(lo, lo.keys_ + force, lo.keys_+n);
                n = force;
            }
            LASSERT(numPendings_<MAX_MERGE_PENDING);
            pending_[numPendings_].base_ = lo;
            pending_[numPendings_].length_ = n;
            ++numPendings_;
            merge_collapse();
            sortslice::advance(lo, n);
            size -= n;
        } while(0<size);
        merge_force_collapse();
    }
}
#endif //INC_TIMSORT_H__
