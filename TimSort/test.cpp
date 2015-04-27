#include <iostream>
#include <time.h>
#include "Timsort.h"
#include <list>
#include <vector>
#include <algorithm>

namespace
{
    class TestTimSort
    {
    public:
        static void JDKWorstCase(std::vector<long>& dst, int length)
        {
            TestTimSort t(length);
            t.runsJDKWorstCase(length);
            t.createArray(dst, length);
        }

        static void CorrectWorstCase(std::vector<long>& dst, int length)
        {
            TestTimSort t(length);
            t.runsCorrectWorstCase(length);
            t.createArray(dst, length);
        }

        static void create(std::vector<long>& dst, int length)
        {
            JDKWorstCase(dst, length);
        }
    private:
        static const int MIN_MERGE = 32;
        //private static final Comparator<Object> NATURAL_ORDER =
        //    new Comparator<Object>() {
        //    @SuppressWarnings("unchecked")
        //        public int compare(Object first, Object second) {
        //        return ((Comparable<Object>)first).compareTo(second);
        //    }
        //};

        TestTimSort(int len)
        {
            minRun_ = minRunLength(len);
        }

        int minRun_;
        std::list<long> runs_;

        static int minRunLength(int n)
        {
            int r = 0;      // Becomes 1 if any 1 bits are shifted off
            while(n >= MIN_MERGE) {
                r |= (n & 1);
                n >>= 1;
            }
            return n + r;
        }

        /**
        * Adds a sequence x_1, ..., x_n of run lengths to <code>runs</code> such that:<br>
        * 1. X = x_1 + ... + x_n <br>
        * 2. x_j >= minRun for all j <br>
        * 3. x_1 + ... + x_{j-2}  <  x_j  <  x_1 + ... + x_{j-1} for all j <br>
        * These conditions guarantee that TimSort merges all x_j's one by one
        * (resulting in X) using only merges on the second-to-last element.
        * @param X  The sum of the sequence that should be added to runs.
        */
        void generateJDKWrongElem(long X)
        {
            for(long newTotal; X >= 2*minRun_+1; X = newTotal) {
                //Default strategy
                newTotal = X/2 + 1;

                //Specialized strategies
                if(3*minRun_+3 <= X && X <= 4*minRun_+1) {
                    // add x_1=MIN+1, x_2=MIN, x_3=X-newTotal  to runs
                    newTotal = 2*minRun_+1;
                } else if(5*minRun_+5 <= X && X <= 6*minRun_+5) {
                    // add x_1=MIN+1, x_2=MIN, x_3=MIN+2, x_4=X-newTotal  to runs
                    newTotal = 3*minRun_+3;
                } else if(8*minRun_+9 <= X && X <= 10*minRun_+9) {
                    // add x_1=MIN+1, x_2=MIN, x_3=MIN+2, x_4=2MIN+2, x_5=X-newTotal  to runs
                    newTotal = 5*minRun_+5;
                } else if(13*minRun_+15 <= X && X <= 16*minRun_+17) {
                    // add x_1=MIN+1, x_2=MIN, x_3=MIN+2, x_4=2MIN+2, x_5=3MIN+4, x_6=X-newTotal  to runs
                    newTotal = 8*minRun_+9;
                }
                runs_.push_front(X-newTotal);
            }
            runs_.push_front(X);
        }

        /**
        * Fills <code>runs</code> with a sequence of run lengths of the form<br>
        * Y_n     x_{n,1}   x_{n,2}   ... x_{n,l_n} <br>
        * Y_{n-1} x_{n-1,1} x_{n-1,2} ... x_{n-1,l_{n-1}} <br>
        * ... <br>
        * Y_1     x_{1,1}   x_{1,2}   ... x_{1,l_1}<br>
        * The Y_i's are chosen to satisfy the invariant throughout execution,
        * but the x_{i,j}'s are merged (by <code>TimSort.mergeCollapse</code>)
        * into an X_i that violates the invariant.
        * @param X  The sum of all run lengths that will be added to <code>runs</code>.
        */
        void runsJDKWorstCase(int length)
        {
            long runningTotal = 0, Y=minRun_+4, X=minRun_;

            while(runningTotal+Y+X <= length) {
                runningTotal += X + Y;
                generateJDKWrongElem(X);
                runs_.push_front(Y);
                // X_{i+1} = Y_i + x_{i,1} + 1, since runs.get(1) = x_{i,1} 
                std::list<long>::iterator itr = runs_.begin();
                std::advance(itr, 1);
                X = Y + *itr + 1;
                // Y_{i+1} = X_{i+1} + Y_i + 1
                Y += X + 1;
            }

            if(runningTotal + X <= length) {
                runningTotal += X;
                generateJDKWrongElem(X);
            }

            runs_.push_back(length-runningTotal);
        }

        void runsCorrectWorstCase(int len) {
            long curr = minRun_, prev=0;
            long runningTotal = len<minRun_ ? len : minRun_;

            runs_.push_back(runningTotal);

            while(runningTotal+prev+curr+1 <= len) {
                curr += prev+1;
                prev=curr-prev-1;
                runs_.push_front(curr);
                runningTotal += curr;
            }

            runs_.push_back(len-runningTotal);
        }

        void createArray(std::vector<long>& dst, int length)
        {
            dst.clear();
            dst.resize(length);
            std::fill(dst.begin(), dst.end(), 0);

            int endRun = -1;
            for(std::list<long>::iterator itr = runs_.begin();
                itr != runs_.end();
                ++itr)
            {
                dst[endRun+=*itr] = 1;
            }
            dst[length-1]=0;
        }
    };
}
typedef bool(*SortCompFuncType)(const long& lhs, const long& rhs);

/* Comparison function: PyObject_RichCompareBool with Py_LT.
 * Returns -1 on error, 1 if x < y, 0 if x >= y.
 */
bool compare(const long& lhs, const long& rhs)
{
    return (lhs<rhs);
}

template<class T>
void swap(T& v0, T& v1)
{
    T tmp = v0;
    v0 = v1;
    v1 = tmp;
}
#define USER_WORST_CASE (1)

int main(int argc, char** argv)
{
#if USER_WORST_CASE
    const lcore::s32 NumSamples = 67108864;
    std::vector<long> samples(NumSamples);
    TestTimSort::create(samples, NumSamples);
#else
    const lcore::s32 NumSamples = 1000*2;
    std::vector<long> samples(NumSamples);
    time_t timer;
    time(&timer);
    srand(timer);

    for(lcore::s32 i=0; i<NumSamples; i+=2){
        samples[i] = i;
        samples[i+1] = i;
    }

    for(lcore::s32 i=0; i<NumSamples; ++i){
        lcore::s32 s = rand() % NumSamples;
        swap(samples[i], samples[s]);
    }
#endif
    lcore::Timsort<long, SortCompFuncType>  timesort(compare);
    timesort.sort(&samples[0], NumSamples);

#if USER_WORST_CASE
    for(lcore::s32 i=1; i<NumSamples; ++i){
        if(samples[i] < samples[i-1]){
            LASSERT(false);
            std::cout << "wrong" << std::endl;
        }
    }
#else
    for(lcore::s32 i=1; i<NumSamples; ++i){
        LASSERT(samples[i-1]<=samples[i]);
        std::cout << samples[i] << std::endl;
    }
#endif
    return 0;
}
